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
	PendingDamage = Damage;
	bPendingIsCrit = bIsCrit;

	if (!DamageWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[DamageNumberActor] DamageWidgetClass가 블루프린트에서 설정되지 않았습니다. (Owner: %s)"), *GetName());
		return;
	}

	WidgetComp->SetWidgetClass(DamageWidgetClass);
	WidgetComp->InitWidget();

	TryApplyWidget();

	if (bIsCrit)
	{
		SetActorScale3D(FVector(1.5f));
	}

	SetLifeSpan(LifeDuration);
}

void ADamageNumberActor::TryApplyWidget()
{
	UDamageNumberWidget* Widget = Cast<UDamageNumberWidget>(WidgetComp->GetWidget());
	if (Widget)
	{
		CachedWidget = Widget;
		bWidgetReady = true;
		Widget->SetDamageInfo(PendingDamage, bPendingIsCrit);
		return;
	}

	// PlayerController 미연결 등으로 InitWidget 직후 위젯이 null인 경우 1프레임 지연 재시도
	TWeakObjectPtr<ADamageNumberActor> WeakThis(this);
	GetWorld()->GetTimerManager().SetTimerForNextTick([WeakThis]()
	{
		ADamageNumberActor* Self = WeakThis.Get();
		if (!Self) return;

		UDamageNumberWidget* RetryWidget = Cast<UDamageNumberWidget>(Self->WidgetComp->GetWidget());
		if (RetryWidget)
		{
			Self->CachedWidget = RetryWidget;
			Self->bWidgetReady = true;
			RetryWidget->SetDamageInfo(Self->PendingDamage, Self->bPendingIsCrit);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[DamageNumberActor] 1프레임 지연 후에도 DamageNumberWidget을 가져오지 못했습니다. Actor를 제거합니다. (Owner: %s)"), *Self->GetName());
			Self->Destroy();
		}
	});
}

void ADamageNumberActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	FVector Location = GetActorLocation();
	Location.Z += FloatSpeed * DeltaTime;
	SetActorLocation(Location);

	if (bWidgetReady && CachedWidget.IsValid())
	{
		float Alpha = FMath::Clamp(1.0f - (ElapsedTime / LifeDuration), 0.0f, 1.0f);
		CachedWidget->SetRenderOpacity(Alpha);
	}
}
