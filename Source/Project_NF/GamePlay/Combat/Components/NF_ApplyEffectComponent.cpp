#include "GamePlay/Combat/Components/NF_ApplyEffectComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Actor.h"

namespace
{
	bool CanOwnEffect(const UGameplayEffect* Effect)
	{
		if (!Effect || Effect->DurationPolicy == EGameplayEffectDurationType::Instant) { return false; }
		// UE 5.7 GetStackingType의 DLL export 누락을 피한다. 공유 스택 효과는 제거하지 않는다.
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		const bool bUnique = Effect->StackingType == EGameplayEffectStackingType::None;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		return bUnique;
	}
}

UNF_ApplyEffectComponent::UNF_ApplyEffectComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UNF_ApplyEffectComponent::HasValidSettings() const
{
	if (Effects.IsEmpty()) { return false; }
	for (const FNF_EffectApplication& Entry : Effects)
	{
		if (!Entry.EffectClass || (Entry.Removal == ENF_EffectRemovalPolicy::OnExecutionEnd && !CanOwnEffect(Entry.EffectClass->GetDefaultObject<UGameplayEffect>()))) { return false; }
		for (const auto& Value : Entry.SetByCaller) { if (!Value.Key.IsValid() || !FMath::IsFinite(Value.Value)) { return false; } }
	}
	return true;
}

bool UNF_ApplyEffectComponent::PrepareEffects(AActor* SourceAvatar, float Level)
{
	if (bEndingPlay || !IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(SourceAvatar)
		|| !FMath::IsFinite(Level) || Level <= 0.0f || !HasValidSettings()) { return false; }
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAvatar);
	if (!ASC) { return false; }
	StopEffects();
	if (bEndingPlay) { return false; }
	bStopped = false; ++Generation; Source = SourceAvatar;
	for (const FNF_EffectApplication& Entry : Effects)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(GetOwner());
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Entry.EffectClass, Level, Context);
		if (!Spec.IsValid()) { StopEffects(); return false; }
		for (const auto& Value : Entry.SetByCaller) { Spec.Data->SetSetByCallerMagnitude(Value.Key, Value.Value); }
		PreparedSpecs.Add(Spec);
		PreparedRemoval.Add(Entry.Removal == ENF_EffectRemovalPolicy::OnExecutionEnd);
	}
	return true;
}

bool UNF_ApplyEffectComponent::SubmitSpec(AActor* Authority, AActor* Target, const FGameplayEffectSpecHandle& Spec,
	FVector Origin, const FHitResult& Hit, FActiveGameplayEffectHandle* OutHandle)
{
	if (OutHandle) { *OutHandle = FActiveGameplayEffectHandle(); }
	if (!IsValid(Authority) || !Authority->HasAuthority() || !IsValid(Target) || !Target->HasAuthority()
		|| !Spec.IsValid() || !Spec.Data->GetContext().IsValid() || Origin.ContainsNaN()) { return false; }
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!IsValid(ASC)) { return false; }
	FGameplayEffectSpec TargetSpec(*Spec.Data.Get());
	FGameplayEffectContextHandle Context = TargetSpec.GetContext().Duplicate();
	Context.AddOrigin(Origin);
	if (Hit.GetActor() || Hit.bBlockingHit) { Context.AddHitResult(Hit, true); }
	TargetSpec.SetContext(Context, true);
	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(TargetSpec);
	if (OutHandle) { *OutHandle = Handle; }
	return true;
}

bool UNF_ApplyEffectComponent::ApplyInternal(AActor* Target, const FGameplayEffectSpecHandle& Spec,
	FVector Origin, const FHitResult& Hit, bool bRemove, int32 EntryIndex)
{
	if (bStopped || bEndingPlay || !IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !IsValid(Target) || !Spec.IsValid()
		|| (bRemove && !CanOwnEffect(Spec.Data->Def))) { return false; }
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!ASC) { return false; }
	if (bRemove)
	{
		for (const FTrackedEffect& Existing : Tracked)
		{
			// 같은 실행의 지속 버프를 반복 호출해도 중복 인스턴스를 만들지 않는다.
			if (Existing.ASC.Get() == ASC && Existing.EntryIndex == EntryIndex && Existing.Definition == Spec.Data->Def.Get()
				&& ASC->GetActiveGameplayEffect(Existing.Handle)) { return true; }
		}
	}
	const uint32 PreviousGeneration = Generation;
	FActiveGameplayEffectHandle Handle;
	if (!SubmitSpec(GetOwner(), Target, Spec, Origin, Hit, &Handle)) { return false; }
	if (bRemove && Handle.IsValid())
	{
		if (bStopped || bEndingPlay || PreviousGeneration != Generation || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed())
		{
			if (IsValid(ASC)) { ASC->RemoveActiveGameplayEffect(Handle); }
		}
		else { Tracked.Add({ ASC, Handle, Spec.Data->Def.Get(), EntryIndex }); }
	}
	return true;
}

bool UNF_ApplyEffectComponent::ApplySpecToTarget(AActor* Target, const FGameplayEffectSpecHandle& Spec,
	FVector Origin, const FHitResult& Hit, bool bRemoveOnStop)
{
	return ApplyInternal(Target, Spec, Origin, Hit, bRemoveOnStop, INDEX_NONE);
}
int32 UNF_ApplyEffectComponent::ApplyToTarget(AActor* Target, const FHitResult& Hit, FVector Origin)
{
	if (PreparedSpecs.IsEmpty()) { return 0; }
	const uint32 PreviousGeneration = Generation;
	// GE 적용 이벤트가 Stop/Prepare를 호출할 수 있어 캐시 배열을 직접 순회하지 않는다.
	const TArray<FGameplayEffectSpecHandle> Specs = PreparedSpecs;
	const TArray<bool> Removal = PreparedRemoval;
	int32 Submitted = 0;
	for (int32 Index = 0; Index < Specs.Num(); ++Index)
	{
		if (bStopped || PreviousGeneration != Generation) { break; }
		Submitted += ApplyInternal(Target, Specs[Index], Origin, Hit, Removal[Index], Index) ? 1 : 0;
	}
	return Submitted;
}
int32 UNF_ApplyEffectComponent::ApplyToSelf()
{
	return Source.IsValid() ? ApplyToTarget(Source.Get(), FHitResult(), Source->GetActorLocation()) : 0;
}
int32 UNF_ApplyEffectComponent::ApplyToTargets(const TArray<FNF_TargetQueryResult>& Targets, FVector Origin)
{
	TSet<UAbilitySystemComponent*> Submitted;
	for (const FNF_TargetQueryResult& Target : Targets)
	{
		if (bStopped || bEndingPlay) { break; }
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target.Target);
		if (!IsValid(ASC) || Submitted.Contains(ASC)) { continue; }
		Submitted.Add(ASC);
		if (ApplyToTarget(Target.Target, Target.Hit, Origin) == 0) { Submitted.Remove(ASC); }
	}
	return Submitted.Num();
}
void UNF_ApplyEffectComponent::StopEffects()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) { return; }
	bStopped = true; ++Generation;
	PreparedSpecs.Reset(); PreparedRemoval.Reset(); Source.Reset();
	TArray<FTrackedEffect> Owned = MoveTemp(Tracked);
	for (const FTrackedEffect& Effect : Owned) { if (Effect.ASC.IsValid()) { Effect.ASC->RemoveActiveGameplayEffect(Effect.Handle); } }
}
void UNF_ApplyEffectComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true; StopEffects(); Super::EndPlay(Reason);
}
