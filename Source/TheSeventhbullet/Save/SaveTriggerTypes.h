#pragma once

#include "CoreMinimal.h"
#include "SaveTriggerTypes.generated.h"

/**
 * 저장 요청의 발생 원인을 식별하는 열거형.
 * SaveManager는 트리거 종류에 따라 저장 완료 후 동작을 다르게 처리한다.
 * - QuitGame: 저장 완료 콜백에서 FPlatformMisc::RequestExit 호출 (IO 완료 보장)
 * - Sleep: 침대 상호작용 (SaveComponent)
 * - ReturnToMenu: 메인 메뉴 복귀 (MainGameInstance::ReturnToMainMenu)
 * - Auto: 향후 자동저장 훅 (현재 미사용)
 */
UENUM(BlueprintType)
enum class ESaveTrigger : uint8
{
    Sleep          UMETA(DisplayName = "Sleep"),
    QuitGame       UMETA(DisplayName = "Quit Game"),
    ReturnToMenu   UMETA(DisplayName = "Return To Menu"),
    Auto           UMETA(DisplayName = "Auto Save"),
};
