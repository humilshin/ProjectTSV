#include "ChestActor.h"
#include "Inventory/InventoryComponent.h"
#include "Character/MainCharacter.h"
#include "Data/SaveAndLoadGame.h"
#include "Manager/UIManager.h"
#include "Save/SaveManager.h"
#include "UI/StorageWidget.h"
#include "UI/UITags.h"
#include "UI/WeaponSelectWidget.h"

AChestActor::AChestActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InventoryComp = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	// 부모의 StaticMesh를 창고 메시로 활용
	// PromptText를 창고용으로 변경
	PromptText = FText::FromString(TEXT("F키를 눌러 상자 열기"));
}

void AChestActor::BeginPlay()
{
	Super::BeginPlay();
	if (USaveManager* SM = USaveManager::Get(this))
	{
		SM->Register(TScriptInterface<ISaveableComponent>(this));
	}
}

void AChestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USaveManager* SM = USaveManager::Get(this))
	{
		SM->Unregister(TScriptInterface<ISaveableComponent>(this));
	}
	Super::EndPlay(EndPlayReason);
}

void AChestActor::SaveTo(USaveAndLoadGame* SaveData) const
{
	if (!SaveData || !InventoryComp) return;
	SaveData->ChestInventoryItems = InventoryComp->GetAllItems();
}

void AChestActor::LoadFrom(const USaveAndLoadGame* SaveData)
{
	if (!SaveData || !InventoryComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ChestActor::LoadFrom] SaveData 또는 InventoryComp 없음 — 건너뜀"));
		return;
	}
	TArray<FItemInstance> ItemsCopy = SaveData->ChestInventoryItems;
	InventoryComp->LoadData(ItemsCopy);
}

void AChestActor::Interact(AActor* Interactor)
{
	AMainCharacter* Player = Cast<AMainCharacter>(Interactor);
	if (!Player)
	{
		return;
	}

	UUIManager* UIMgr = UUIManager::Get(this);
	if (!UIMgr)
	{
		return;
	}

	UUserWidget* Widget = UIMgr->Open(UITags::Storage);
	UStorageWidget* StorageWidget = Cast<UStorageWidget>(Widget);
	if (StorageWidget)
	{
		StorageWidget->OpenStorage(InventoryComp, Player->InventoryComponent,Player);
	}
	
}
