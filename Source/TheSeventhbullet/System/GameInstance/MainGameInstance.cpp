
#include "MainGameInstance.h"

#include "LevelSequencePlayer.h"
#include "Character/MainCharacter.h"
#include "Character/Component/EquipmentComponent.h"
#include "Character/Component/StatusComponent.h"
#include "Data/SaveAndLoadGame.h"
#include "Inventory/InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/SoundManager.h"
#include "Manager/UIManager.h"
#include "Save/SaveManager.h"
#include "System/MonsterManagerSubSystem.h"
#include "UI/UITags.h"
#include "UI/LoadingScreenWidget.h"
#include "TheSeventhbullet/System/MainGameMode.h"

const FName UMainGameInstance::StartLevelName = FName(TEXT("L_Town"));

UMainGameInstance* UMainGameInstance::Get(const UObject* WorldContext)
{
	return Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
}

void UMainGameInstance::OnStart()
{
	Super::OnStart();
	// GameInstanceSubsystem이 초기화된 후 호출되므로 Register 안전
	RegisterToSaveManager();
}

// ISaveableComponent 구현 — GameInstance 담당 영역(통계/날짜)만 처리

void UMainGameInstance::SaveTo(USaveAndLoadGame* SaveData) const
{
	if (!SaveData) return;
	SaveData->CurrentDay = CurrentDay;
	SaveData->TotalRequestAttack = TotalRequestAttack;
	SaveData->TotalRequestHit = TotalRequestHit;
}

void UMainGameInstance::LoadFrom(const USaveAndLoadGame* SaveData)
{
	if (!SaveData) return;
	CurrentDay = SaveData->CurrentDay;
	TotalRequestAttack = SaveData->TotalRequestAttack;
	TotalRequestHit = SaveData->TotalRequestHit;
}


void UMainGameInstance::StartNewGame()
{
	// 새 게임: 데이터 로드 없이 맵만 스트리밍. SaveManager에 폴링 플래그만 설정.
	bPollDataReady = true;
	GameStartMapLoad();
}

void UMainGameInstance::ReturnToMainMenu()
{
	if (USaveManager* SM = GetSubsystem<USaveManager>())
	{
		SM->RequestSave(ESaveTrigger::ReturnToMenu, FSimpleDelegate::CreateLambda([this]()
		{
			UGameplayStatics::SetGamePaused(GetWorld(), false);

			AMainGameMode* GM = AMainGameMode::Get(this);
			if (GM)
			{
				GM->ReturnToMainMenu();
			}
		}));
	}
	else
	{
		// SaveManager 없을 경우 폴백 (에디터 등 예외 상황)
		UGameplayStatics::SetGamePaused(GetWorld(), false);

		AMainGameMode* GM = AMainGameMode::Get(this);
		if (GM)
		{
			GM->ReturnToMainMenu();
		}
	}
}

void UMainGameInstance::GameStartMapLoad()
{
	if (UUIManager* UIMgr = UUIManager::Get(this))
	{
		UIMgr->Close(UITags::MainMenu);
	}

	// SaveManager의 로드 완료 이벤트를 구독하여 로딩 진행률 폴링에 반영
	if (USaveManager* SM = GetSubsystem<USaveManager>())
	{
		SM->OnLoadApplied.AddUObject(this, &UMainGameInstance::NotifySaveManagerLoadApplied);
	}

	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = FName("OnMapLoadFinished");
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = GetUniqueID();

	ShowLoadingScreen();

	UGameplayStatics::LoadStreamLevel(this, StartLevelName, true, false, LatentInfo);
}

void UMainGameInstance::RegisterToSaveManager()
{
	if (USaveManager* SM = GetSubsystem<USaveManager>())
	{
		SM->Register(TScriptInterface<ISaveableComponent>(this));
	}
}

void UMainGameInstance::NotifySaveManagerLoadApplied(bool bSuccess)
{
	bPollDataReady = true;
}

void UMainGameInstance::OnMapLoadFinished()
{
	bPollMapReady = true;

	// SaveManager에 맵 로드 완료 알림 → TryApplyLoadedData 조건 충족 시 Saveable LoadFrom 적용
	if (USaveManager* SM = GetSubsystem<USaveManager>())
	{
		SM->NotifyMapLoaded();
	}
}

