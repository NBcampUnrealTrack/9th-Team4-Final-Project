#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_MonsterProjectile.generated.h"

class UGameplayEffect;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class PROJECT_NF_API ANF_MonsterProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_MonsterProjectile();

	void InitializeProjectile(
		float InDamageAmount,
		TSubclassOf<UGameplayEffect> InDamageEffect);
	
protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);
	
	UFUNCTION()
	void HandleProjectileOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ApplyDamageToPlayer(AActor* TargetActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
		Category = "NF|Monster|Projectile",
		meta = (ClampMin = "0.0"))
	float LifeSpan = 5.0f;

private:
	float DamageAmount = 0.0f;

	TSubclassOf<UGameplayEffect> DamageEffect;

};
