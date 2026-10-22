#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GamePlay/Combat/Components/NF_DeployableComponent.h"
#include "GamePlay/Combat/Components/NF_RepeatApplicationComponent.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewData.h"
#include "NF_SkillExecutionActor.generated.h"

class UNFGA_SkillBase;
class UGameplayEffect;

USTRUCT()
struct FNF_SkillExecutionFrame
{
	GENERATED_BODY()
	UPROPERTY()
	bool bActive = false;
	UPROPERTY()
	bool bHasTarget = false;
	UPROPERTY()
	FVector Start = FVector::ZeroVector;
	UPROPERTY()
	FVector End = FVector::ZeroVector;
	UPROPERTY()
	float DisplayRadius = 0.0f;
};
DECLARE_MULTICAST_DELEGATE(FNF_SkillExecutionEnded);

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: BP에 부착한 타기팅/반복/피해/설치/체력 컴포넌트를 그래프 없이 연결한다.
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_SkillExecutionActor : public AActor
{
	GENERATED_BODY()
public:
	ANF_SkillExecutionActor();
	void InitializeExecution(AActor* Avatar, UNFGA_SkillBase* Ability);
	bool ValidateComposition() const;
	bool PrepareExecution();
	bool CanRunIndependently() const;
	// Started projectiles and stationary deployments survive caster EMP; maintained beams/followers do not.
	bool SurvivesCasterEMP() const;
	bool HasProjectileLaunch() const;
	bool HasGroundTravel() const;
	bool GetProjectilePreviewData(AActor* Avatar, FNF_SkillPreviewData& Data) const;
	bool ValidateProjectileDirection(AActor* Avatar, FVector Direction) const;
	bool StartExecution();
	void StopExecution();
	FNF_SkillExecutionEnded OnExecutionEnded;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	// 범위 피해 컴포넌트가 있을 때만 필요하다. 벽처럼 공격하지 않는 설치물은 비워둔다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill Execution|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill Execution|Damage")
	FGameplayTag DamageMagnitudeTag;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill Execution|Damage", meta = (ClampMin = "0.0"))
	float DamageMagnitude = 50.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Skill Execution")
	bool bFollowSource = false;
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill Execution")
	void ExecutionStarted();
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill Execution")
	void ExecutionStopped();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "NF|Skill Execution")
	void DetonationCosmetic(FVector Location, float Radius);
private:
	friend class FNF_EMPTest;
	UFUNCTION()
	void HandleHitEffectsSubmitted(AActor* Target, int32 EffectCount);
	UFUNCTION()
	void HandleDetonated();
	UFUNCTION()
	void HandleGroundTravelEnded();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastDetonation(FVector Location, float Radius);
	UFUNCTION()
	void ApplyAttack(int32 Index);
	UFUNCTION()
	void HandleRepeatStopped(ENF_RepeatStopReason Reason);
	UFUNCTION()
	void HandleDeploymentEnded(ENF_DeploymentEndReason Reason);
	UFUNCTION()
	void OnRep_Frame();
	bool UpdateTarget();
	void UpdateVisuals();
	UPROPERTY(Replicated)
	TObjectPtr<AActor> SourceAvatar;
	UPROPERTY(ReplicatedUsing = OnRep_Frame)
	FNF_SkillExecutionFrame Frame;
	UPROPERTY(Transient)
	FGameplayEffectSpecHandle DamageSpec;
	TWeakObjectPtr<UNFGA_SkillBase> SourceAbility;
	FNF_SkillPreviewData PreviewData;
	FVector ConfirmedLaunchDirection = FVector::ForwardVector;
	bool bHasLaunchDirection = false;
	bool bGroundResolved = false;
	bool bStartedNotified = false;
	bool bStopping = false;
	bool bPrepared = false;
	bool bEndedNotified = false;
};
