#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "NF_BossHomingProjectile.generated.h"

class ANF_Player;
class UStaticMeshComponent;
class UNiagaraSystem;

/** Server-driven projectile. World Y is fixed at launch; only X/Z track the target. */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_BossHomingProjectile : public AActor
{
	GENERATED_BODY()
public:
	ANF_BossHomingProjectile();
	void InitializeProjectile(ANF_Player* Target, float Speed, float TriggerRadius, float BlastRadius,
		float LaunchVelocity, const FGameplayEffectSpecHandle& Damage, const FGameplayEffectSpecHandle& Airborne);
	static FVector CalculateTrackingLocation(const FVector& Current, const FVector& Target, float Speed, float DeltaSeconds);
	static constexpr float TrackingLifetime = 4.0f;
	virtual void Tick(float DeltaSeconds) override;
protected:
	/** Server event: use Spawn Replicated Boss Niagara for custom BP effects. Actor is destroyed after this callback. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|BossProjectile|VFX")
	void OnHomingExplosion(FVector ExplosionLocation, float BlastRadius);
	/** Optional automatic one-shot effect, independent of projectile destruction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|BossProjectile|VFX")
	TObjectPtr<UNiagaraSystem> ExplosionNiagaraSystem;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|BossProjectile|VFX")
	FVector ExplosionNiagaraScale = FVector::OneVector;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|BossProjectile")
	TObjectPtr<UStaticMeshComponent> Mesh;
private:
	void Explode();
	bool IsLivingPlayer(ANF_Player* Player) const;
	TWeakObjectPtr<ANF_Player> TargetPlayer;
	UPROPERTY(Transient)
	FGameplayEffectSpecHandle DamageSpec;
	UPROPERTY(Transient)
	FGameplayEffectSpecHandle AirborneSpec;
	float MoveSpeed = 600.f;
	float DetectionRadius = 120.f;
	float ExplosionRadius = 300.f;
	float AirborneLaunchVelocity = 650.f;
	bool bExploded = false;
	FTimerHandle LifetimeTimer;
};
