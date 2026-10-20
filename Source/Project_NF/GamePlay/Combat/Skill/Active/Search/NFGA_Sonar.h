#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "GamePlay/Combat/Components/NF_TargetQueryTypes.h"
#include "UI/Sonar/NF_SilhouetteView.h"
#include "NFGA_Sonar.generated.h"

class ANF_SonarReveal;

/** One pulse: snapshot targets at cast completion, then reveal their moving silhouettes for ExecutionDuration. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Sonar : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Sonar();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Sonar", meta = (ClampMin = "1", Units = "cm"))
	float DetectionRadius = 2000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Sonar")
	FNF_TargetFilterSettings TargetFilter;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Sonar|Silhouette")
	TObjectPtr<UMaterialInterface> SilhouetteMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Sonar|Silhouette")
	FNF_SilhouetteStyle SilhouetteStyle;
private:
	bool HasValidSettings() const;
	void EmitPulse();
	void CheckLifetime();
	TWeakObjectPtr<AActor> OriginalAvatar;
	UPROPERTY(Transient)
	TObjectPtr<ANF_SonarReveal> Reveal;
	FTimerHandle CastTimer;
	FTimerHandle LifetimeTimer;
	double EndsAt = 0.0;
};
