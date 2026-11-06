#include "GAS/Abilities/Monster/NFGA_MonsterLeapAttack.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Tags/NFTag_Monster.h"
#include "Monster/AI/NF_MonsterAIController.h"
#include "Monster/NF_Monster.h"
#include "DrawDebugHelpers.h"

UNFGA_MonsterLeapAttack::UNFGA_MonsterLeapAttack()
{
	InstancingPolicy =
		EGameplayAbilityInstancingPolicy::InstancedPerActor;

	NetExecutionPolicy =
		EGameplayAbilityNetExecutionPolicy::ServerOnly;

	FGameplayTagContainer AbilityAssetTags;

	AbilityAssetTags.AddTag(
		NFTag_Monster::Ability_Monster_LeapAttack);

	SetAssetTags(AbilityAssetTags);

	ActivationOwnedTags.AddTag(
		NFTag_Monster::State_Monster_Attacking);

	ActivationBlockedTags.AddTag(
		NFTag_Monster::State_Monster_Dead);

	ActivationBlockedTags.AddTag(
		NFTag_Monster::State_Monster_Stunned);

	ActivationBlockedTags.AddTag(
		NFTag_Monster::State_Monster_HitReact);

	ActivationBlockedTags.AddTag(
		NFTag_Monster::State_Monster_Attacking);
}

void UNFGA_MonsterLeapAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true);

		return;
	}

	ANF_Monster* Monster =
		Cast<ANF_Monster>(ActorInfo->AvatarActor.Get());

	if (!IsValid(Monster))
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true);

		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster Leap Attack activated: %s"),
		*GetNameSafe(Monster));

	UAbilityTask_WaitDelay* PreparationTask =
		UAbilityTask_WaitDelay::WaitDelay(
			this,
			FMath::Max(PreparationTime, 0.01f));

	if (!PreparationTask)
	{
		EndAbility(
			Handle,
			ActorInfo,
			ActivationInfo,
			true,
			true);

		return;
	}

	PreparationTask->OnFinish.AddDynamic(
		this,
		&UNFGA_MonsterLeapAttack::HandlePreparationFinished);

	PreparationTask->ReadyForActivation();
}

void UNFGA_MonsterLeapAttack::HandlePreparationFinished()
{
	if (!CurrentActorInfo)
	{
		FinishLeapAbility(true);
		return;
	}

	ANF_Monster* Monster =
		Cast<ANF_Monster>(CurrentActorInfo->AvatarActor.Get());

	if (!IsValid(Monster) || !Monster->HasAuthority())
	{
		FinishLeapAbility(true);
		return;
	}

	ANF_MonsterAIController* MonsterAIController =
		Cast<ANF_MonsterAIController>(Monster->GetController());

	if (!MonsterAIController)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster Leap failed: AI Controller is null"));

		FinishLeapAbility(true);
		return;
	}

	AActor* TargetActor =
		MonsterAIController->GetCurrentTargetActor();

	if (!IsValid(TargetActor))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster Leap failed: Target is invalid"));

		FinishLeapAbility(true);
		return;
	}

	UCharacterMovementComponent* MovementComponent =
		Monster->GetCharacterMovement();

	UWorld* World = Monster->GetWorld();

	if (!MovementComponent || !World)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster Leap failed: Movement or World is null"));

		FinishLeapAbility(true);
		return;
	}

	// 땅 위에 있을 때만 도약
	if (!MovementComponent->IsMovingOnGround())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster Leap failed: Monster is not on the ground"));

		FinishLeapAbility(true);
		return;
	}

	const float GravityZ = MovementComponent->GetGravityZ();

	if (GravityZ >= 0.0f)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Monster Leap failed: Invalid gravity"));

		FinishLeapAbility(true);
		return;
	}

	// 준비가 끝난 시점의 플레이어 위치를 사용한다.
	const FVector StartLocation = Monster->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();

	FVector ToTarget = TargetLocation - StartLocation;

	// 이번 단계에서는 평지를 기준으로 수평 방향만 조준한다.
	ToTarget.Z = 0.0f;

	const float SafeMaxLeapDistance =
		FMath::Max(MaxLeapDistance, 0.0f);

	const FVector ClampedOffset =
		ToTarget.GetClampedToMaxSize(SafeMaxLeapDistance);

	// 같은 중력에서 도약 높이는 체공 시간의 제곱에 비례한다.
	const float SafeFlightTime = FMath::Max(
		FMath::Max(FlightTime, 0.2f) * FMath::Sqrt(FMath::Max(LeapHeightMultiplier, 0.01f)),
		0.2f);

	FVector LaunchVelocity = ClampedOffset / SafeFlightTime;

	// 평지에서 출발한 높이로 돌아온다고 가정한 수직 속도.
	LaunchVelocity.Z =
		-GravityZ * SafeFlightTime * 0.5f;

	// 기존 AI 이동 요청이 남아 있다면 정지한다.
	MonsterAIController->StopMovement();

	LeapingMonster = Monster;

	Monster->LandedDelegate.AddUniqueDynamic(
		this,
		&UNFGA_MonsterLeapAttack::HandleMonsterLanded);

	World->GetTimerManager().SetTimer(
		LandingTimeoutHandle,
		this,
		&UNFGA_MonsterLeapAttack::HandleLandingTimeout,
		FMath::Max(LandingTimeout, SafeFlightTime + 0.1f),
		false);

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster Leap launched: %s / Velocity: %s / Target: %s"),
		*GetNameSafe(Monster),
		*LaunchVelocity.ToString(),
		*TargetLocation.ToString());

	Monster->LaunchCharacter(
		LaunchVelocity,
		true,
		true);

	if (LeapMontage && LeapMontage->RateScale > 0.0f)
	{
		// 이동 틱과 지면 높이 차이를 감안해 착지 전에 블렌드 아웃이 시작되지 않도록 한다.
		constexpr float LandingTimingMargin = 0.2f;
		const float BlendOutLeadTime = FMath::Max(
			LeapMontage->BlendOutTriggerTime, LeapMontage->BlendOut.GetBlendTime());
		const float MontageDuration = SafeFlightTime + LandingTimingMargin + BlendOutLeadTime;
		const float PlayRate = LeapMontage->GetPlayLength() / (MontageDuration * LeapMontage->RateScale);

		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, LeapMontage, PlayRate);
		MontageTask->ReadyForActivation();
	}
}

