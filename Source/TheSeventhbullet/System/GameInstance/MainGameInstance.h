// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Save/ISaveableComponent.h"
#include "MainGameInstance.generated.h"

class ULevelSequencePlayer;
class USaveGame;
class UUIManager;
class ULoadingScreenWidget;
class USaveAndLoadGame;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/* [MainGameInstance Guide]
 *
 * 게임 전체 생명주기를 관리하는 GameInstance 클래스
 * 엔진 시작 시 1회 생성되며 맵 전환에도 유지됨
 *
 * [역할]
 *   1. 게임 시작 흐름 관리 (NewGame / Continue)
 *   2. 로딩 화면 표시 및 진행률 보간
 *   3. 메인 메뉴 복귀 처리
 *   4. 날짜/통계 캡슐화 (ISaveableComponent 구현 → SaveManager에 위임)
 *
 * [게임 시작 흐름]
 *   MainMenu 표시 (MainGameMode::BeginPlay)
 *     └─ NewGame 클릭 → StartNewGame()
 *          └─ bPollDataReady = true (저장 로드 건너뜀) + GameStartMapLoad()
 *     └─ Continue 클릭 → SaveManager::RequestLoad() + GameStartMapLoad()
 *          └─ 데이터 로드 + 맵 스트리밍 병렬 실행
 *     └─ 둘 다 완료 → SaveManager::TryApplyLoadedData() → 보간 100% 도달 → StartGamePlay()
 *
 * [메인 메뉴 복귀]
 *   ReturnToMainMenu()
 *     └─ SaveManager::RequestSave(ReturnToMenu, 콜백) → 일시정지 해제 → GameMode에 리셋 위임
 *
 * [사용 예시]
 *   UMainGameInstance* GI = UMainGameInstance::Get(this);
 *
 *   // 새 게임 시작
 *   GI->StartNewGame();
 *
 *   // 이어하기 (MainMenuWidget에서)
 *   USaveManager::Get(this)->RequestLoad();
 *   GI->GameStartMapLoad();
 *
 *   // 메인 메뉴로 복귀
 *   GI->ReturnToMainMenu();
 *
 *   // 세이브 존재 여부 확인
 *   bool bHasSave = GI->DoesSaveExist();
 */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS()
class THESEVENTHBULLET_API UMainGameInstance : public UGameInstance, public ISaveableComponent
{
	GENERATED_BODY()
public:
	static UMainGameInstance* Get(const UObject* WorldContext);

	virtual void OnStart() override;

	void GameStartMapLoad();
	void StartNewGame();
	void ReturnToMainMenu();

	bool DoesSaveExist() const;

	void ResetGameData();

	/** 현재 날짜 접근자 */
	int32 GetCurrentDay() const { return CurrentDay; }

	/** 하루 전진 (침대 저장 시 호출) */
	void AdvanceDay() { CurrentDay++; }

	void AddAttackCount(int32 Count = 1) { TotalRequestAttack += Count; }
	void AddHitCount(int32 Count = 1) { TotalRequestHit += Count; }

	// ISaveableComponent
	virtual void SaveTo(USaveAndLoadGame* SaveData) const override;
	virtual void LoadFrom(const USaveAndLoadGame* SaveData) override;
	virtual FName GetSaveableId() const override { return FName("GameInstance"); }

	/** GameInstance를 SaveManager에 등록. OnStart()에서 호출. */
	void RegisterToSaveManager();

	/** SaveManager::OnLoadApplied 콜백 수신 → 로딩 진행률 폴링 플래그 설정 */
	void NotifySaveManagerLoadApplied(bool bSuccess);
	
	//Town에서 보스 의뢰 수락 시 호출
	//현석 : 블루프린트 테스트용으로 추가
	UFUNCTION(BlueprintCallable)
	void RequestBossStage(int32 InRequestID);
private:
	UFUNCTION()
	void OnMapLoadFinished();

	void ShowLoadingScreen();
	void HideLoadingScreen();
	void PollLoadingProgress();
	
	UFUNCTION()
	void OnBossSequenceLevelLoaded();//시퀄스 로드 완료
	UFUNCTION()
	void OnBossMapLoaded();//보스맵 로드 완료 -> 웨이브 시작
	UFUNCTION()
	void OnBossSequenceFinishedDelegate();//시퀀스 종료 -> 보스맵 로드
	
	void PlayBossSequence();//시퀀스 재생
	void OnBossSequenceFinished();//시퀀스 종료 델리게이트 수신

private:
	static const FName StartLevelName;

	/** 로딩 진행률 폴링용 플래그 (SaveManager와 독립적으로 로딩 UI만 제어) */
	bool bPollDataReady = false;
	bool bPollMapReady = false;

	UPROPERTY()
	TObjectPtr<ULoadingScreenWidget> CachedLoadingWidget;

	FTimerHandle ProgressTimerHandle;

	float DisplayProgress = 0.0f;
	float TargetProgress = 0.0f;

	/** CurrentDay, TotalRequestAttack/Hit — ISaveableComponent::SaveTo/LoadFrom으로 저장/로드 */
	int32 CurrentDay = 1;
	int32 TotalRequestAttack = 0;
	int32 TotalRequestHit = 0;
	
	int32 PendingBossRequestID = INDEX_NONE;
	
	//현석 : SequencePlayer 캐싱 및 GC에서 메모리 해제 방지
	UPROPERTY()
	TObjectPtr<ULevelSequencePlayer> BossSequencePlayer;
	
	FName BossSequenceLevelName = FName(TEXT("L_BossSequence"));
	FName BossMapLevelName = FName(TEXT("L_Boss"));
	
	//현석 : 기존 맵 언로드를 위해 추가
	FName TownMapLevelName=FName(TEXT("L_Town"));
};
