#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_WeaponProjectile.generated.h"

UCLASS()
class PROJECT_NF_API ANF_WeaponProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_WeaponProjectile();

	void InitializeProjectile(float InSpeed);
	void IgnoreProjectile(ANF_WeaponProjectile* OtherProjectile);
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	TObjectPtr<class USphereComponent> SphereComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	TObjectPtr<class UStaticMeshComponent> StaticMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovementComponent;

	UFUNCTION()
	void OnProjectileHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);
};
