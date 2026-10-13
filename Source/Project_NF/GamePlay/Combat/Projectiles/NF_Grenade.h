#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "NF_Grenade.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-12
 * 목적: 서버 권한으로 곡사 이동, 바운스, 지연 폭발과 범위 GE 적용을 처리한다.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_Grenade : public AActor
{
	GENERATED_BODY()

public:
	ANF_Grenade();

	// Deferred spawn의 FinishSpawning 전에 서버에서 호출한다.
	void InitializeGrenade(const FVector& InitialVelocity, const FGameplayEffectSpecHandle& InDamageSpec);
	float GetCollisionRadius() const;
	float GetProjectileGravityScale() const;
	float GetFuseTime() const { return FMath::Max(FuseTime, 0.1f); }
	float GetExplosionRadius() const { return FMath::Max(ExplosionRadius, 1.0f); }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 기본값: ASC가 있는 대상 중 자기 자신 제외. 팀 규칙은 자식 BP에서 확장한다.
	UFUNCTION(BlueprintNativeEvent, Category = "NF|Grenade")
	bool CanAffectTarget(AActor* Target) const;
	virtual bool CanAffectTarget_Implementation(AActor* Target) const;

	// 로컬 연출만 구현한다. 서버 피해 처리는 이 이벤트에 넣지 않는다.
	// 수류탄은 직후 제거되므로 이 위치에 독립적인 Niagara/사운드를 생성한다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "NF|Grenade")
	void OnExplosionCosmetic(FVector ExplosionLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Grenade|Components")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Grenade|Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Grenade|Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Explosion", meta = (ClampMin = "0.1", Units = "s"))
	float FuseTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Explosion", meta = (ClampMin = "1.0", Units = "cm"))
	float ExplosionRadius = 400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Explosion")
	bool bRequireLineOfSight = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grenade|Explosion")
	TEnumAsByte<ECollisionChannel> OcclusionTraceChannel = ECC_Visibility;

private:
	void Explode();
	void ApplyExplosionDamage(const FVector& ExplosionLocation);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplosion(FVector_NetQuantize ExplosionLocation);

	UPROPERTY(Transient)
	FGameplayEffectSpecHandle DamageSpec;

	FTimerHandle FuseTimer;
	bool bExploded = false;
};
