#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Weapons/NF_WeaponTypes.h"
#include "GAS/Abilities/NFGA_MultiPurposeBase.h"
#include "NFGA_MeleeAttack.generated.h"


enum class ENF_WeaponType : uint8;

UCLASS()
class PROJECT_NF_API UNFGA_MeleeAttack : public UNFGA_MultiPurposeBase
{
	GENERATED_BODY()

public:
	UNFGA_MeleeAttack();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
private:
	ENF_WeaponType EquippedWeaponType = ENF_WeaponType::None;
	FName EquippedWeaponSocketName = NAME_None;
};
