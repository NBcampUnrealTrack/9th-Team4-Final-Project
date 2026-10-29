#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NFGC_WeaponProjectileHit.generated.h"

/** 자식 BP에 GameplayCue.Weapon.ProjectileHit를 지정한다. 투사체/명중 대상의 생존에 의존하지 않는다. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGC_WeaponProjectileHit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UNFGC_WeaponProjectileHit(const FObjectInitializer& ObjectInitializer);
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
