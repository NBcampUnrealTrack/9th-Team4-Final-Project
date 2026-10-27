#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_WeaponPassive.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GamePlay/Combat/Damage/NF_WeaponDamageLibrary.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "Project_NF.h"

UNFGA_WeaponPassive::UNFGA_WeaponPassive()
{
	SkillType = ENF_SkillType::Passive;
	SkillCategory = ENF_SkillCategory::Attack;
	SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
	bActivatePassiveOnGrant = false;
}
void UNFGA_WeaponPassive::PrepareOffenseAttributes(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		UNF_WeaponDamageLibrary::EnsureOffenseAttributes(ActorInfo->AbilitySystemComponent.Get());
	}
}
void UNFGA_WeaponPassive::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	PrepareOffenseAttributes(ActorInfo);
	Super::OnGiveAbility(ActorInfo, Spec);
	RefreshStatEffect();
}
void UNFGA_WeaponPassive::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	PrepareOffenseAttributes(ActorInfo);
	Super::OnAvatarSet(ActorInfo, Spec);
	RefreshStatEffect();
}
void UNFGA_WeaponPassive::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	RemoveStatEffect();
	Super::OnRemoveAbility(ActorInfo, Spec);
}
void UNFGA_WeaponPassive::RemoveStatEffect()
{
	const auto OldEffect = StatEffect;
	UAbilitySystemComponent* OldASC = StatASC.Get();
	StatEffect = FActiveGameplayEffectHandle(); StatASC.Reset();
	if (IsValid(OldASC) && OldEffect.IsValid()) { OldASC->RemoveActiveGameplayEffect(OldEffect); }
}
void UNFGA_WeaponPassive::RefreshStatEffect()
{
	const auto* Info = GetCurrentActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Info || !Info->IsNetAuthority() || !IsValid(ASC)) { return; }
	const auto* AbilitySpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!AbilitySpec || AbilitySpec->GetPrimaryInstance() != this || !IsValid(GetAvatarActorFromActorInfo())) { return; }
	if (StatASC.Get() == ASC && StatEffect.IsValid() && ASC->GetActiveGameplayEffect(StatEffect)) { return; }
	RemoveStatEffect();
	const float Magnitude = GetStatMagnitude();
	const float Minimum = MagnitudeTag == NFTag_Skill::Data_Weapon_LastRoundMultiplier ? 1.0f : 0.0f;
	if (SkillType != ENF_SkillType::Passive || !StatEffectClass || !MagnitudeTag.IsValid()
		|| !FMath::IsFinite(Magnitude) || Magnitude < Minimum || !UNF_WeaponDamageLibrary::EnsureOffenseAttributes(ASC))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[WeaponPassive] Invalid passive type/magnitude/settings: %s"), *GetName()); return;
	}
	const auto Spec = MakeOutgoingGameplayEffectSpec(StatEffectClass, GetAbilityLevel());
	if (!Spec.IsValid()) { return; }
	Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);
	const auto Applied = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	// GE callbacks can synchronously unequip the skill. Do not leak its effect in that case.
	AbilitySpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!AbilitySpec || AbilitySpec->GetPrimaryInstance() != this)
	{
		ASC->RemoveActiveGameplayEffect(Applied); return;
	}
	StatASC = ASC; StatEffect = Applied;
	TrackPassiveEffect(Applied);
}
