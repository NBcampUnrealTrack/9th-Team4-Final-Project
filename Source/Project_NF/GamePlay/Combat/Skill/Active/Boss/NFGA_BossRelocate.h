#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossPatternBase.h"
#include "NFGA_BossRelocate.generated.h"

class ANF_BossAreaProjectile;
class UGameplayEffect;

/** Takes off, remains airborne while telegraphing a landing point, then dives and launches nearby players. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_BossRelocate : public UNFGA_BossPatternBase
{
	GENERATED_BODY()

public:
	UNFGA_BossRelocate();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (DisplayName = "Takeoff Warning Duration", ClampMin = "0.0", Units = "s"))
	float DirtCueDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (DisplayName = "Ascent Duration", ClampMin = "0.0", Units = "s"))
	float AscentDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (DisplayName = "Flight Altitude", ClampMin = "0.0", Units = "cm"))
	float FlightAltitude = 1800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (DisplayName = "Time Aloft Before Landing Warning", ClampMin = "0.0", Units = "s"))
	float HiddenDurationBeforeWarning = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (DisplayName = "Landing Warning Duration", ClampMin = "0.0", Units = "s"))
	float EmergenceWarningDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern", meta = (DisplayName = "Dive Duration", ClampMin = "0.0", Units = "s"))
	float DiveDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float RelocationRange = 2500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Pattern")
	float EmergenceRadius = 750.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	FGameplayTag DamageMagnitudeTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	float Damage = 40.0f;

	/** Optional override. Empty uses BossAirborneEffectClass from DA_NF_BossGE (GE Manager). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage")
	TSubclassOf<UGameplayEffect> AirborneEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Boss|Damage", meta = (ClampMin = "0.0"))
	float AirborneLaunchVelocity = 900.0f;

private:
	enum class EMovementPhase : uint8
	{
		None,
		TakingOff,
		Diving,
		ReturningToHover
	};

	void BeginTakeoff();
	void BeginDive();
	void ShowEmergenceWarning();
	void Emerge();
	void FinishPattern();
	void StartBossMovement(const FVector& Destination, float Duration, EMovementPhase Phase);
	void UpdateBossMovement();
	void FinishBossMovement();
	bool ChooseEmergencePoint(FVector& OutPoint) const;
	void ExecuteBossCue(const FGameplayTag CueTag, const FVector& Location) const;

	FVector EmergencePoint = FVector::ZeroVector;
	FVector StartingLocation = FVector::ZeroVector;
	float StartingHoverHeight = 0.0f;
	FVector MovementStart = FVector::ZeroVector;
	FVector MovementDestination = FVector::ZeroVector;
	float MovementStartTime = 0.0f;
	float CurrentMovementDuration = 0.0f;
	EMovementPhase MovementPhase = EMovementPhase::None;
	bool bIsInFlight = false;
	FTimerHandle TakeoffTimer;
	FTimerHandle LandingWarningTimer;
	FTimerHandle DiveTimer;
	FTimerHandle FinishTimer;
	FTimerHandle MovementTimer;
};
