#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Portal.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Components/NF_GroundTargetingComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GamePlay/Combat/Skill/Portal/NF_Portal.h"
#include "Project_NF.h"
#include "TimerManager.h"

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 기존 GA 비용/쿨다운/취소 규칙과 Deployable 수명을 재사용한다. 특정 PC에 의존하지 않는다.
UNFGA_Portal::UNFGA_Portal()
{
	GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Movement;
	GetMutableActiveDefaults().ExecutionMode = ENF_SkillExecutionMode::Instant;
	GetMutableActiveDefaults().bActivateOnInputRelease = true;
	GetMutableActiveDefaults().bRequiresConfirmedTargetData = true;
	GetMutableSkillDefaults().BuildCost = 5;
	PortalClass = ANF_Portal::StaticClass();
}
bool UNFGA_Portal::HasValidPortalSettings() const
{
	return PortalClass && PortalClass->GetDefaultObject<ANF_Portal>()->HasValidSettings() && !GetActiveSettings().ExecutionActorClass
		&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly && GetActiveSettings().bRequiresConfirmedTargetData
		&& GetSkillExecutionMode() == ENF_SkillExecutionMode::Instant && FMath::IsFinite(GetCastTime()) && GetCastTime() >= 0.0f
		&& FMath::IsFinite(PortalRange) && PortalRange >= 100.0f && FMath::IsFinite(GroundSearchDepth) && GroundSearchDepth >= 100.0f
		&& FMath::IsFinite(EntranceForwardDistance) && EntranceForwardDistance >= 100.0f
		&& FMath::IsFinite(MinimumSeparation) && MinimumSeparation >= 100.0f
		&& FMath::IsFinite(PreviewRadius) && PreviewRadius > 0.0f;
}
bool UNFGA_Portal::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	return ActorInfo && Cast<ACharacter>(ActorInfo->AvatarActor.Get()) && HasValidPortalSettings()
		&& Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
