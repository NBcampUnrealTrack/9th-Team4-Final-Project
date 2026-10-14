#include "GamePlay/Combat/Skill/Preview/NF_GrenadePreviewActor.h"

#include "Engine/World.h"
#include "GamePlay/Combat/Components/NF_AreaIndicatorComponent.h"
#include "GamePlay/Combat/Projectiles/NF_Grenade.h"
#include "GamePlay/Combat/Projectiles/NF_ProjectileTrajectoryComponent.h"
#include "GamePlay/Combat/Skill/Active/Attack/NFGA_Grenade.h"

ANF_GrenadePreviewActor::ANF_GrenadePreviewActor()
{
	Trajectory = CreateDefaultSubobject<UNF_ProjectileTrajectoryComponent>(TEXT("Trajectory"));
	AreaIndicator = CreateDefaultSubobject<UNF_AreaIndicatorComponent>(TEXT("AreaIndicator"));
}

void ANF_GrenadePreviewActor::BeginPlay()
{
	Super::BeginPlay();
	Trajectory->StartPreview();
	AreaIndicator->AddTickPrerequisiteActor(this);
}

void ANF_GrenadePreviewActor::UpdatePreview_Implementation()
{
	const UNFGA_Grenade* Ability = Cast<UNFGA_Grenade>(GetSourceAbility());
	FVector SpawnLocation;
	FVector Velocity;
	if (!Ability || !Ability->GetGrenadeClass()
		|| !Ability->CalculateThrowParameters(GetSourceAvatar(), SpawnLocation, Velocity))
	{
		AreaIndicator->HideArea();
		return;
	}
	const ANF_Grenade* Defaults = Ability->GetGrenadeClass()->GetDefaultObject<ANF_Grenade>();
	Trajectory->UpdatePreview(SpawnLocation, Velocity, Defaults->GetCollisionRadius(),
		GetWorld()->GetGravityZ() * Defaults->GetProjectileGravityScale(), Defaults->GetFuseTime());
	FHitResult Hit;
	if (Trajectory->GetPredictedImpact(Hit))
	{
		AreaIndicator->ShowArea(Hit.ImpactPoint, Defaults->GetExplosionRadius(), Hit.ImpactNormal);
	}
	else
	{
		AreaIndicator->HideArea();
	}
}

void ANF_GrenadePreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Trajectory->StopPreview();
	AreaIndicator->HideArea();
	Super::EndPlay(EndPlayReason);
}
