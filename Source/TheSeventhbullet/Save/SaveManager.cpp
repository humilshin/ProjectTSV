#include "Save/SaveManager.h"

#include "Data/SaveAndLoadGame.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

const FString USaveManager::SlotName = TEXT("TheSeventhBullet");

USaveManager* USaveManager::Get(const UObject* WorldContext)
{
    if (!WorldContext) return nullptr;
    UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContext);
    if (!GI) return nullptr;
    return GI->GetSubsystem<USaveManager>();
}

void USaveManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Log, TEXT("[SaveManager] Initialize"));
}

void USaveManager::Deinitialize()
{
    Saveables.Empty();
    CurrentSaveData = nullptr;
    Super::Deinitialize();
}

void USaveManager::Register(TScriptInterface<ISaveableComponent> Saveable)
{
    if (!Saveable.GetObject())
    {
        UE_LOG(LogTemp, Warning, TEXT("[SaveManager] Register: null Saveable 무시"));
        return;
    }

    // 중복 등록 방지
    for (const TScriptInterface<ISaveableComponent>& Existing : Saveables)
    {
        if (Existing.GetObject() == Saveable.GetObject())
        {
            return;
        }
    }

    Saveables.Add(Saveable);
    UE_LOG(LogTemp, Log, TEXT("[SaveManager] Registered: %s"), *Saveable->GetSaveableId().ToString());
}

void USaveManager::Unregister(TScriptInterface<ISaveableComponent> Saveable)
{
    if (!Saveable.GetObject()) return;

    const int32 Removed = Saveables.RemoveAll([&Saveable](const TScriptInterface<ISaveableComponent>& Entry)
    {
        return Entry.GetObject() == Saveable.GetObject();
    });

    if (Removed > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[SaveManager] Unregistered: %s"), *Saveable->GetSaveableId().ToString());
    }
}

bool USaveManager::HasSaveSlot() const
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

void USaveManager::RequestSave(ESaveTrigger Trigger, FSimpleDelegate OnComplete)
{
    if (bSaveInFlight)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SaveManager] RequestSave: 이미 저장 중 — 요청 무시 (Trigger=%d)"), static_cast<int32>(Trigger));
        return;
    }

    USaveAndLoadGame* SaveObj = Cast<USaveAndLoadGame>(
        UGameplayStatics::CreateSaveGameObject(USaveAndLoadGame::StaticClass()));

    if (!SaveObj)
    {
        UE_LOG(LogTemp, Error, TEXT("[SaveManager] RequestSave: SaveGameObject 생성 실패"));
        OnSaveCompleted.Broadcast(false);
        return;
    }

    // 등록된 Saveable들이 각자 담당 영역을 SaveObj에 채운다
    for (const TScriptInterface<ISaveableComponent>& Saveable : Saveables)
    {
        if (Saveable.GetObject() && IsValid(Saveable.GetObject()))
        {
            Saveable->SaveTo(SaveObj);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SaveManager] RequestSave: 유효하지 않은 Saveable 건너뜀"));
        }
    }

    bSaveInFlight = true;
    PendingTrigger = Trigger;
    PendingOnComplete = OnComplete;

    FAsyncSaveGameToSlotDelegate SaveDelegate;
    SaveDelegate.BindUObject(this, &USaveManager::HandleAsyncSaveDone);
    UGameplayStatics::AsyncSaveGameToSlot(SaveObj, SlotName, UserIndex, SaveDelegate);
}

void USaveManager::HandleAsyncSaveDone(const FString& Slot, const int32 UserIdx, bool bOk)
{
    bSaveInFlight = false;

    if (bOk)
    {
        UE_LOG(LogTemp, Log, TEXT("[SaveManager] 저장 완료 — 슬롯: %s"), *Slot);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[SaveManager] 저장 실패 — 슬롯: %s"), *Slot);
    }

    OnSaveCompleted.Broadcast(bOk);

    // OnComplete 델리게이트 먼저 호출
    if (PendingOnComplete.IsBound())
    {
        PendingOnComplete.Execute();
        PendingOnComplete.Unbind();
    }

    // QuitGame 트리거: IO 완료 후 정상 종료 (UE 정리 흐름 보장)
    if (PendingTrigger == ESaveTrigger::QuitGame)
    {
        UKismetSystemLibrary::QuitGame(GetGameInstance(), nullptr, EQuitPreference::Quit, false);
    }
}

void USaveManager::RequestLoad(FSimpleDelegate OnApplied)
{
    bIsDataLoaded = false;
    PendingLoadDelegate = OnApplied;

    FAsyncLoadGameFromSlotDelegate LoadDelegate;
    LoadDelegate.BindUObject(this, &USaveManager::HandleAsyncLoadDone);
    UGameplayStatics::AsyncLoadGameFromSlot(SlotName, UserIndex, LoadDelegate);
}

void USaveManager::NotifyMapLoaded()
{
    bIsMapLoaded = true;
    TryApplyLoadedData();
}

void USaveManager::HandleAsyncLoadDone(const FString& Slot, const int32 UserIdx, USaveGame* Loaded)
{
    CurrentSaveData = Cast<USaveAndLoadGame>(Loaded);
    if (!CurrentSaveData)
    {
        UE_LOG(LogTemp, Error, TEXT("[SaveManager] 로드 실패 또는 슬롯 없음 — 슬롯: %s"), *Slot);
        // 데이터 없이도 맵 완료 조건은 충족시켜 로딩이 멈추지 않도록 한다
        bIsDataLoaded = true;
        TryApplyLoadedData();
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SaveManager] 로드 완료 — 슬롯: %s"), *Slot);
    bIsDataLoaded = true;
    TryApplyLoadedData();
}

void USaveManager::TryApplyLoadedData()
{
    if (!bIsDataLoaded || !bIsMapLoaded) return;

    // 플래그 즉시 리셋 (재진입 방지)
    bIsDataLoaded = false;
    bIsMapLoaded = false;

    if (!CurrentSaveData)
    {
        // 새 게임 또는 로드 실패: 로드 적용 없이 완료 알림
        UE_LOG(LogTemp, Log, TEXT("[SaveManager] TryApplyLoadedData: CurrentSaveData 없음 — Saveable 적용 건너뜀"));
        OnLoadApplied.Broadcast(false);

        if (PendingLoadDelegate.IsBound())
        {
            PendingLoadDelegate.Execute();
            PendingLoadDelegate.Unbind();
        }
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[SaveManager] TryApplyLoadedData: %d개 Saveable에 LoadFrom 적용"), Saveables.Num());

    // 각 Saveable에 독립적으로 LoadFrom 호출 — 한 컴포넌트 실패가 다른 컴포넌트를 막지 않는다
    for (const TScriptInterface<ISaveableComponent>& Saveable : Saveables)
    {
        if (Saveable.GetObject() && IsValid(Saveable.GetObject()))
        {
            Saveable->LoadFrom(CurrentSaveData);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SaveManager] TryApplyLoadedData: 유효하지 않은 Saveable 건너뜀"));
        }
    }

    OnLoadApplied.Broadcast(true);

    if (PendingLoadDelegate.IsBound())
    {
        PendingLoadDelegate.Execute();
        PendingLoadDelegate.Unbind();
    }
}
