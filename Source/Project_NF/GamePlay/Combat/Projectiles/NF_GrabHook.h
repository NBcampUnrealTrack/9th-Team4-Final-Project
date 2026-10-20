#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "NF_GrabHook.generated.h"

class ACharacter;
class USphereComponent;
class ANF_GrabHookVisual;
class UProjectileMovementComponent;
class UPrimitiveComponent;

UENUM(BlueprintType)
enum class ENF_GrabHookPhase : uint8 { Flying, Attached, Returning, Finished };

USTRUCT()
struct FNF_GrabHookFrame
{
	GENERATED_BODY()
	UPROPERTY()
	TObjectPtr<AActor> Source;
	UPROPERTY()
	TObjectPtr<ACharacter> Target;
	UPROPERTY()
	FVector_NetQuantize TargetLocalOffset = FVector::ZeroVector;
	UPROPERTY()
	ENF_GrabHookPhase Phase = ENF_GrabHookPhase::Finished;
};

DECLARE_DELEGATE_RetVal_OneParam(bool, FNF_GrabHookHit, ACharacter*);
DECLARE_DELEGATE_RetVal(bool, FNF_GrabHookCanContinue);
DECLARE_MULTICAST_DELEGATE(FNF_GrabHookFlightEnded);

/** Authoritative collision/state only. VisualClass is a separate, local presentation actor. */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_GrabHook : public AActor
{
	GENERATED_BODY()
public:
	ANF_GrabHook();
	bool StartFlight(AActor* Source, FVector Direction, float Range, float Radius, float MaximumHoldTime);
	FVector GetSourceAnchor(AActor* Source) const;
	void Latch(ACharacter* Target);
	void BeginReturn();
	void Abort();
	bool HasValidSettings() const;
	UFUNCTION(BlueprintPure, Category = "NF|Grab Hook")
	ENF_GrabHookPhase GetHookPhase() const { return Frame.Phase; }
	FNF_GrabHookHit OnCharacterHit;
	FNF_GrabHookCanContinue CanContinue;
	FNF_GrabHookFlightEnded OnFlightEnded;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Grab Hook")
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Grab Hook")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Hook|Presentation")
	TSubclassOf<ANF_GrabHookVisual> VisualClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Hook", meta = (ClampMin = "1", Units = "cm/s"))
	float FireSpeed = 2000.0f;
	// Stable gameplay origin; cosmetic mesh/socket pose does not affect collision.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Hook|Anchor")
	FVector FallbackOffset = FVector(40, 20, -20);
private:
	friend class FNF_GrabHookTest;
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void HandleProjectileStopped(const FHitResult& Hit);
	UFUNCTION()
	void OnRep_Frame();
	void UpdateVisuals();
	void FinishReturn();
	UPROPERTY(ReplicatedUsing = OnRep_Frame)
	FNF_GrabHookFrame Frame;
	FVector FlightOrigin = FVector::ZeroVector;
	FVector FlightDirection = FVector::ForwardVector;
	float FlightRange = 0.0f;
	double Deadline = 0.0;
	bool bLaunched = false;
	TWeakObjectPtr<ACharacter> PendingHitTarget;
	FVector PendingHitLocalOffset = FVector::ZeroVector;
	bool bPendingCharacterHit = false;
	TWeakObjectPtr<ANF_GrabHookVisual> VisualActor;
	bool bVisualSpawnAttempted = false;
	bool bFlightEndedNotified = false;
};
