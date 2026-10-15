#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "NF_DetonationComponent.generated.h"

class UPrimitiveComponent;
class UParticleSystem;
UENUM(BlueprintType)
enum class ENF_DetonationMode : uint8 { Fuse, Impact, FuseOrImpact };
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_Detonated);

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 서버의 시간/충돌 조건을 단일 폭발 요청으로 바꾼다. 피해 적용은 공통 실행 액터가 담당한다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_DetonationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_DetonationComponent();
	bool HasValidSettings() const;
	float GetPreviewTime() const;
	bool Arm(UPrimitiveComponent* Collision);
	void Disarm();
	void PlayCosmetic(FVector Location, float Radius);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Detonation")
	void Detonate();
	UPROPERTY(BlueprintAssignable, Category = "NF|Detonation")
	FNF_Detonated OnDetonated;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Detonation")
	ENF_DetonationMode Mode = ENF_DetonationMode::Fuse;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Detonation", meta = (ClampMin = "0.01", Units = "s"))
	float FuseTime = 3.0f;
	// 충돌하지 않는 Impact 투사체도 무한히 남지 않게 시간 만료 시 폭발한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Detonation", meta = (ClampMin = "0.01", Units = "s"))
	float MaxLifetime = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Detonation|Visual")
	TObjectPtr<UParticleSystem> ExplosionEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Detonation|Visual")
	bool bDrawDebugRadius = true;
private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);
	TWeakObjectPtr<UPrimitiveComponent> BoundCollision;
	FTimerHandle DetonationTimer;
	bool bArmed = false;
	bool bDetonated = false;
};
