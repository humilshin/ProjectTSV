#pragma once

#include "CoreMinimal.h"
#include "BaseInteractionComponent.h"
#include "UI/SaveWidget.h"
#include "SaveComponent.generated.h"

class UMainGameInstance;
class USaveManager;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THESEVENTHBULLET_API USaveComponent : public UBaseInteractionComponent
{
	GENERATED_BODY()

public:
	USaveComponent();
	virtual void BeginInteract(AActor* Interactor) override;
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Bed|UI")
	TSubclassOf<USaveWidget> SaveWidgetClass;
	
private:
	UPROPERTY()
	TObjectPtr<USaveWidget> SaveWidget;
	
	bool bIsTransitioning = false;
	
	UFUNCTION()
	void OnSleepAnimComplete();
	
	UFUNCTION()
	void OnWakeAnimComplete();
	
	void HandleNextDay();
	void SetPlayerInputEnabled(AActor* Interactor, bool bEnabled);

	
private:
	UPROPERTY()
	TObjectPtr<UMainGameInstance> GI = nullptr;

	UPROPERTY()
	TObjectPtr<USaveManager> SaveManager = nullptr;
};
