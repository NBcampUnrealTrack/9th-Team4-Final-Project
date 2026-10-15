#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_ProjectileLaunchComponent.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 조합 투사체의 발사 계산/충돌 루트/서버 이동을 관리한다. 피해/폭발은 담당하지 않는다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_ProjectileLaunchComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_ProjectileLaunchComponent();
	bool HasValidSettings() const;
	bool ConfigureComponents();
	bool CalculateLaunch(AActor* Avatar, FVector AimDirection, float CollisionRadius, float MaxSpeed,
		FVector& Location, FVector& Velocity) const;
	bool PrepareLaunch(AActor* Avatar, FVector AimDirection);
	bool Launch(AActor* Avatar);
	void StopProjectile();
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Projectile Launch", meta = (ClampMin = "1.0", Units = "cm/s"))
	float ThrowSpeed = 1200.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Projectile Launch", meta = (ClampMin = "-80.0", ClampMax = "80.0"))
	float ThrowAngleOffset = 15.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Projectile Launch", meta = (ClampMin = "1.0", Units = "cm"))
	float AimDistance = 10000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Projectile Launch")
	FVector SpawnOffset = FVector(60.0f, 0.0f, -20.0f);
	// 기본적으로 월드/캐릭터를 Block하고 시전자만 이동 충돌에서 제외한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Projectile Launch")
	bool bConfigureCollision = true;
private:
	TWeakObjectPtr<USphereComponent> Collision;
	TWeakObjectPtr<UProjectileMovementComponent> Movement;
	FVector InitialVelocity = FVector::ZeroVector;
	bool bPrepared = false;
};
