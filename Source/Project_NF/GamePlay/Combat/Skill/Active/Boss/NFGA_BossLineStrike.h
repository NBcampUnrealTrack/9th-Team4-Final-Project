#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossLineStrike.generated.h"

class ANF_BossLineTelegraph;
class UAnimMontage;
class UNiagaraSystem;

/** Snapshots random target positions and strikes the warned ground rectangles once. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossLineStrike : public UNFGA_BossPatternBase
{
	GENERATED_BODY()
public:
	UNFGA_BossLineStrike();
protected:
	/** Server event after the warning. Parent implementation plays ShotMontage on relevant clients. */
	UFUNCTION(BlueprintNativeEvent, Category="NF|Boss|Line Strike|Presentation")
	void PlayStrikeMontage();
	virtual void PlayStrikeMontage_Implementation();

	/** Server event at impact. Parent implementation replicates a burst at LaunchEffectSocket.
	 * This is cosmetic only: BP delays here do not delay damage. Use StrikeDelay for timing.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="NF|Boss|Line Strike|Presentation")
	void PlayStrikeNiagara();
	virtual void PlayStrikeNiagara_Implementation();

	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Line Strike", meta=(ClampMin="1"))
	float TargetRange = 3000.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Line Strike", meta=(ClampMin="1", ClampMax="3"))
	int32 MaxTargets = 3;
	/** Full width in cm. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Line Strike", meta=(ClampMin="1"))
	float StrikeWidth = 200.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Line Strike", meta=(ClampMin="1"))
	float StrikeHalfHeight = 200.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Line Strike", meta=(ClampMin="0.01"))
	float WarningDuration = 2.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Line Strike", meta=(ClampMin="0"))
	float Damage = 40.f;
	/** Use a full-rectangle deferred decal material, without a circular opacity mask. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Line Strike")
	TObjectPtr<UMaterialInterface> WarningMaterial;

	/** Starts after WarningDuration. The warning remains visible until the strike. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch Animation")
	TObjectPtr<UAnimMontage> ShotMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch Animation", meta=(ClampMin="0.01"))
	float ShotMontagePlayRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch Animation")
	FName ShotMontageStartSection;
	/** Game seconds from montage start to socket VFX and damage; independent of montage play rate. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch Animation", meta=(ClampMin="0.0", Units="s"))
	float StrikeDelay = 0.f;

	/** Non-looping burst at the socket's world transform at strike time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch VFX")
	TObjectPtr<UNiagaraSystem> LaunchNiagaraSystem;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch VFX")
	FName LaunchEffectSocket;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch VFX")
	FVector LaunchEffectLocalOffset = FVector::ZeroVector;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch VFX")
	FRotator LaunchEffectLocalRotation = FRotator::ZeroRotator;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="NF|Boss|Launch VFX")
	FVector LaunchNiagaraScale = FVector::OneVector;
private:
	void BeginStrikeWindup();
	void Strike();
	TArray<FTransform> StrikeTransforms;
	TArray<FVector> StrikeExtents;
	UPROPERTY(Transient) TArray<TObjectPtr<ANF_BossLineTelegraph>> Warnings;
	FTimerHandle WarningTimer;
	FTimerHandle StrikeTimer;
};