void UNFGA_MonsterLeapAttack::HandleMonsterLanded(
	const FHitResult& Hit)
{
	ANF_Monster* Monster = LeapingMonster.Get();

	if (!IsValid(Monster) || !Monster->HasAuthority())
	{
		FinishLeapAbility(true);

		return;
	}

	if (UAbilitySystemComponent* MonsterASC = Monster->GetAbilitySystemComponent())
	{
		if (LeapMontage && MonsterASC->GetAnimatingAbility() == this
			&& MonsterASC->GetCurrentMontage() == LeapMontage)
		{
			MonsterASC->CurrentMontageStop();
		}
	}

	FVector LandingLocation = Monster->GetActorLocation();

	if (Hit.bBlockingHit)
	{
		LandingLocation = FVector(Hit.ImpactPoint);
	}
	
	if (LandingMontage)
	{
		UAbilityTask_PlayMontageAndWait* LandingTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, TEXT("LandingMontage"), LandingMontage);

		LandingTask->ReadyForActivation();
	}
	
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster Leap landed: %s / Impact: %s"),
		*GetNameSafe(Monster),
		*LandingLocation.ToString());
	
	DrawDebugSphere(
		Monster->GetWorld(),
		LandingLocation,
		LandingDamageRadius,
		24,
		FColor::Red,
		false,
		2.0f,
		0,
		2.0f);
	
	Monster->ApplyLandingDamage(
		LandingLocation,
		LandingDamageRadius,
		LandingDamageMultiplier);
	
	// 후딜 시작
	if (UWorld* World = Monster->GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			LandingTimeoutHandle);
	}

	UAbilityTask_WaitDelay* RecoveryTask =
		UAbilityTask_WaitDelay::WaitDelay(
			this,
			FMath::Max(RecoveryDuration, 0.01f));

	if (!RecoveryTask)
	{
		FinishLeapAbility(false);

		return;
	}

	RecoveryTask->OnFinish.AddDynamic(
		this,
		&UNFGA_MonsterLeapAttack::HandleRecoveryFinished);

	RecoveryTask->ReadyForActivation();

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster recovery started: %.2f seconds"),
		RecoveryDuration);
}

void UNFGA_MonsterLeapAttack::HandleLandingTimeout()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster Leap failed: Landing timeout"));

	FinishLeapAbility(true);
}

void UNFGA_MonsterLeapAttack::FinishLeapAbility(bool bWasCancelled)
{
	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		true,
		bWasCancelled);
}

void UNFGA_MonsterLeapAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ANF_Monster* Monster = LeapingMonster.Get())
	{
		Monster->LandedDelegate.RemoveDynamic(
			this,
			&UNFGA_MonsterLeapAttack::HandleMonsterLanded);
	}

	LeapingMonster.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			LandingTimeoutHandle);
	}

	Super::EndAbility(
		Handle,
		ActorInfo,
		ActivationInfo,
		bReplicateEndAbility,
		bWasCancelled);
}

void UNFGA_MonsterLeapAttack::HandleRecoveryFinished()
{
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("Monster recovery finished."));

	FinishLeapAbility(false);
}
