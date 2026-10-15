#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GamePlay/Combat/Components/NF_TargetQueryTypes.h"
#include "NF_ApplyEffectComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
UENUM(BlueprintType)
enum class ENF_EffectTargetMode : uint8 { Self, Query, Hit };
UENUM(BlueprintType)
enum class ENF_EffectRemovalPolicy : uint8 { Independent, OnExecutionEnd };
USTRUCT(BlueprintType)
struct PROJECT_NF_API FNF_EffectApplication
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TSubclassOf<UGameplayEffect> EffectClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TMap<FGameplayTag, float> SetByCaller;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	ENF_EffectRemovalPolicy Removal = ENF_EffectRemovalPolicy::Independent;
};

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 서버 GE 적용과 자신이 소유한 효과의 종료 정리를 담당한다. 중첩/도트 자체는 GE가 관리한다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_ApplyEffectComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_ApplyEffectComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Apply Effect")
	ENF_EffectTargetMode TargetMode = ENF_EffectTargetMode::Query;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Apply Effect")
	TArray<FNF_EffectApplication> Effects;
	bool HasValidSettings() const;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Apply Effect")
	bool PrepareEffects(AActor* SourceAvatar, float Level = 1.0f);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Apply Effect")
	int32 ApplyToSelf();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Apply Effect")
	int32 ApplyToTarget(AActor* Target, const FHitResult& Hit, FVector Origin);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Apply Effect")
	int32 ApplyToTargets(const TArray<FNF_TargetQueryResult>& Targets, FVector Origin);
	// 외부 GA가 만든 Spec도 사용 가능. 반환값은 GE 제출 여부이며 실제 피해/면역 여부와 다르다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Apply Effect")
	bool ApplySpecToTarget(AActor* Target, const FGameplayEffectSpecHandle& Spec, FVector Origin,
		const FHitResult& Hit, bool bRemoveOnStop = false);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Apply Effect")
	void StopEffects();
	static bool SubmitSpec(AActor* AuthorityActor, AActor* Target, const FGameplayEffectSpecHandle& Spec,
		FVector Origin, const FHitResult& Hit, FActiveGameplayEffectHandle* OutHandle = nullptr);
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	struct FTrackedEffect
	{
		TWeakObjectPtr<UAbilitySystemComponent> ASC;
		FActiveGameplayEffectHandle Handle;
		const UGameplayEffect* Definition = nullptr;
		int32 EntryIndex = INDEX_NONE;
	};
	bool ApplyInternal(AActor* Target, const FGameplayEffectSpecHandle& Spec, FVector Origin,
		const FHitResult& Hit, bool bRemoveOnStop, int32 EntryIndex);
	UPROPERTY(Transient)
	TArray<FGameplayEffectSpecHandle> PreparedSpecs;
	TArray<bool> PreparedRemoval;
	TWeakObjectPtr<AActor> Source;
	TArray<FTrackedEffect> Tracked;
	uint32 Generation = 0;
	bool bStopped = false;
	bool bEndingPlay = false;
};