void UMainGameInstance::ShowLoadingScreen()
{
	DisplayProgress = 0.0f;
	TargetProgress = 0.0f;

	UUIManager* UIMgr = UUIManager::Get(this);
	if (UIMgr)
	{
		UUserWidget* Widget = UIMgr->Open(UITags::LoadingScreen);
		CachedLoadingWidget = Cast<ULoadingScreenWidget>(Widget);
		if (CachedLoadingWidget)
		{
			CachedLoadingWidget->SetProgress(0.0f);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ProgressTimerHandle, this,
			&UMainGameInstance::PollLoadingProgress, 0.03f, true
		);
	}
}

void UMainGameInstance::HideLoadingScreen()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProgressTimerHandle);
	}

	CachedLoadingWidget = nullptr;

	UUIManager* UIMgr = UUIManager::Get(this);
	if (UIMgr)
	{
		UIMgr->Close(UITags::LoadingScreen);
	}
}

bool UMainGameInstance::DoesSaveExist() const
{
	if (const USaveManager* SM = GetSubsystem<USaveManager>())
	{
		return SM->HasSaveSlot();
	}
	return false;
}

void UMainGameInstance::ResetGameData()
{
	CurrentDay = 1;
    
	AMainCharacter* Character = Cast<AMainCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!Character) return;
    
	Character->ResetGold();
	Character->AddGold(10000);
    
	UInventoryComponent* CharacterInventory = Character->GetComponentByClass<UInventoryComponent>();
	if (!CharacterInventory) return;
	CharacterInventory->ClearAllItems();
    
	UEquipmentComponent* Equipment = Character->GetComponentByClass<UEquipmentComponent>();
	if (!Equipment) return;
	Equipment->EquippedSoulGems.Empty();
	Equipment->PendingWeapon = nullptr;
	Equipment->CurrentWeapon = nullptr;
	Equipment->OnGemEquipmentChanged.Broadcast();

	// 강화 스탯 초기화
	UStatusComponent* StatusComponent = Character->GetComponentByClass<UStatusComponent>();
	if (StatusComponent)
	{
		FEnhancerStatus ResetEnhance;
		StatusComponent->SetCharacterEnhanceStatus(ResetEnhance);
		StatusComponent->UpdateTotalStat();
	}

	// 1일차 물약 1개 지급
	FPrimaryAssetId PotionID(FPrimaryAssetType("Item"), FName("DA_HealthPotion"));
	CharacterInventory->AddItem(PotionID, 1);

	// SaveManager 슬롯 이름과 동일한 슬롯 삭제
	const FString ResetSlotName = TEXT("TheSeventhBullet");
	if (UGameplayStatics::DoesSaveGameExist(ResetSlotName, 0))
	{
		bool bIsDeleted = UGameplayStatics::DeleteGameInSlot(ResetSlotName, 0);
		if (bIsDeleted)
		{
			UE_LOG(LogTemp, Log, TEXT("세이브 파일이 성공적으로 삭제되었습니다."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("세이브 파일 삭제에 실패했습니다."));
		}
	}
}

void UMainGameInstance::RequestBossStage(int32 InRequestID)
{
	PendingBossRequestID = InRequestID;

	// 보스 맵 전환 전 TownHUD 정리
	UUIManager* UIMgr = UUIManager::Get(this);
	if (UIMgr)
	{
		UIMgr->Close(UITags::TownHUD);
	}

	ShowLoadingScreen();
	
	FLatentActionInfo LatentInfo;
	LatentInfo.CallbackTarget = this;
	LatentInfo.ExecutionFunction = FName("OnBossSequenceLevelLoaded");
	LatentInfo.Linkage = 0;
	LatentInfo.UUID = GetUniqueID();
	
	UGameplayStatics::LoadStreamLevel(this, BossSequenceLevelName,true,false,LatentInfo);
	
	//현석 : 기존 타운맵 언로드
	FLatentActionInfo UnloadLatentInfo;
	UnloadLatentInfo.CallbackTarget = this;
	UnloadLatentInfo.ExecutionFunction = NAME_None;
	UnloadLatentInfo.Linkage = 1;
	UnloadLatentInfo.UUID = GetUniqueID() + 1;
    
	UGameplayStatics::UnloadStreamLevel(this, TownMapLevelName, UnloadLatentInfo, false);
	
	USoundManager* SoundMgr = USoundManager::Get(this);
	if (SoundMgr)
	{
		SoundMgr->PlayBGM(TEXT("BossBGM"), 2.0f, 0.7f);
	}
}

