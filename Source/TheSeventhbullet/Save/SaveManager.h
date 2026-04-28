#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Save/SaveTriggerTypes.h"
#include "Save/ISaveableComponent.h"
#include "SaveManager.generated.h"

class USaveAndLoadGame;
class USaveGame;

/**
 * 저장/로드 단일 진입점 GameInstanceSubsystem.
 *
 * 모든 저장 요청은 RequestSave, 로드 요청은 RequestLoad를 통해서만 진행한다.
 * 각 Saveable 객체는 BeginPlay에서 Register, EndPlay에서 Unregister를 호출한다.
 *
 * 흐름:
 *   RequestLoad() + NotifyMapLoaded() → 양쪽 완료 시 TryApplyLoadedData() → 각 Saveable에 LoadFrom 호출
 *   RequestSave() → 각 Saveable에 SaveTo 호출 → AsyncSaveGameToSlot
 *
 * 사용 예시:
 *   USaveManager* SM = USaveManager::Get(this);
 *   SM->RequestSave(ESaveTrigger::Sleep);
 *   SM->RequestLoad(FSimpleDelegate::CreateLambda([](){ ... }));
 */
UCLASS()
class THESEVENTHBULLET_API USaveManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** WorldContext로부터 SaveManager를 가져오는 편의 메서드. */
    static USaveManager* Get(const UObject* WorldContext);

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * 저장 요청.
     * @param Trigger   저장 원인 (ESaveTrigger::QuitGame이면 완료 후 RequestExit 호출)
     * @param OnComplete 저장 완료 후 호출될 델리게이트 (선택)
     */
    void RequestSave(ESaveTrigger Trigger, FSimpleDelegate OnComplete = {});

    /**
     * 로드 요청.
     * 맵 로드(NotifyMapLoaded)와 데이터 로드가 모두 완료되면 Saveable들에 LoadFrom을 적용한다.
     * @param OnApplied 로드 적용 완료 후 호출될 델리게이트 (선택)
     */
    void RequestLoad(FSimpleDelegate OnApplied = {});

    /** 맵 스트리밍 완료 시 GameInstance가 호출. TryApplyLoadedData 조건 중 하나. */
    void NotifyMapLoaded();

    /** ISaveableComponent 구현체를 등록한다. BeginPlay에서 호출. */
    void Register(TScriptInterface<ISaveableComponent> Saveable);

    /** ISaveableComponent 구현체를 등록 해제한다. EndPlay에서 호출. */
    void Unregister(TScriptInterface<ISaveableComponent> Saveable);

    /** 현재 슬롯에 세이브 파일이 존재하는지 확인. */
    bool HasSaveSlot() const;

    DECLARE_MULTICAST_DELEGATE_OneParam(FOnSaveCompleted, bool /*bSuccess*/);
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnLoadApplied, bool /*bSuccess*/);

    /** 저장 완료 시 브로드캐스트 (성공/실패 여부 포함). */
    FOnSaveCompleted OnSaveCompleted;

    /** 로드 적용 완료 시 브로드캐스트 (성공/실패 여부 포함). */
    FOnLoadApplied OnLoadApplied;

private:
    /** 현재 세이브 데이터 객체. GC 수집 방지를 위해 UPROPERTY 필수. */
    UPROPERTY()
    TObjectPtr<USaveAndLoadGame> CurrentSaveData;

    /** 등록된 Saveable 목록. GC 안전하게 TScriptInterface 배열로 보유. */
    UPROPERTY()
    TArray<TScriptInterface<ISaveableComponent>> Saveables;

    static const FString SlotName;
    int32 UserIndex = 0;

    bool bSaveInFlight = false;
    bool bIsDataLoaded = false;
    bool bIsMapLoaded = false;

    FSimpleDelegate PendingLoadDelegate;
    ESaveTrigger PendingTrigger = ESaveTrigger::Sleep;
    FSimpleDelegate PendingOnComplete;

    void HandleAsyncSaveDone(const FString& Slot, const int32 UserIdx, bool bOk);
    void HandleAsyncLoadDone(const FString& Slot, const int32 UserIdx, USaveGame* Loaded);

    /** bIsDataLoaded && bIsMapLoaded 조건에서 Saveable들에 LoadFrom 적용. */
    void TryApplyLoadedData();
};
