#pragma once
#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Passive/NFGA_PassiveSkillBase.h"
#include "NFGA_DoubleJump.generated.h"

class ANF_JumpCountEffect;
class ACharacter;

// @Owner: 이정준 (구현: Codex) | 2026-10-21
// Equipped by granting the ability; no input activation or player source changes required.
UCLASS(Blueprintable)
class PROJECT_NF_API UNFGA_DoubleJump : public UNFGA_PassiveSkillBase
{
	GENERATED_BODY()
public:
	UNFGA_DoubleJump();
	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override { return false; }
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Double Jump", meta = (ClampMin = "2", ClampMax = "32"))
	int32 MaxJumpCount = 2;
private:
	void RefreshJumpRequest();
	void RemoveJumpRequest();
	TWeakObjectPtr<ANF_JumpCountEffect> JumpEffect;
	TWeakObjectPtr<ACharacter> JumpAvatar;
	FGuid JumpRequest;
};