bool UNFGA_Portal::GetSkillPreviewData_Implementation(AActor* Avatar, FNF_SkillPreviewData& Out) const
{
	Out = FNF_SkillPreviewData();
	Out.TargetRange = PortalRange; Out.GroundSearchDepth = GroundSearchDepth; Out.DisplayRadius = PreviewRadius;
	return IsValid(Avatar) && HasValidPortalSettings();
}
bool UNFGA_Portal::BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& Out) const
{
	Out.Clear();
	FHitResult Hit; FVector Direction;
	if (!HasValidPortalSettings() || !UNF_GroundTargetingComponent::CalculateGroundTarget(GetAvatarActorFromActorInfo(),
		PortalRange, GroundSearchDepth, Hit, Direction)) { return false; }
	Out = MakeSkillTargetData(ENF_SkillTargetKind::GroundPoint, Hit.ImpactPoint, Direction);
	return true;
}
bool UNFGA_Portal::ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& Data) const
{
	const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(Data);
	return HasValidPortalSettings() && UNFGA_ActiveSkillBase::ValidateConfirmedTargetData_Implementation(Data)
		&& Target && Target->Kind == ENF_SkillTargetKind::GroundPoint
		&& UNF_GroundTargetingComponent::ValidateGroundTarget(GetAvatarActorFromActorInfo(), Target->Location, PortalRange, GroundSearchDepth);
}
void UNFGA_Portal::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !GetWorld() || !HasValidPortalSettings() || !CanContinueSkillExecution()
		|| !UNFGA_Portal::ValidateConfirmedTargetData_Implementation(GetConfirmedTargetData())
		|| !ValidateConfirmedTargetData(GetConfirmedTargetData()))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return;
	}
	PlacementAvatar = GetAvatarActorFromActorInfo();
	if (GetCastTime() > 0.0f) { GetWorld()->GetTimerManager().SetTimer(PlacementTimer, this, &ThisClass::CreatePortalPair, GetCastTime(), false); }
	else { CreatePortalPair(); }
}
void UNFGA_Portal::FailPlacement(const TCHAR* Reason)
{
	UE_LOG(LogProject_NF, Warning, TEXT("[Portal] Placement failed: %s"), Reason);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
void UNFGA_Portal::CreatePortalPair()
{
	if (!IsActive()) { return; }
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	const FGameplayAbilityTargetDataHandle TargetData = GetConfirmedTargetData();
	const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(TargetData);
	if (!IsValid(Character) || Character->IsActorBeingDestroyed() || Character != PlacementAvatar.Get()
		|| !Character->GetCapsuleComponent() || !Character->GetCharacterMovement() || !CanContinueSkillExecution()
		|| !HasValidPortalSettings() || !Target || Target->Kind != ENF_SkillTargetKind::GroundPoint
		|| !UNFGA_Portal::ValidateConfirmedTargetData_Implementation(TargetData) || !ValidateConfirmedTargetData(TargetData))
	{
		FailPlacement(TEXT("avatar, configuration or confirmed ground target is invalid")); return;
	}
	const FVector Forward = FVector(Target->AimDirection).GetSafeNormal2D();
	if (Forward.IsNearlyZero()) { FailPlacement(TEXT("horizontal aim direction is invalid")); return; }
	const FVector Near = Character->GetActorLocation() + Forward * EntranceForwardDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NF_PortalEntrance), false, Character);
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FHitResult Floor;
	if (!GetWorld()->LineTraceSingleByObjectType(Floor, Near + FVector::UpVector * 150.0f,
		Near - FVector::UpVector * GroundSearchDepth, Objects, Params) || !Character->GetCharacterMovement()->IsWalkable(Floor)
		|| !UNF_GroundTargetingComponent::ValidateGroundTarget(Character, Floor.ImpactPoint, EntranceForwardDistance + 100.0f, GroundSearchDepth))
	{
		FailPlacement(TEXT("entrance has no visible walkable ground")); return;
	}
	const FVector ExitLocation = Target->Location;
	if (FVector::DistSquared2D(Floor.ImpactPoint, ExitLocation) < FMath::Square(MinimumSeparation))
	{
		FailPlacement(TEXT("entrance and exit are too close")); return;
	}
	const FRotator Rotation(0.0f, Forward.Rotation().Yaw, 0.0f);
	const FTransform EntranceTransform(Rotation, Floor.ImpactPoint), ExitTransform(Rotation, ExitLocation);
	PendingEntrance = GetWorld()->SpawnActorDeferred<ANF_Portal>(PortalClass, EntranceTransform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	PendingExit = GetWorld()->SpawnActorDeferred<ANF_Portal>(PortalClass, ExitTransform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(PendingEntrance) || !IsValid(PendingExit)) { FailPlacement(TEXT("portal actor spawn failed")); return; }
	PendingEntrance->InitializePortal(PendingExit, false, Character, GetAbilitySystemComponentFromActorInfo());
	PendingExit->InitializePortal(PendingEntrance, true, Character, GetAbilitySystemComponentFromActorInfo());
	PendingEntrance->FinishSpawning(EntranceTransform);
	if (!IsActive()) { return; }
	if (!IsValid(PendingEntrance) || !IsValid(PendingExit)) { FailPlacement(TEXT("portal was removed during initialization")); return; }
	PendingExit->FinishSpawning(ExitTransform);
	if (!IsActive()) { return; }
	FVector Arrival;
	if (!IsValid(PendingEntrance) || !IsValid(PendingExit) || !PendingEntrance->CanPlaceFor(Character) || !PendingExit->CanPlaceFor(Character)
		|| !PendingExit->FindArrivalLocation(Character, Arrival))
	{
		FailPlacement(TEXT("portal volume or exit capsule space is blocked")); return;
	}
	// 위치 검사가 통과한 뒤에만 비용/쿨다운을 적용한다. 진입 판정도 Commit 이후 켠다.
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo)) { FailPlacement(TEXT("GAS commit failed")); return; }
	if (!IsActive()) { return; }
	if (!IsValid(PendingExit) || !PendingExit->StartPortal() || !IsActive()
		|| !IsValid(PendingEntrance) || !PendingEntrance->StartPortal())
	{
		if (IsActive()) { FailPlacement(TEXT("deployment start failed")); }
		return;
	}
	// 성공한 쌍은 GA와 독립적으로 Deployable 수명을 사용한다.
	PendingEntrance = nullptr; PendingExit = nullptr;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
void UNFGA_Portal::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0) { Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled); return; }
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(PlacementTimer); }
	ANF_Portal* Entrance = PendingEntrance; ANF_Portal* Exit = PendingExit;
	PendingEntrance = nullptr; PendingExit = nullptr; PlacementAvatar.Reset();
	if (IsValid(Entrance)) { Entrance->Destroy(); }
	if (IsValid(Exit)) { Exit->Destroy(); }
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
