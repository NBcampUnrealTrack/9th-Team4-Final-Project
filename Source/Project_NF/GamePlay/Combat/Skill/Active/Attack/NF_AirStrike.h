#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "NF_AirStrike.generated.h"

class UStaticMeshComponent;
class UNF_AreaIndicatorComponent;
class ANF_AirStrikeBomb;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 서버가 비행 경로/투하 위치를 결정하고 복제 액터로 공습 연출을 진행한다.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_AirStrike : public AActor
{
	GENERATED_BODY()
public:
	ANF_AirStrike();
	void InitializeStrike(FVector Center, FVector Direction, const FGameplayEffectSpecHandle& InDamageSpec);
	float GetStrikeRadius() const { return FMath::Max(StrikeRadius, 1.0f); }
	bool IsStrikeConfigured() const;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|AirStrike")
	TObjectPtr<UStaticMeshComponent> PlaneBody;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|AirStrike")
	TObjectPtr<UStaticMeshComponent> PlaneWings;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|AirStrike")
	TObjectPtr<UNF_AreaIndicatorComponent> WarningArea;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "1.0", Units = "cm"))
	float StrikeRadius = 1000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "0.0", Units = "s"))
	float WarningDelay = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "100.0", Units = "cm"))
	float FlightAltitude = 2200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "0.0", Units = "cm"))
	float ApproachDistance = 1000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "100.0", Units = "cm/s"))
	float FlightSpeed = 1200.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "1", ClampMax = "128"))
	int32 BombCount = 24;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike")
	TSubclassOf<ANF_AirStrikeBomb> BombClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "1.0", Units = "cm/s"))
	float InitialFallSpeed = 400.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike", meta = (ClampMin = "100.0", Units = "cm"))
	float GroundSearchDepth = 5000.0f;

private:
	void StartFlight();
	void DropBomb(FVector Target);
	UFUNCTION()
	void RefreshVisuals();
	UPROPERTY(ReplicatedUsing = RefreshVisuals)
	FVector StrikeCenter = FVector::ZeroVector;
	UPROPERTY(ReplicatedUsing = RefreshVisuals)
	float ReplicatedRadius = 1000.0f;
	UPROPERTY(ReplicatedUsing = RefreshVisuals)
	bool bFlightVisible = false;
	UPROPERTY(Transient)
	FGameplayEffectSpecHandle DamageSpec;
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<ANF_AirStrikeBomb>> DroppedBombs;

	FVector FlightDirection = FVector::ForwardVector;
	TArray<FVector> BombTargets;
	FTimerHandle StartTimer;
	float Travel = 0.0f;
	float MaxFallTime = 0.0f;
	int32 NextBomb = 0;
	bool bFlightActive = false;
};
