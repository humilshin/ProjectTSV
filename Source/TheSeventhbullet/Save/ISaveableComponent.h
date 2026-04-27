#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ISaveableComponent.generated.h"

class USaveAndLoadGame;

/**
 * UInterface 더미 클래스 (리플렉션 시스템 요구).
 * 구현체는 ISaveableComponent를 상속한다.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class USaveableComponent : public UInterface
{
    GENERATED_BODY()
};

/**
 * SaveManager에 등록될 수 있는 객체가 구현해야 하는 인터페이스.
 *
 * 구현 규칙:
 *   - SaveTo: USaveAndLoadGame 내 자기 담당 영역만 채운다. throw 금지.
 *   - LoadFrom: 실패해도 로그만 출력하고 return. 다른 컴포넌트 로드를 막지 말 것.
 *   - GetSaveableId: 디버그/순서 제어용 고유 식별자. (예: "Equipment", "Inventory")
 *
 * 등록 패턴:
 *   BeginPlay()  → SaveManager->Register(this)
 *   EndPlay()    → SaveManager->Unregister(this)
 */
class THESEVENTHBULLET_API ISaveableComponent
{
    GENERATED_BODY()

public:
    /** SaveManager가 저장 시 호출. SaveData의 자기 영역만 채운다. */
    virtual void SaveTo(USaveAndLoadGame* SaveData) const = 0;

    /** SaveManager가 로드 적용 시 호출. 실패해도 throw 금지. */
    virtual void LoadFrom(const USaveAndLoadGame* SaveData) = 0;

    /** 디버그 및 순서 제어용 식별자. */
    virtual FName GetSaveableId() const = 0;
};
