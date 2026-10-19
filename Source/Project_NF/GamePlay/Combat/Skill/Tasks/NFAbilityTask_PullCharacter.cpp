#include "GamePlay/Combat/Skill/Tasks/NFAbilityTask_PullCharacter.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"

UNFAbilityTask_PullCharacter::UNFAbilityTask_PullCharacter() { bTickingTask = true; }

UNFAbilityTask_PullCharacter* UNFAbilityTask_PullCharacter::PullCharacter(UGameplayAbility* OwningAbility,
	ACharacter* Target, FVector Destination, FNF_CharacterPullSettings Settings)
{
	auto* Task = NewAbilityTask<UNFAbilityTask_PullCharacter>(OwningAbility);
	Task->TargetCharacter = Target; Task->DestinationLocation = Destination; Task->PullSettings = Settings;
	return Task;
}

void UNFAbilityTask_PullCharacter::Activate()
{
	AActor* Source = GetAvatarActor(); OriginalAvatar = Source;
	if (!IsValid(Source) || !Source->HasAuthority() || !TargetCharacter.IsValid()) { Result = ENF_CharacterPullResult::InvalidTarget; HandleFinished(false); return; }
	FActorSpawnParameters Params; Params.Owner = TargetCharacter.Get();
	ANF_CharacterPullActor* Actor = Source->GetWorld()->SpawnActor<ANF_CharacterPullActor>(Params);
	if (!Actor) { Result = ENF_CharacterPullResult::MovementRejected; HandleFinished(false); return; }
	PullActor = Actor;
	Actor->OnPullEnded.AddUObject(this, &ThisClass::HandleFinished);
	if (!Actor->StartPull(Source, TargetCharacter.Get(), DestinationLocation, PullSettings, AbilitySystemComponent.Get(), Ability->GetAbilityLevel()))
	{
		// StartPull may already have broadcast failure and ended this task.
		if (PullActor.IsValid()) { HandleFinished(false); }
	}
	else { bStartedPull = true; }
}

void UNFAbilityTask_PullCharacter::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	if (!OriginalAvatar.IsValid() || GetAvatarActor() != OriginalAvatar.Get() || !PullActor.IsValid()) { HandleFinished(false); }
}

void UNFAbilityTask_PullCharacter::HandleFinished(bool bReached)
{
	if (PullActor.IsValid()) { Result = PullActor->GetResult(); }
	if (ShouldBroadcastAbilityTaskDelegates()) { OnFinished.Broadcast(bReached); }
	EndTask();
}

void UNFAbilityTask_PullCharacter::OnDestroy(bool AbilityIsEnding)
{
	if (ANF_CharacterPullActor* Actor = PullActor.Get())
	{
		Actor->OnPullEnded.RemoveAll(this); Actor->StopPull();
	}
	PullActor.Reset();
	Super::OnDestroy(AbilityIsEnding);
}
