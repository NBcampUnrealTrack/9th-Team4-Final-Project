#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_ProjectileBase.generated.h"

UCLASS()
class PROJECT_NF_API ANF_ProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_ProjectileBase();

	void InitializeProjectile(float InSpeed);
	
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
