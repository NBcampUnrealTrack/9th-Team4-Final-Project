#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NFGC_WeaponFire.generated.h"

/** 자식 GC BP에서 GameplayCue.Weapon.Fire 태그를 지정한다. 실행별 상태를 저장하지 않는다. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGC_WeaponFire : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	UNFGC_WeaponFire(const FObjectInitializer& ObjectInitializer);
	virtual bool HandlesEvent(EGameplayCueEvent::Type EventType) const override;
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