void UMainGameInstance::OnBossSequenceLevelLoaded()
{
	HideLoadingScreen();
	PlayBossSequence();
}

void UMainGameInstance::OnBossMapLoaded()
{
	HideLoadingScreen();
	
	UMonsterManagerSubSystem* SubSystem = UMonsterManagerSubSystem::Get(this);
	if (SubSystem)
		SubSystem->CacheSpawners();
	
	AMainGameMode* GM = AMainGameMode::Get(this);
	if (!GM) return;
	
	GM->SetTargetRequestID(PendingBossRequestID);
	GM->PrepareStageAndPreLoad();
}

void UMainGameInstance::OnBossSequenceFinishedDelegate()
{
	//현석 : 바인딩 해제
	if (BossSequencePlayer)
	{
		BossSequencePlayer->OnFinished.RemoveDynamic(
			this, &UMainGameInstance::OnBossSequenceFinishedDelegate);
	}
	OnBossSequenceFinished();
}

void UMainGameInstance::PlayBossSequence()
{
	//현석 : BossMeetSequence를 플레이
	FString SequencePath=TEXT("/Game/TheSeventhBullet/Blueprints/Enemy/Boss/Sequence/BossMeetSequence.BossMeetSequence");
	ULevelSequence* BossMeetSequence=Cast<ULevelSequence>(StaticLoadObject(ULevelSequence::StaticClass(),nullptr,*SequencePath));
	if (BossMeetSequence)
	{
		ALevelSequenceActor* OutActor;
		FMovieSceneSequencePlaybackSettings Settings;
		BossSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), BossMeetSequence, Settings, OutActor);
		if (BossSequencePlayer)
		{
			//시퀀스 재생 전 UI 정리 (보스 입장 시퀀스는 보스전 시작 전이므로 일반 HUD 정리)
			UUIManager* UIMgr = UUIManager::Get(this);
			if (UIMgr)
			{
				UIMgr->Close(UITags::HUD);
				UIMgr->Close(UITags::Crosshair);
			}

			//현석 : 시퀀스 재생 종료를 위한 콜백함수 바인딩
			BossSequencePlayer->OnFinished.AddDynamic(this,&UMainGameInstance::OnBossSequenceFinishedDelegate);
			BossSequencePlayer->Play();
		}
	}
}

void UMainGameInstance::OnBossSequenceFinished()
{
	FLatentActionInfo UnLoadInfo;
	UGameplayStatics::UnloadStreamLevel(this,BossSequenceLevelName, UnLoadInfo, false);
	
	ShowLoadingScreen();
	
	FLatentActionInfo LatentActionInfo;
	LatentActionInfo.CallbackTarget = this;
	LatentActionInfo.ExecutionFunction = FName("OnBossMapLoaded");
	LatentActionInfo.Linkage = 0;
	LatentActionInfo.UUID = GetUniqueID()+1;
	
	UGameplayStatics::LoadStreamLevel(this, BossMapLevelName,true,false,LatentActionInfo);
}


void UMainGameInstance::PollLoadingProgress()
{
	if (!CachedLoadingWidget)
	{
		return;
	}
	
	float RealTarget = 0.0f;
	if (bPollDataReady) RealTarget += 0.5f;
	if (bPollMapReady) RealTarget += 0.5f;
	if (RealTarget > TargetProgress)
	{
		TargetProgress = RealTarget;
	}
	
	const float InterpSpeed = 2.0f;
	DisplayProgress = FMath::FInterpTo(DisplayProgress, TargetProgress, 0.03f, InterpSpeed);
	
	if (FMath::IsNearlyEqual(DisplayProgress, TargetProgress, 0.005f))
	{
		DisplayProgress = TargetProgress;
	}

	CachedLoadingWidget->SetProgress(DisplayProgress);
	
	if (DisplayProgress >= 1.0f)
	{
		bPollDataReady = false;
		bPollMapReady = false;

		HideLoadingScreen();

		AMainGameMode* GM = AMainGameMode::Get(this);
		if (GM)
		{
			GM->StartGamePlay();
		}
	}
}

