#include "GamePlay/Combat/Skill/Execution/NF_JumpCountEffect.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

ANF_JumpCountEffect::ANF_JumpCountEffect()
{
	bReplicates = true; bNetUseOwnerRelevancy = true;
	PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickInterval = 0.1f;
}

ANF_JumpCountEffect* ANF_JumpCountEffect::FindOrCreate(ACharacter* Character)
{
	if (!IsValid(Character) || !Character->HasAuthority() || Character->IsActorBeingDestroyed()) { return nullptr; }
	for (TActorIterator<ANF_JumpCountEffect> It(Character->GetWorld()); It; ++It)
	{
		if (!It->IsActorBeingDestroyed() && It->Frame.Target == Character)
		{
			// Reuse a closing bridge on rapid re-equip, so clients never capture another bridge's modified count.
			It->bClosing = false; It->SetLifeSpan(0.0f); return *It;
		}
	}
	FActorSpawnParameters Params; Params.Owner = Character;
	auto* Effect = Character->GetWorld()->SpawnActor<ANF_JumpCountEffect>(Params);
	if (Effect) { Effect->Frame.Target = Character; }
	return Effect;
}

FGuid ANF_JumpCountEffect::AddRequest(int32 Count)
{
	if (!HasAuthority() || bClosing || !IsValid(Frame.Target) || Count < 2 || Count > 32) { return FGuid(); }
	const FGuid Id = FGuid::NewGuid(); Requests.Add(Id, Count); RefreshRequests(); return Id;
}

void ANF_JumpCountEffect::RemoveRequest(FGuid Id)
{
	if (HasAuthority() && Requests.Remove(Id) > 0) { RefreshRequests(); }
}

void ANF_JumpCountEffect::RefreshRequests()
{
	Frame.RequiredCount = 0;
	for (const auto& Request : Requests) { Frame.RequiredCount = FMath::Max(Frame.RequiredCount, Request.Value); }
	ApplyLocal(); ForceNetUpdate();
	if (Requests.IsEmpty()) { bClosing = true; SetLifeSpan(0.5f); }
}

void ANF_JumpCountEffect::ApplyLocal()
{
	if (Frame.RequiredCount == 0) { RemoveLocal(); return; }
	if (!IsValid(Frame.Target) || Frame.Target->IsActorBeingDestroyed()) { return; }
	if (AppliedCharacter.Get() != Frame.Target)
	{
		RemoveLocal(); AppliedCharacter = Frame.Target;
		OriginalCount = Frame.Target->JumpMaxCount;
	}
	else if (Frame.Target->JumpMaxCount != AppliedCount)
	{
		// Preserve a later external change instead of blindly restoring a stale value.
		OriginalCount = Frame.Target->JumpMaxCount;
	}
	AppliedCount = FMath::Max(OriginalCount, Frame.RequiredCount);
	Frame.Target->JumpMaxCount = AppliedCount;
	// Do not reset JumpCurrentCount: equipping mid-air must not refill used jumps.
}

void ANF_JumpCountEffect::RemoveLocal()
{
	if (ACharacter* Character = AppliedCharacter.Get())
	{
		if (Character->JumpMaxCount == AppliedCount) { Character->JumpMaxCount = OriginalCount; }
	}
	AppliedCharacter.Reset();
}

void ANF_JumpCountEffect::OnRep_Frame() { ApplyLocal(); }

void ANF_JumpCountEffect::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && !bClosing && (!IsValid(Frame.Target) || Frame.Target->IsActorBeingDestroyed()))
	{
		Requests.Reset(); RefreshRequests(); return;
	}
	// Retry only unresolved references, rather than overriding character settings every tick.
	if (Frame.RequiredCount > 0 && !AppliedCharacter.IsValid()) { ApplyLocal(); }
}

void ANF_JumpCountEffect::EndPlay(const EEndPlayReason::Type Reason)
{
	RemoveLocal(); Super::EndPlay(Reason);
}

void ANF_JumpCountEffect::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_JumpCountEffect, Frame);
}
