#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_Movement.generated.h"

class ACharacter;

UENUM(BlueprintType)
enum class ENF_MovementMethod : uint8 { Dash, Launch };

UENUM(BlueprintType)
enum class ENF_MovementDirection : uint8 { World, Avatar, Aim, Target };

/** Shared Character movement ability. Damage and AI decision making are external. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Movement : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Movement();
	// Set on the granted ability instance on the server before AI activation.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Movement")
	void SetMovementTarget(AActor* Target);
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement")
	ENF_MovementMethod MovementMethod = ENF_MovementMethod::Dash;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement")
	ENF_MovementDirection DirectionSource = ENF_MovementDirection::Aim;
	// Unit direction in the selected frame: X=forward, Y=right, Z=up. Length does not multiply speed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement")
	FVector DirectionAxis = FVector::ForwardVector;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement")
	bool bHorizontalOnly = true;
	// Dash applies this velocity change once. Character mass does not change the skill's strength.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement", meta = (ClampMin = "1.0", Units = "cm/s"))
	float MovementSpeed = 1800.0f;
	// Active skill/contact window only. Travel distance depends on friction and existing velocity.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement", meta = (ClampMin = "0.01", Units = "s", EditCondition = "MovementMethod == ENF_MovementMethod::Dash"))
	float MovementDuration = 0.3f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement", meta = (EditCondition = "MovementMethod == ENF_MovementMethod::Dash"))
	bool bStopOnBlockingHit = true;
	// Optional hard stop (including falling velocity). Off preserves natural inertia at skill end.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement", meta = (EditCondition = "MovementMethod == ENF_MovementMethod::Dash"))
	bool bStopVelocityOnEnd = false;
	// Launch ends the GA immediately; gravity/landing remain CharacterMovement's responsibility.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement", meta = (EditCondition = "MovementMethod == ENF_MovementMethod::Launch"))
	bool bOverrideLaunchXY = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Movement", meta = (EditCondition = "MovementMethod == ENF_MovementMethod::Launch"))
	bool bOverrideLaunchZ = true;
private:
	friend class FNF_MovementAbilityTest;
	bool HasValidMovementSettings() const;
	bool ResolveDirection(ACharacter* Character, const AActor* Target, FVector& OutDirection) const;
	void StartMovement();
	UFUNCTION()
	void FinishMovement();
	UFUNCTION()
	void HandleBlockingHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);
	TWeakObjectPtr<AActor> PendingTarget;
	TWeakObjectPtr<ACharacter> MovingCharacter;
	FVector ActiveDirection = FVector::ZeroVector;
	FTimerHandle MovementCastTimer;
	FTimerHandle MovementEndTimer;
	bool bDashStarted = false;
};
