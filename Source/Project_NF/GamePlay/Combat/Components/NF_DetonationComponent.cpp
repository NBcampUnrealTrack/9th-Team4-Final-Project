#include "GamePlay/Combat/Components/NF_DetonationComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

UNF_DetonationComponent::UNF_DetonationComponent() { PrimaryComponentTick.bCanEverTick = false; }
bool UNF_DetonationComponent::HasValidSettings() const
{
	return (Mode == ENF_DetonationMode::Fuse || Mode == ENF_DetonationMode::Impact || Mode == ENF_DetonationMode::FuseOrImpact)
		&& FMath::IsFinite(FuseTime) && FuseTime >= 0.01f && FMath::IsFinite(MaxLifetime) && MaxLifetime >= 0.01f;
}
float UNF_DetonationComponent::GetPreviewTime() const { return Mode == ENF_DetonationMode::Impact ? MaxLifetime : FuseTime; }
bool UNF_DetonationComponent::Arm(UPrimitiveComponent* Collision, bool bListenForCollision)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld() || bDetonated || bArmed || !HasValidSettings()
		|| (Mode != ENF_DetonationMode::Fuse && !IsValid(Collision))) { return false; }
	bArmed = true;
	BoundCollision = Collision;
	if (bListenForCollision && Collision && Mode != ENF_DetonationMode::Fuse) { Collision->OnComponentHit.AddDynamic(this, &UNF_DetonationComponent::HandleHit); }
	GetWorld()->GetTimerManager().SetTimer(DetonationTimer, this, &UNF_DetonationComponent::Detonate, GetPreviewTime(), false);
	return true;
}
void UNF_DetonationComponent::Detonate()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bArmed || bDetonated) { return; }
	bDetonated = true;
	Disarm();
	OnDetonated.Broadcast();
}
void UNF_DetonationComponent::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	AActor* Owner = GetOwner();
	if (IsValid(Owner) && OtherActor != Owner->GetOwner() && OtherActor != Owner) { Detonate(); }
}
void UNF_DetonationComponent::Disarm()
{
	bArmed = false;
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(DetonationTimer); }
	if (BoundCollision.IsValid()) { BoundCollision->OnComponentHit.RemoveDynamic(this, &UNF_DetonationComponent::HandleHit); }
	BoundCollision.Reset();
}
void UNF_DetonationComponent::PlayCosmetic(FVector Location, float Radius)
{
	if (GetNetMode() == NM_DedicatedServer || !GetWorld()) { return; }
	if (ExplosionEffect) { UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, Location); }
	if (bDrawDebugRadius && Radius > 0.0f) { DrawDebugSphere(GetWorld(), Location, Radius, 32, FColor::Orange, false, 1.5f); }
}
void UNF_DetonationComponent::EndPlay(const EEndPlayReason::Type Reason) { Disarm(); Super::EndPlay(Reason); }
