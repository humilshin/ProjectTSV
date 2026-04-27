#include "StatusComponent.h"

#include "EquipmentComponent.h"
#include "Character/MainCharacter.h"
#include "Data/SaveAndLoadGame.h"
#include "Save/SaveManager.h"

UStatusComponent::UStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CharacterStatus.Speed = 600.0f;
	CharacterStatus.HP = 100;
	CharacterStatus.Stamina = 100;
	CharacterStatus.Attack = 10;
	CharacterStatus.Defence = 0;
	CharacterStatus.CriticalChance = 0.15f;
	CharacterStatus.CriticalDamage = 1.5f;
}

void UStatusComponent::UpdateTotalStat()
{	
	UE_LOG(LogTemp, Warning, TEXT("Total Stat Updated"));
	
	FCharacterStat FinalStat;
	const FCharacterStat EnhanceStat = EnhanceStatToCharacterStat();
	FinalStat = EnhanceStat + CharacterStatus;
	
	UEquipmentComponent* GemComponent = GetOwner()->GetComponentByClass<UEquipmentComponent>();
	
	if (!GemComponent) return;
	FinalStat += GemComponent->GetTotalGemStats();
	
	AMainCharacter* Character = Cast<AMainCharacter>(GetOwner());
	if (!Character) return;
	
	Character->SetTotalStatus(FinalStat);
}

void UStatusComponent::BeginPlay()
{
	Super::BeginPlay();
	if (USaveManager* SM = USaveManager::Get(this))
	{
		SM->Register(TScriptInterface<ISaveableComponent>(this));
	}
}

void UStatusComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USaveManager* SM = USaveManager::Get(this))
	{
		SM->Unregister(TScriptInterface<ISaveableComponent>(this));
	}
	Super::EndPlay(EndPlayReason);
}

void UStatusComponent::SaveTo(USaveAndLoadGame* SaveData) const
{
	if (!SaveData) return;
	SaveData->CharacterBaseStat = GetCharacterBaseStatus();
	SaveData->CharacterEnhanceStat = GetCharacterEnhanceStatus();
}

void UStatusComponent::LoadFrom(const USaveAndLoadGame* SaveData)
{
	if (!SaveData) return;
	FCharacterStat StatCopy = SaveData->CharacterBaseStat;
	FEnhancerStatus EnhanceCopy = SaveData->CharacterEnhanceStat;
	LoadData(StatCopy, EnhanceCopy);
}

FCharacterStat UStatusComponent::EnhanceStatToCharacterStat()
{
	FCharacterStat FinalStat;
	
	FinalStat.HP = CharacterEnhanceStatus.EnhanceHp * FEnhancerIncreaseStatus::IncreaseHp;
	FinalStat.Defence = CharacterEnhanceStatus.EnhanceDefense * FEnhancerIncreaseStatus::IncreaseDefense;
	FinalStat.Attack = CharacterEnhanceStatus.EnhanceAttack * FEnhancerIncreaseStatus::IncreaseAttack;
	FinalStat.Stamina = CharacterEnhanceStatus.EnhanceStamina * FEnhancerIncreaseStatus::IncreaseStamina;
	
	return FinalStat;
}

void UStatusComponent::LoadData(FCharacterStat& LoadCharacterStatus, FEnhancerStatus& LoadEnhancerStatus)
{
	CharacterStatus = LoadCharacterStatus;
	CharacterEnhanceStatus = LoadEnhancerStatus;
}

FCharacterStat UStatusComponent::GetCharacterBaseStatus() const
{
	return CharacterStatus;
}

void UStatusComponent::SetCharacterStatus(FCharacterStat& Status)
{
	this->CharacterStatus = Status;
}

void UStatusComponent::SetCharacterEnhanceStatus(FEnhancerStatus& Status)
{
	this->CharacterEnhanceStatus = Status;
}

FEnhancerStatus UStatusComponent::GetCharacterEnhanceStatus() const
{
	return CharacterEnhanceStatus;
}


