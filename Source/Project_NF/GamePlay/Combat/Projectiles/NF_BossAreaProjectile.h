#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "NF_BossAreaProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/** Replicated linear visual projectile; damage is resolved by the server at its saved destination. */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_BossAreaProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANF_BossAreaProjectile();
	void InitializeProjectile(const FVector& InDestination, float InDamageRadius, float InSpeed,
		const FGameplayEffectSpecHandle& InDamageSpec, FGameplayTag InImpactCueTag);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|BossProjectile|Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|BossProjectile|Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|BossProjectile|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	void ReachDestination();
	void ApplyImpactDamage();

	UPROPERTY(Replicated)
	FVector_NetQuantize Destination;

	UPROPERTY(Replicated)
	float DamageRadius = 350.0f;

	UPROPERTY(Transient)
	FGameplayEffectSpecHandle DamageSpec;

	FGameplayTag ImpactCueTag;

	FTimerHandle ArrivalTimer;
};
