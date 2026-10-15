#include "GamePlay/Combat/Components/NF_RepeatApplicationComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UNF_RepeatApplicationComponent::UNF_RepeatApplicationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNF_RepeatApplicationComponent::BeginPlay()
{
	Super::BeginPlay();
	Deployment = GetOwner()->FindComponentByClass<UNF_DeployableComponent>();
	if (Deployment.IsValid()) { Deployment->OnDeploymentEnded.AddDynamic(this, &UNF_RepeatApplicationComponent::HandleDeploymentEnded); }
}

bool UNF_RepeatApplicationComponent::StartRepeating()
{
	if (bRepeating || bEndingPlay || !HasBegunPlay() || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed()
		|| !GetOwner()->HasAuthority() || !GetWorld()
		|| !FMath::IsFinite(Interval) || Interval < 0.01f || !FMath::IsFinite(Duration) || Duration < 0.0f
		|| MaxApplications < 0 || (Deployment.IsValid() && !Deployment->IsDeploymentActive())) { return false; }
	bRepeating = true;
	++Generation;
	ApplicationCount = 0;
	ActiveInterval = Interval;
	ActiveMaxApplications = MaxApplications;
	EndTime = Duration > 0.0f ? GetWorld()->GetTimeSeconds() + Duration : 0.0;
	if (Duration > 0.0f) { GetWorld()->GetTimerManager().SetTimer(DurationTimer, this, &UNF_RepeatApplicationComponent::ExpireRepeating, Duration, false); }
	if (bApplyImmediately) { ApplyOnce(); }
	else { ScheduleNext(); }
	return true;
}

bool UNF_RepeatApplicationComponent::HasValidSettings() const
{
	return FMath::IsFinite(Interval) && Interval >= 0.01f && FMath::IsFinite(Duration) && Duration >= 0.0f && MaxApplications >= 0;
}

void UNF_RepeatApplicationComponent::ScheduleNext()
{
	if (bRepeating && !bEndingPlay && GetWorld())
	{
		// 단발 타이머를 다시 예약하여 프레임 지연 때 피해가 한꺼번에 몰리지 않게 한다.
		GetWorld()->GetTimerManager().SetTimer(ApplicationTimer, this, &UNF_RepeatApplicationComponent::ApplyOnce, ActiveInterval, false);
	}
}

void UNF_RepeatApplicationComponent::ApplyOnce()
{
	if (!bRepeating || bEndingPlay || !IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !GetWorld()) { return; }
	if (Deployment.IsValid() && !Deployment->IsDeploymentActive()) { StopRepeating(ENF_RepeatStopReason::DeploymentEnded); return; }
	if (EndTime > 0.0 && GetWorld()->GetTimeSeconds() >= EndTime) { ExpireRepeating(); return; }
	const uint32 CurrentGeneration = Generation;
	OnApplication.Broadcast(++ApplicationCount);
	// BP가 종료/재시작하거나 액터를 제거한 경우 이전 반복의 후속 처리를 하지 않는다.
	if (!bRepeating || bEndingPlay || CurrentGeneration != Generation || !IsValid(GetOwner())) { return; }
	if (ActiveMaxApplications > 0 && ApplicationCount >= ActiveMaxApplications) { StopRepeating(ENF_RepeatStopReason::CountReached); }
	else { ScheduleNext(); }
}

void UNF_RepeatApplicationComponent::StopRepeating(ENF_RepeatStopReason Reason)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bRepeating) { return; }
	bRepeating = false;
	++Generation;
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ApplicationTimer);
		GetWorld()->GetTimerManager().ClearTimer(DurationTimer);
	}
	OnRepeatStopped.Broadcast(Reason);
}

void UNF_RepeatApplicationComponent::ExpireRepeating() { StopRepeating(ENF_RepeatStopReason::DurationExpired); }
void UNF_RepeatApplicationComponent::HandleDeploymentEnded(ENF_DeploymentEndReason Reason) { StopRepeating(ENF_RepeatStopReason::DeploymentEnded); }

void UNF_RepeatApplicationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	StopRepeating(ENF_RepeatStopReason::OwnerRemoved);
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ApplicationTimer);
		GetWorld()->GetTimerManager().ClearTimer(DurationTimer);
	}
	if (Deployment.IsValid()) { Deployment->OnDeploymentEnded.RemoveDynamic(this, &UNF_RepeatApplicationComponent::HandleDeploymentEnded); }
	Super::EndPlay(Reason);
}
