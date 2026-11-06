#include "GamePlay/Combat/Projectiles/NF_BossHomingProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GamePlay/Combat/GameplayCue/NF_BossNiagaraBurst.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GAS/Tags/NFGameplayCueTags.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "Player/Character/NF_Player.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ANF_BossHomingProjectile::ANF_BossHomingProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) Mesh->SetStaticMesh(Sphere.Object);
	Mesh->SetRelativeScale3D(FVector(0.4f));
}

void ANF_BossHomingProjectile::InitializeProjectile(ANF_Player* Target, float Speed, float TriggerRadius,
	float BlastRadius, float LaunchVelocity, const FGameplayEffectSpecHandle& Damage, const FGameplayEffectSpecHandle& Airborne)
{
	TargetPlayer = Target;
	MoveSpeed = FMath::Max(Speed, 1.f);
	DetectionRadius = FMath::Max(TriggerRadius, 1.f);
	ExplosionRadius = FMath::Max(BlastRadius, DetectionRadius);
	AirborneLaunchVelocity = FMath::Max(LaunchVelocity, 0.f);
	DamageSpec = Damage;
	AirborneSpec = Airborne;
}

void ANF_BossHomingProjectile::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(HasAuthority());
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(LifetimeTimer, this, &ThisClass::Explode, TrackingLifetime, false);
	}
}

FVector ANF_BossHomingProjectile::CalculateTrackingLocation(const FVector& Current, const FVector& Target,
	float Speed, float DeltaSeconds)
{
	const FVector Delta = Target - Current;
	return Current + Delta.GetSafeNormal() * FMath::Min(Delta.Size(), FMath::Max(Speed, 0.f) * FMath::Max(DeltaSeconds, 0.f));
}

bool ANF_BossHomingProjectile::IsLivingPlayer(ANF_Player* Player) const
{
	const auto* ASC = IsValid(Player) ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player) : nullptr;
	return ASC && !ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead);
}

void ANF_BossHomingProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || bExploded) return;
	const FVector Start = GetActorLocation();
	const FVector End = IsLivingPlayer(TargetPlayer.Get())
		? CalculateTrackingLocation(Start, TargetPlayer->GetActorLocation(), MoveSpeed, DeltaSeconds) : Start;
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BossHomingProximity), false, this);
	Params.AddIgnoredActor(GetOwner());
	// Sweep the detection sphere to avoid skipping players on a slow server frame.
	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, Start, End, FQuat::Identity, Objects,
		FCollisionShape::MakeSphere(DetectionRadius), Params);
	float FirstContact = 2.f;
	for (const FHitResult& Hit : Hits)
	{
		if (IsLivingPlayer(Cast<ANF_Player>(Hit.GetActor()))) FirstContact = FMath::Min(FirstContact, Hit.Time);
	}
	if (FirstContact <= 1.f)
	{
		SetActorLocation(FMath::Lerp(Start, End, FirstContact));
		Explode();
		return;
	}
	const FVector Step = End - Start;
	if (Step.IsNearlyZero()) SetActorLocation(End);
	else SetActorLocationAndRotation(End, Step.Rotation());
	// Also covers stationary missiles and initial overlaps.
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, End, FQuat::Identity, Objects,
		FCollisionShape::MakeSphere(DetectionRadius), Params);
	for (const auto& Overlap : Overlaps)
	{
		if (IsLivingPlayer(Cast<ANF_Player>(Overlap.GetActor()))) { Explode(); return; }
	}
}

void ANF_BossHomingProjectile::Explode()
{
	if (!HasAuthority() || bExploded || IsActorBeingDestroyed()) return;
	bExploded = true;
	UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Explode Projectile=%s Location=%s Age=%.2f"),
		*GetName(), *GetActorLocation().ToString(), GetGameTimeSinceCreation());
	GetWorldTimerManager().ClearTimer(LifetimeTimer);
	const FVector Center = GetActorLocation();
	ANF_BossNiagaraBurst::SpawnBurst(this, ExplosionNiagaraSystem, Center, FRotator::ZeroRotator, ExplosionNiagaraScale);
	if (auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
	{
		FGameplayCueParameters Cue;
		Cue.Location = Center;
		Cue.Instigator = GetOwner();
		Cue.EffectCauser = this;
		Cue.RawMagnitude = ExplosionRadius;
		ASC->ExecuteGameplayCue(NFGameplayCueTags::GameplayCue_Boss_ProjectileImpact, Cue);
	}
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ExplosionRadius));
	TSet<ANF_Player*> Affected;
	for (const auto& Overlap : Overlaps)
	{
		ANF_Player* Player = Cast<ANF_Player>(Overlap.GetActor());
		if (!IsLivingPlayer(Player) || Affected.Contains(Player)) continue;
		Affected.Add(Player);
		auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);
		for (const FGameplayEffectSpecHandle* Spec : { &DamageSpec, &AirborneSpec })
		{
			if (!Spec->IsValid()) continue;
			FGameplayEffectSpec Copy(*Spec->Data.Get());
			auto Context = Copy.GetContext().Duplicate();
			Context.AddOrigin(Center);
			Copy.SetContext(Context);
			ASC->ApplyGameplayEffectSpecToSelf(Copy);
		}
		Player->LaunchCharacter(FVector(0.f, 0.f, AirborneLaunchVelocity), false, true);
	}
	OnHomingExplosion(Center, ExplosionRadius);
	UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] Explosion Projectile=%s AffectedPlayers=%d Radius=%.1f"),
		*GetName(), Affected.Num(), ExplosionRadius);
	Destroy();
}

void ANF_BossHomingProjectile::EndPlay(const EEndPlayReason::Type Reason)
{
	UE_LOG(LogTemp, Display, TEXT("[BossHomingDiag] ProjectileEnd Name=%s Authority=%d Exploded=%d Reason=%d Location=%s"),
		*GetName(), HasAuthority(), bExploded, static_cast<int32>(Reason), *GetActorLocation().ToString());
	GetWorldTimerManager().ClearTimer(LifetimeTimer);
	Super::EndPlay(Reason);
}
