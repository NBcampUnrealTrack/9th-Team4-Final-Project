#include "GamePlay/Combat/Components/NF_HitEffectComponent.h"
#include "GamePlay/Combat/Components/NF_ApplyEffectComponent.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UNF_HitEffectComponent::UNF_HitEffectComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UNF_HitEffectComponent::HasValidSettings() const { return FMath::IsFinite(ReapplyInterval) && ReapplyInterval >= 0.0f; }
bool UNF_HitEffectComponent::StartListening(AActor* SourceAvatar)
{
	if (bEndingPlay || bListening || !IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !HasValidSettings()) { return false; }
	ApplyEffect = GetOwner()->FindComponentByClass<UNF_ApplyEffectComponent>();
	if (!ApplyEffect.IsValid()) { return false; }
	Collision = GetOwner()->FindComponentByClass<USphereComponent>();
	if (!Collision.IsValid()) { Collision = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()); }
	if (!Collision.IsValid()) { Collision = GetOwner()->FindComponentByClass<UPrimitiveComponent>(); }
	if (!Collision.IsValid()) { return false; }
	++Generation; LastSubmission.Reset(); Source = SourceAvatar; bListening = true;
	Collision->OnComponentHit.AddDynamic(this, &UNF_HitEffectComponent::HandleHit);
	if (bListenToOverlaps) { Collision->OnComponentBeginOverlap.AddDynamic(this, &UNF_HitEffectComponent::HandleOverlap); }
	return true;
}
int32 UNF_HitEffectComponent::SubmitHit(AActor* Target, const FHitResult& Hit)
{
	if (!bListening || bEndingPlay || !IsValid(GetOwner()) || !GetOwner()->HasAuthority() || !GetWorld() || Target == GetOwner()
		|| !ApplyEffect.IsValid() || !UNF_TargetQueryComponent::PassesFilter(Target, Source.Get(), Filter)) { return 0; }
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!ASC) { return 0; }
	for (auto It = LastSubmission.CreateIterator(); It; ++It) { if (!It.Key().IsValid()) { It.RemoveCurrent(); } }
	const double Now = GetWorld()->GetTimeSeconds();
	const TWeakObjectPtr<UAbilitySystemComponent> Key(ASC);
	if (const double* Previous = LastSubmission.Find(Key))
	{
		if (bOncePerTarget || Now - *Previous < ReapplyInterval) { return 0; }
	}
	const uint32 PreviousGeneration = Generation;
	LastSubmission.Add(Key, Now); // GE 재진입으로 동일 명중이 중복 실행되지 않게 먼저 예약한다.
	const int32 Count = ApplyEffect->ApplyToTarget(Target, Hit, GetOwner()->GetActorLocation());
	if (PreviousGeneration != Generation || !bListening) { return Count; }
	if (Count == 0) { LastSubmission.Remove(Key); }
	else { OnHitEffectsSubmitted.Broadcast(Target, Count); }
	return Count;
}
void UNF_HitEffectComponent::HandleHit(UPrimitiveComponent* Component, AActor* Target, UPrimitiveComponent* Other,
	FVector Impulse, const FHitResult& Hit) { SubmitHit(Target, Hit); }
void UNF_HitEffectComponent::HandleOverlap(UPrimitiveComponent* Component, AActor* Target, UPrimitiveComponent* Other,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (bFromSweep) { SubmitHit(Target, Hit); }
	else if (IsValid(Target))
	{
		const FVector Point = IsValid(Other) ? Other->Bounds.Origin : Target->GetActorLocation();
		SubmitHit(Target, FHitResult(Target, Other, Point, FVector::ZeroVector));
	}
}
void UNF_HitEffectComponent::StopListening()
{
	bListening = false; ++Generation; LastSubmission.Reset();
	if (Collision.IsValid())
	{
		Collision->OnComponentHit.RemoveDynamic(this, &UNF_HitEffectComponent::HandleHit);
		Collision->OnComponentBeginOverlap.RemoveDynamic(this, &UNF_HitEffectComponent::HandleOverlap);
	}
	Collision.Reset(); ApplyEffect.Reset(); Source.Reset();
}
void UNF_HitEffectComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true; StopListening(); Super::EndPlay(Reason);
}
