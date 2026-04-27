#include "DamageNumberActor.h"
#include "DamageNumberWidget.h"
#include "Components/WidgetComponent.h"

ADamageNumberActor::ADamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;

	WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
	SetRootComponent(WidgetComp);

	WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	WidgetComp->SetDrawAtDesiredSize(true);
	WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADamageNumberActor::Init(float Damage, bool bIsCrit)
{
	// [T0.4] DamageWidgetClass 미설정 또는 위젯 생성 실패 시 명시적 경고 로그
	if (!DamageWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DamageNumberActor] DamageWidgetClass가 블루프린트에서 설정되지 않았습니다 — 데미지 숫자 위젯을 생성할 수 없습니다."));
	}
	else
	{
		WidgetComp->SetWidgetClass(DamageWidgetClass);
		WidgetComp->InitWidget();
	}

	UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(WidgetComp->GetWidget());
	if (DamageWidget)
	{
		DamageWidget->SetDamageInfo(Damage, bIsCrit);
	}
	else
	{
		// [T0.4] InitWidget() 이후 위젯이 null인 경우 — PlayerController 미연결 또는 클래스 불일치 가능
		UE_LOG(LogTemp, Warning, TEXT("[DamageNumberActor] WidgetComponent에서 DamageNumberWidget을 가져오지 못했습니다 — 데미지 숫자가 표시되지 않습니다. (InitWidget 타이밍 문제 의심, Phase 2에서 1프레임 지연 재시도로 해소 예정)"));
	}

	if (bIsCrit)
	{
		SetActorScale3D(FVector(1.5f));
	}

	SetLifeSpan(LifeDuration);
}

void ADamageNumberActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	// Float upward
	FVector Location = GetActorLocation();
	Location.Z += FloatSpeed * DeltaTime;
	SetActorLocation(Location);

	// Fade out
	float Alpha = FMath::Clamp(1.0f - (ElapsedTime / LifeDuration), 0.0f, 1.0f);
	if (UDamageNumberWidget* DamageWidget = Cast<UDamageNumberWidget>(WidgetComp->GetWidget()))
	{
		DamageWidget->SetRenderOpacity(Alpha);
	}
}
