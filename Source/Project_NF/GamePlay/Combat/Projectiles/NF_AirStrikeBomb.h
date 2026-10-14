#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "NF_AirStrikeBomb.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;
class UNF_AreaDamageComponent;
class UPrimitiveComponent;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 서버에서 낙하하고 첫 충돌에 폭발하는 공습 폭탄. 공통 광역 피해 컴포넌트를 사용한다.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_AirStrikeBomb : public AActor
{
	GENERATED_BODY()
public:
	ANF_AirStrikeBomb();
	void InitializeBomb(FVector InitialVelocity, const FGameplayEffectSpecHandle& InDamageSpec);
	void IgnoreOtherBomb(AActor* OtherBomb);
	float GetGravityScale() const;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, Category = "NF|AirStrike Bomb")
	bool CanAffectTarget(AActor* Target) const;
	virtual bool CanAffectTarget_Implementation(AActor* Target) const;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "NF|AirStrike Bomb")
	void OnExplosionCosmetic(FVector Location);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|AirStrike Bomb")
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|AirStrike Bomb")
	TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|AirStrike Bomb")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|AirStrike Bomb")
	TObjectPtr<UNF_AreaDamageComponent> AreaDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike Bomb", meta = (ClampMin = "1.0", Units = "cm"))
	float ExplosionRadius = 250.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike Bomb", meta = (ClampMin = "1.0", Units = "s"))
	float MaxLifetime = 12.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|AirStrike Bomb|Debug", meta = (ClampMin = "0.0"))
	float ExplosionDebugDuration = 2.0f;

private:
	UFUNCTION()
	void OnBombHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	bool FilterDamageTarget(AActor* Target);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplosion(FVector_NetQuantize Location, float Radius, float DebugDuration);

	UPROPERTY(Transient)
	FGameplayEffectSpecHandle DamageSpec;
	bool bExploded = false;
};
