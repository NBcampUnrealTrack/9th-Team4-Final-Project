#include "GAS/Cue/NFGC_WeaponProjectileHit.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "GamePlay/Combat/Projectiles/NF_WeaponProjectileHitType.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"

UNFGC_WeaponProjectileHit::UNFGC_WeaponProjectileHit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UNFGC_WeaponProjectileHit::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return EventType == EGameplayCueEvent::Executed;
}

bool UNFGC_WeaponProjectileHit::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	UWorld* World = IsValid(MyTarget) ? MyTarget->GetWorld() : nullptr;
	if (!IsValid(World) || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const UNF_WeaponDataAsset* Data = Cast<UNF_WeaponDataAsset>(Parameters.SourceObject.Get());
	if (!IsValid(Data))
	{
		return false;
	}

	// 분류는 서버에서 확정한다. 알 수 없는 값은 환경 기본 효과로 처리한다.
	UParticleSystem* Particle = nullptr;
	if (Parameters.RawMagnitude == static_cast<float>(ENF_WeaponProjectileHitType::Boss))
	{
		Particle = Data->BossImpactParticle;
	}
	else if (Parameters.RawMagnitude == static_cast<float>(ENF_WeaponProjectileHitType::Monster))
	{
		Particle = Data->MonsterImpactParticle;
	}
	else if (Parameters.RawMagnitude == static_cast<float>(ENF_WeaponProjectileHitType::Player))
	{
		Particle = Data->PlayerImpactParticle;
	}
	if (!IsValid(Particle)) Particle = Data->DefaultImpactParticle;
	if (!IsValid(Particle)) return false;

	// 파티클의 +X 축을 표면 법선에 맞춘다. 투사체 제거 후에도 독립적으로 재생한다.
	UGameplayStatics::SpawnEmitterAtLocation(World, Particle, Parameters.Location,
		Parameters.Normal.Rotation(), FVector::OneVector, true);
	return true;
}
