#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "NF_CharacterPullActor.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UGameplayEffect;

UENUM(BlueprintType)
enum class ENF_CharacterPullResult : uint8
{
	Reached, InvalidTarget, Blocked, ImmuneOrInterrupted, TimedOut, Cancelled, EffectRejected, MovementRejected
};

USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_CharacterPullSettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pull", meta = (ClampMin = "1", Units = "cm/s"))
	float Speed = 1500.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pull", meta = (ClampMin = "0.05", Units = "s"))
	float MaxDuration = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pull", meta = (ClampMin = "1", Units = "cm"))
	float ArrivalTolerance = 15.0f;
	// Optional duration/infinite GE. Its exact handle is removed when this pull ends.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pull")
	TSubclassOf<UGameplayEffect> StunEffectClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pull")
	FGameplayTagContainer ImmunityTags;
	// Death/interruption tags only. Do not include the stun granted by this pull.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pull")
	FGameplayTagContainer InterruptTags;
	bool IsValid() const;
};

USTRUCT()
struct FNF_CharacterPullMotion
{
	GENERATED_BODY()
	UPROPERTY()
	TObjectPtr<ACharacter> Target;
	UPROPERTY()
	FVector_NetQuantize Destination = FVector::ZeroVector;
	UPROPERTY()
	FVector_NetQuantize StartLocation = FVector::ZeroVector;
	UPROPERTY()
	FName MotionName;
	UPROPERTY()
	float Duration = 0.0f;
	UPROPERTY()
	float ActiveDuration = 0.0f;
	UPROPERTY()
	float StartServerTime = 0.0f;
	UPROPERTY()
	bool bActive = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_CharacterPullEnded, bool);

/** Server controls the pull. Only the target's owner receives the movement command.
 * Simulated proxies use normal Character movement replication. No Character/Controller edits required. */
UCLASS(NotBlueprintable, Transient)
class PROJECT_NF_API ANF_CharacterPullActor : public AActor
{
	GENERATED_BODY()
public:
	ANF_CharacterPullActor();
	bool StartPull(AActor* Source, ACharacter* Target, FVector Destination, const FNF_CharacterPullSettings& Settings,
		UAbilitySystemComponent* SourceASC, float AbilityLevel);
	void StopPull(bool bReached = false, ENF_CharacterPullResult Reason = ENF_CharacterPullResult::Cancelled);
	ENF_CharacterPullResult GetResult() const { return Result; }
	static bool CanPull(ACharacter* Target, const FNF_CharacterPullSettings& Settings);
	FNF_CharacterPullEnded OnPullEnded;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	friend class FNF_GrabTest;
	UFUNCTION()
	void OnRep_Motion();
	UFUNCTION()
	void HandleHit(AActor* SelfActor, AActor* OtherActor, FVector Impulse, const FHitResult& Hit);
	void ApplyMotion(float Duration, float Elapsed = 0.0f);
	void RemoveMotion();
	UPROPERTY(ReplicatedUsing = OnRep_Motion)
	FNF_CharacterPullMotion Motion;
	TWeakObjectPtr<AActor> SourceActor;
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	FActiveGameplayEffectHandle StunHandle;
	FNF_CharacterPullSettings PullSettings;
	uint16 MotionId = MAX_uint16;
	double EndsAt = 0.0;
	bool bStarted = false;
	bool bStopping = false;
	ENF_CharacterPullResult Result = ENF_CharacterPullResult::Cancelled;
};
