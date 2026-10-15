#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePlay/Combat/Components/NF_TargetQueryTypes.h"
#include "NF_HitEffectComponent.generated.h"

class UPrimitiveComponent;
class UAbilitySystemComponent;
class UNF_ApplyEffectComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNF_HitEffectsSubmitted, AActor*, Target, int32, EffectCount);

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 서버의 충돌/명중을 GE 적용으로 전달한다. 폭발 범위 피해와 독립적이다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_HitEffectComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_HitEffectComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Hit Effect")
	FNF_TargetFilterSettings Filter;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Hit Effect")
	bool bOncePerTarget = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Hit Effect", meta = (ClampMin = "0.0"))
	float ReapplyInterval = 0.1f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Hit Effect")
	bool bListenToOverlaps = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Hit Effect")
	bool bEndExecutionAfterHit = true;
	UPROPERTY(BlueprintAssignable, Category = "NF|Hit Effect")
	FNF_HitEffectsSubmitted OnHitEffectsSubmitted;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Hit Effect")
	bool StartListening(AActor* SourceAvatar);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Hit Effect")
	void StopListening();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Hit Effect")
	int32 SubmitHit(AActor* Target, const FHitResult& Hit);
	bool HasValidSettings() const;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* Component, AActor* OtherActor, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);
	TWeakObjectPtr<UNF_ApplyEffectComponent> ApplyEffect;
	TWeakObjectPtr<UPrimitiveComponent> Collision;
	TWeakObjectPtr<AActor> Source;
	TMap<TWeakObjectPtr<UAbilitySystemComponent>, double> LastSubmission;
	uint32 Generation = 0;
	bool bListening = false;
	bool bEndingPlay = false;
};
