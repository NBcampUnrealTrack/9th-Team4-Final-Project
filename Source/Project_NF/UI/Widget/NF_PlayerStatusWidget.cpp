#include "UI/Widget/NF_PlayerStatusWidget.h"

#include "Components/ProgressBar.h"

void UNF_PlayerStatusWidget::SetHealthPercent(float NewPercent)
{
	const float ClampedPercent = FMath::Clamp(NewPercent, 0.0f, 1.0f);

	// 최초 값은 연출 없이 바로 표시
	if (!bHasInitialHealth)
	{
		bHasInitialHealth = true;
		TargetPercent = ClampedPercent;
		TrailPercent = ClampedPercent;
		TrailDelayRemaining = 0.0f;

		if (PB_Health)
		{
			PB_Health->SetPercent(TargetPercent);
		}
		if (PB_HealthTrail)
		{
			PB_HealthTrail->SetPercent(TrailPercent);
		}
		return;
	}

	const float PreviousPercent = TargetPercent;
	TargetPercent = ClampedPercent;

	if (PB_Health)
	{
		PB_Health->SetPercent(TargetPercent);
	}

	// Health와 MaxHealth 알림으로 같은 비율이 재전달될 수 있음
	if (FMath::IsNearlyEqual(PreviousPercent, TargetPercent)) return;
	

	if (TargetPercent < PreviousPercent)
	{
		// 연속 피격 시에도 기존 잔상 유지
		TrailPercent = FMath::Max(TrailPercent, PreviousPercent);
		TrailDelayRemaining = TrailDelay;
		BP_PlayDamageFeedback();
	}
	else
	{
		// 회복 시 잔상도 즉시 새 체력으로 맞춤
		TrailPercent = TargetPercent;
		TrailDelayRemaining = 0.0f;
		BP_PlayHealFeedback();
	}

	if (PB_HealthTrail)
	{
		PB_HealthTrail->SetPercent(TrailPercent);
	}
}

void UNF_PlayerStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bHasInitialHealth || !PB_HealthTrail) return;
	
	if (FMath::IsNearlyEqual(TrailPercent, TargetPercent)) return;

	// Tick은 데이터 조회 없이 잔상 애니메이션에만 사용
	float AnimationDelta = InDeltaTime;
	if (TrailDelayRemaining > 0.0f)
	{
		const float ConsumedDelay = FMath::Min(TrailDelayRemaining, AnimationDelta);
		TrailDelayRemaining -= ConsumedDelay;
		AnimationDelta -= ConsumedDelay;
	}
	
	if (AnimationDelta <= 0.0f) return;

	TrailPercent = FMath::FInterpConstantTo(TrailPercent, TargetPercent, AnimationDelta, TrailSpeed);
	PB_HealthTrail->SetPercent(TrailPercent);
}

void UNF_PlayerStatusWidget::NativeDestruct()
{
	bHasInitialHealth = false;
	TargetPercent = 0.0f;
	TrailPercent = 0.0f;
	TrailDelayRemaining = 0.0f;

	Super::NativeDestruct();
}
