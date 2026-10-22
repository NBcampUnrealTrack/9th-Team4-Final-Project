#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_AccelerationEffect.generated.h"

class ACharacter;
class UCharacterMovementComponent;

// @Owner: 이정준 (구현: Codex) | 2026-10-20
// Standard CharacterMovement speed settings are not replicated by the engine.
// This skill-owned actor applies/removes the factor on server and clients.
UCLASS(NotBlueprintable)
class PROJECT_NF_API ANF_AccelerationEffect : public AActor
{
	GENERATED_BODY()
public:
	ANF_AccelerationEffect();
	bool Initialize(ACharacter* Character, float Factor);
	void StopEffect();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(ReplicatedUsing = OnRep_Effect)
	TObjectPtr<ACharacter> Target;
	UPROPERTY(ReplicatedUsing = OnRep_Effect)
	float Multiplier = 1.0f;
	UPROPERTY(ReplicatedUsing = OnRep_Effect)
	bool bActive = false;
	UFUNCTION()
	void OnRep_Effect();
	bool ApplyLocal();
	void RemoveLocal();
	TWeakObjectPtr<UCharacterMovementComponent> AppliedMovement;
	float AppliedMultiplier = 1.0f;
};
