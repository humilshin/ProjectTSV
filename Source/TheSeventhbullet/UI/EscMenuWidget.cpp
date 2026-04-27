#include "EscMenuWidget.h"
#include "UITags.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Manager/UIManager.h"
#include "Save/SaveManager.h"
#include "Save/SaveTriggerTypes.h"
#include "System/GameInstance/MainGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UEscMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UEscMenuWidget::OnResumeClicked);
		SetupButtonHover(ResumeButton);
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UEscMenuWidget::OnSettingsClicked);
		SetupButtonHover(SettingsButton);
	}

	if (BackToMenuButton)
	{
		BackToMenuButton->OnClicked.AddDynamic(this, &UEscMenuWidget::OnBackToMenuClicked);
		SetupButtonHover(BackToMenuButton);
	}

	if (QuitGameButton)
	{
		QuitGameButton->OnClicked.AddDynamic(this, &UEscMenuWidget::OnQuitGameClicked);
		SetupButtonHover(QuitGameButton);
	}
}

// --- Button Hover ---

void UEscMenuWidget::SetupButtonHover(UButton* Button)
{
	if (!Button) return;

	UTextBlock* TextBlock = FindChildTextBlock(Button);
	if (!TextBlock) return;

	ButtonTextMap.Add(Button, TextBlock);
	TextBlock->SetColorAndOpacity(NormalTextColor);

	Button->OnHovered.AddDynamic(this, &UEscMenuWidget::OnAnyButtonHovered);
	Button->OnUnhovered.AddDynamic(this, &UEscMenuWidget::OnAnyButtonUnhovered);
}

UTextBlock* UEscMenuWidget::FindChildTextBlock(UWidget* Parent)
{
	UPanelWidget* Panel = Cast<UPanelWidget>(Parent);
	if (!Panel) return nullptr;

	for (int32 i = 0; i < Panel->GetChildrenCount(); i++)
	{
		UWidget* Child = Panel->GetChildAt(i);
		if (UTextBlock* Text = Cast<UTextBlock>(Child))
		{
			return Text;
		}
		if (UTextBlock* Found = FindChildTextBlock(Child))
		{
			return Found;
		}
	}
	return nullptr;
}

void UEscMenuWidget::OnAnyButtonHovered()
{
	for (auto& Pair : ButtonTextMap)
	{
		if (Pair.Key && Pair.Key->IsHovered() && Pair.Value)
		{
			Pair.Value->SetColorAndOpacity(HoveredTextColor);
		}
	}
}

void UEscMenuWidget::OnAnyButtonUnhovered()
{
	for (auto& Pair : ButtonTextMap)
	{
		if (Pair.Key && !Pair.Key->IsHovered() && Pair.Value)
		{
			Pair.Value->SetColorAndOpacity(NormalTextColor);
		}
	}
}

// --- Button Actions ---

void UEscMenuWidget::OnResumeClicked()
{
	if (UUIManager* UIMgr = UUIManager::Get(this))
	{
		UIMgr->Toggle(UITags::EscMenu);
	}
}

void UEscMenuWidget::OnSettingsClicked()
{
	if (UUIManager* UIMgr = UUIManager::Get(this))
	{
		UIMgr->Open(UITags::Option);
	}
}

void UEscMenuWidget::OnBackToMenuClicked()
{
	if (UUIManager* UIMgr = UUIManager::Get(this))
	{
		UIMgr->Close(UITags::EscMenu);
	}

	UMainGameInstance* GI = UMainGameInstance::Get(this);
	if (GI)
	{
		GI->ReturnToMainMenu();
	}
}

void UEscMenuWidget::OnQuitGameClicked()
{
	// QuitGame 트리거: 저장 완료 콜백 내에서 FPlatformMisc::RequestExit 호출 (IO 완료 보장)
	if (USaveManager* SM = USaveManager::Get(this))
	{
		SM->RequestSave(ESaveTrigger::QuitGame);
		// RequestExit는 SaveManager::HandleAsyncSaveDone에서 처리 — 별도 QuitGame 호출 불필요
	}
	else
	{
		// SaveManager 없을 경우 폴백
		UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, true);
	}
}