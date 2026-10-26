#include "GamePlay/Combat/Skill/Passive/Utility/NFGA_DoubleJump.h"
#include "GamePlay/Combat/Skill/Execution/NF_JumpCountEffect.h"
#include "GAS/Effects/NFGE_DoubleJump.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "GameFramework/Character.h"
#include "Project_NF.h"
#include "AbilitySystemComponent.h"

UNFGA_DoubleJump::UNFGA_DoubleJump()
{
	SkillType = ENF_SkillType::Passive;
	SkillCategory = ENF_SkillCategory::Utility;
	SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
	BuildCost = 3;
	bActivatePassiveOnGrant = false;
	// Actual jump success must call NotifyPassiveTriggered; granting the skill is not a jump.
	ActivationCueTag = NFTag_Skill::GameplayCue_Skill_DoubleJump_Activate;
	PassiveEffects.Add(UNFGE_DoubleJump::StaticClass());
}

void UNFGA_DoubleJump::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec); RefreshJumpRequest();
}

void UNFGA_DoubleJump::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec); RefreshJumpRequest();
}

void UNFGA_DoubleJump::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	RemoveJumpRequest(); Super::OnRemoveAbility(ActorInfo, Spec);
}

void UNFGA_DoubleJump::RefreshJumpRequest()
{
	const auto* Info = GetCurrentActorInfo();
	if (!Info || !Info->IsNetAuthority()) { return; }
	auto* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayAbilitySpec* Spec = ASC ? ASC->FindAbilitySpecFromHandle(CurrentSpecHandle) : nullptr;
	if (!Spec || Spec->GetPrimaryInstance() != this) { return; }
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character == JumpAvatar.Get() && JumpEffect.IsValid() && JumpRequest.IsValid()) { return; }
	RemoveJumpRequest();
	if (!IsValid(Character) || Character->IsActorBeingDestroyed()) { return; }
	if (SkillType != ENF_SkillType::Passive || MaxJumpCount < 2 || MaxJumpCount > 32)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[DoubleJump] Use Skill Type Passive and Max Jump Count 2..32: %s"), *GetName()); return;
	}
	JumpEffect = ANF_JumpCountEffect::FindOrCreate(Character);
	if (JumpEffect.IsValid())
	{
		JumpRequest = JumpEffect->AddRequest(MaxJumpCount);
		JumpAvatar = Character;
	}
}

void UNFGA_DoubleJump::RemoveJumpRequest()
{
	if (JumpEffect.IsValid()) { JumpEffect->RemoveRequest(JumpRequest); }
	JumpEffect.Reset(); JumpAvatar.Reset(); JumpRequest.Invalidate();
}
