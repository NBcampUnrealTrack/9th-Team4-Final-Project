#include "GamePlay/Combat/Skill/Execution/NF_AccelerationEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

ANF_AccelerationEffect::ANF_AccelerationEffect()
{
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
}

bool ANF_AccelerationEffect::Initialize(ACharacter* Character, float Factor)
{
	if (!HasAuthority() || bActive || !IsValid(Character) || !FMath::IsFinite(Factor) || Factor < 1.0f) { return false; }
	Target = Character; Multiplier = Factor; bActive = true;
	if (!ApplyLocal()) { bActive = false; return false; }
	ForceNetUpdate();
	return true;
}

bool ANF_AccelerationEffect::ApplyLocal()
{
	if (AppliedMovement.IsValid()) { return true; }
	UCharacterMovementComponent* Movement = IsValid(Target) ? Target->GetCharacterMovement() : nullptr;
	if (!bActive || !IsValid(Movement) || Target->IsActorBeingDestroyed() || !FMath::IsFinite(Multiplier) || Multiplier < 1.0f
		|| !FMath::IsFinite(Movement->MaxWalkSpeed) || Movement->MaxWalkSpeed < 0.0f
		|| !FMath::IsFinite(Movement->MaxWalkSpeedCrouched) || Movement->MaxWalkSpeedCrouched < 0.0f
		|| !FMath::IsFinite(Movement->MaxWalkSpeed * Multiplier)
		|| !FMath::IsFinite(Movement->MaxWalkSpeedCrouched * Multiplier)) { return false; }
	AppliedMovement = Movement; AppliedMultiplier = Multiplier;
	Movement->MaxWalkSpeed *= AppliedMultiplier;
	Movement->MaxWalkSpeedCrouched *= AppliedMultiplier;
	return true;
}

void ANF_AccelerationEffect::RemoveLocal()
{
	if (UCharacterMovementComponent* Movement = AppliedMovement.Get())
	{
		if (FMath::IsFinite(Movement->MaxWalkSpeed)) { Movement->MaxWalkSpeed /= AppliedMultiplier; }
		if (FMath::IsFinite(Movement->MaxWalkSpeedCrouched)) { Movement->MaxWalkSpeedCrouched /= AppliedMultiplier; }
	}
	AppliedMovement.Reset(); AppliedMultiplier = 1.0f;
}

void ANF_AccelerationEffect::StopEffect()
{
	if (!HasAuthority()) { return; }
	bActive = false; RemoveLocal(); ForceNetUpdate(); SetLifeSpan(0.5f);
}

void ANF_AccelerationEffect::OnRep_Effect()
{
	if (!bActive) { RemoveLocal(); }
	else { ApplyLocal(); }
}

void ANF_AccelerationEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive) { return; }
	if (HasAuthority() && (!IsValid(Target) || Target->IsActorBeingDestroyed())) { StopEffect(); return; }
	// Target/component references can resolve after the first replicated update.
	ApplyLocal();
}

void ANF_AccelerationEffect::EndPlay(const EEndPlayReason::Type Reason)
{
	RemoveLocal();
	Super::EndPlay(Reason);
}

void ANF_AccelerationEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_AccelerationEffect, Target);
	DOREPLIFETIME(ANF_AccelerationEffect, Multiplier);
	DOREPLIFETIME(ANF_AccelerationEffect, bActive);
}
