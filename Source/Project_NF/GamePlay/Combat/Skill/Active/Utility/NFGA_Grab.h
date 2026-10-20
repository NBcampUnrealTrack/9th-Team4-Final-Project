#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "GamePlay/Combat/Components/NF_TargetQueryTypes.h"
#include "GamePlay/Combat/Skill/Execution/NF_CharacterPullActor.h"
#include "NFGA_Grab.generated.h"

class UNFAbilityTask_PullCharacter;
class ANF_GrabHook;

UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Grab : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Grab();
	// Server AI sets this on the granted ability instance before activation; events can pass Target instead.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Grab")
	void SetGrabTarget(AActor* Target);
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab", meta = (ClampMin = "1", Units = "cm"))
	float GrabRange = 600.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab", meta = (ClampMin = "1", Units = "cm"))
	float GrabRadius = 30.0f;
	// Center distance; automatically enlarged to fit both capsules.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab", meta = (ClampMin = "1", Units = "cm"))
	float StopDistance = 120.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab|Hook")
	TSubclassOf<ANF_GrabHook> HookClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab")
	FNF_CharacterPullSettings PullSettings;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab")
	FNF_TargetFilterSettings TargetFilter;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab")
	TEnumAsByte<ECollisionChannel> ObstacleChannel = ECC_Visibility;
private:
	friend class FNF_GrabTest;
	friend class FNF_GrabHookTest;
	bool HasValidSettings() const;
	bool ValidateTarget(ACharacter* Target, bool bCheckInitialAim = true) const;
	bool CalculateDestination(ACharacter* Target, FVector& OutDestination) const;
	void StartGrab();
	bool HandleHookHit(ACharacter* Target);
	bool CanContinueHook() const;
	void HandleHookFlightEnded();
	UFUNCTION()
	void HandlePullFinished(bool bReachedDestination);
	TWeakObjectPtr<AActor> PendingTarget;
	bool bHasPendingTarget = false;
	bool bUseExplicitAim = false;
	TWeakObjectPtr<ACharacter> ActiveTarget;
	TWeakObjectPtr<AActor> OriginalAvatar;
	FTimerHandle GrabCastTimer;
	UPROPERTY(Transient)
	TObjectPtr<UNFAbilityTask_PullCharacter> PullTask;
	UPROPERTY(Transient)
	TObjectPtr<ANF_GrabHook> ActiveHook;
};
