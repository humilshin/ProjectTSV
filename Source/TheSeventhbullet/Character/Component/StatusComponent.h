#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/StatusTypes.h"
#include "Save/ISaveableComponent.h"
#include "StatusComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THESEVENTHBULLET_API UStatusComponent : public UActorComponent, public ISaveableComponent
{
	GENERATED_BODY()

public:
	UStatusComponent();
	
	UFUNCTION()
	void UpdateTotalStat();
	
protected:
	virtual void BeginPlay() override;
	
private:
	FCharacterStat EnhanceStatToCharacterStat();
private:
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category ="Character", meta = (AllowPrivateAccess = "true"))
	FCharacterStat CharacterStatus; // 캐릭터 기본 스탯
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category ="Character", meta = (AllowPrivateAccess = "true"))
	FEnhancerStatus CharacterEnhanceStatus; // 캐릭터 상점 구입한 스탯 레벨 정보
public:
	UFUNCTION()
	void LoadData(FCharacterStat& LoadCharacterStatus, FEnhancerStatus& LoadEnhancerStatus);
	UFUNCTION()
	FCharacterStat GetCharacterBaseStatus() const;
	void SetCharacterStatus(FCharacterStat& Status);
	void SetCharacterEnhanceStatus(FEnhancerStatus& Status);
	FEnhancerStatus GetCharacterEnhanceStatus() const;

	void AddHpStatus(float amount);

	// ISaveableComponent
	virtual void SaveTo(USaveAndLoadGame* SaveData) const override;
	virtual void LoadFrom(const USaveAndLoadGame* SaveData) override;
	virtual FName GetSaveableId() const override { return FName("Status"); }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
