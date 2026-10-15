#include "GamePlay/Combat/Components/NF_DeployableComponent.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

UNF_DeployableComponent::UNF_DeployableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNF_DeployableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNF_DeployableComponent, Status);
}

void UNF_DeployableComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoStart && !bManagedStart && GetOwner()->HasAuthority()) { StartDeployment(); }
	else { NotifyStatus(); }
}

bool UNF_DeployableComponent::StartDeployment()
{
	if (!IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed() || !GetOwner()->HasAuthority() || !GetWorld()
		|| Status.State != ENF_DeploymentState::NotStarted || !FMath::IsFinite(Lifetime) || Lifetime < 0.0f)
	{
		return false;
	}
	Status.State = ENF_DeploymentState::Active;
	if (Lifetime > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(LifetimeTimer, this, &UNF_DeployableComponent::ExpireDeployment, Lifetime, false);
	}
	GetOwner()->ForceNetUpdate();
	NotifyStatus();
	return IsDeploymentActive();
}

void UNF_DeployableComponent::ExpireDeployment()
{
	EndDeployment(ENF_DeploymentEndReason::Expired);
}

void UNF_DeployableComponent::EndDeployment(ENF_DeploymentEndReason Reason)
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority() || Status.State == ENF_DeploymentState::Ended) { return; }
	Status.State = ENF_DeploymentState::Ended;
	Status.Reason = Reason;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer); }
	GetOwner()->ForceNetUpdate();
	// 정리를 먼저 통지한다. 종료 이벤트는 반드시 서버/클라이언트 구분해서 사용한다.
	NotifyStatus();
	if (bDestroyOwnerOnEnd && IsValid(GetOwner()))
	{
		GetOwner()->SetLifeSpan(FMath::Max(FMath::IsFinite(DestroyDelay) ? DestroyDelay : 0.5f, 0.01f));
	}
}

void UNF_DeployableComponent::OnRep_Status()
{
	if (HasBegunPlay()) { NotifyStatus(); }
}

void UNF_DeployableComponent::NotifyStatus()
{
	if (Status.State == ENF_DeploymentState::Active && !bStartedNotified)
	{
		bStartedNotified = true;
		OnDeploymentStarted.Broadcast();
	}
	else if (Status.State == ENF_DeploymentState::Ended && !bEndedNotified)
	{
		bEndedNotified = true;
		if (bDisableOwnerCollisionOnEnd && IsValid(GetOwner())) { GetOwner()->SetActorEnableCollision(false); }
		OnDeploymentEnded.Broadcast(Status.Reason);
	}
}

void UNF_DeployableComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(LifetimeTimer); }
	if (Status.State == ENF_DeploymentState::Active && !bEndedNotified)
	{
		Status.State = ENF_DeploymentState::Ended;
		Status.Reason = ENF_DeploymentEndReason::OwnerRemoved;
		bEndedNotified = true;
		OnDeploymentEnded.Broadcast(ENF_DeploymentEndReason::OwnerRemoved);
	}
	Super::EndPlay(Reason);
}
