#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "NFGA_Portal.generated.h"

class ANF_Portal;

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 지면 목표 확정 후 입구·출구를 서버에서 준비/검증/생성하는 전용 GAS 스킬.
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_Portal : public UNFGA_SkillBase
{
	GENERATED_BODY()
public:
	UNFGA_Portal();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const override;
	virtual bool BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const override;
	virtual bool ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const override;
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal")
	TSubclassOf<ANF_Portal> PortalClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "100.0", Units = "cm"))
	float PortalRange = 1500.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "100.0", Units = "cm"))
	float GroundSearchDepth = 3000.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "100.0", Units = "cm"))
	float EntranceForwardDistance = 180.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "100.0", Units = "cm"))
	float MinimumSeparation = 350.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Portal", meta = (ClampMin = "1.0", Units = "cm"))
	float PreviewRadius = 100.0f;
private:
	bool HasValidPortalSettings() const;
	void CreatePortalPair();
	void FailPlacement(const TCHAR* Reason);
	FTimerHandle PlacementTimer;
	UPROPERTY(Transient)
	TObjectPtr<ANF_Portal> PendingEntrance;
	UPROPERTY(Transient)
	TObjectPtr<ANF_Portal> PendingExit;
	TWeakObjectPtr<AActor> PlacementAvatar;
};
