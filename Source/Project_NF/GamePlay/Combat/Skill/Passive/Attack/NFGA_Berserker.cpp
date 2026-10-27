#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_Berserker.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectExtension.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GamePlay/Combat/Damage/NF_WeaponDamageLibrary.h"
#include "GAS/Effects/NFGE_WeaponPassives.h"
#include "GAS/Tags/NFTag_Player.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Project_NF.h"

UNFGA_Berserker::UNFGA_Berserker()
{
	SkillType = ENF_SkillType::Passive; SkillCategory = ENF_SkillCategory::Attack;
	SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
	bActivatePassiveOnGrant = false; BuildCost = 4;
	ActivationCueTag = NFTag_Skill::GameplayCue_Skill_Berserker_Activate;
}
void UNFGA_Berserker::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec); BindHealth();
}
void UNFGA_Berserker::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec); BindHealth();
}
void UNFGA_Berserker::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	UnbindHealth(); Super::OnRemoveAbility(ActorInfo, Spec);
}
void UNFGA_Berserker::ResetStacks()
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(ExpiryTimer); }
	const auto Previous = BonusEffect;
	BonusEffect = FActiveGameplayEffectHandle(); StackCount = 0;
	StopPassiveActiveCue();
	if (BonusASC.IsValid() && Previous.IsValid()) { BonusASC->RemoveActiveGameplayEffect(Previous); }
}
void UNFGA_Berserker::UnbindHealth()
{
	if (HealthASC.IsValid())
	{
		HealthASC->GetGameplayAttributeValueChangeDelegate(UNF_PlayerStatusAttribute::GetHealthAttribute()).Remove(HealthDelegate);
		HealthASC->RegisterGameplayTagEvent(NFTag_Player::Player_State_Death).Remove(HealthDeathDelegate);
	}
	if (BonusASC.IsValid() && OwnDeathDelegate.IsValid())
	{
		BonusASC->RegisterGameplayTagEvent(NFTag_Player::Player_State_Death).Remove(OwnDeathDelegate);
	}
	HealthDelegate.Reset(); HealthDeathDelegate.Reset(); OwnDeathDelegate.Reset();
	ResetStacks(); HealthASC.Reset(); BonusASC.Reset(); ObservedAvatar.Reset();
}
void UNFGA_Berserker::BindHealth()
{
	const auto* Info = GetCurrentActorInfo();
	if (!Info || !Info->IsNetAuthority()) { return; }
	auto* OwnASC = GetAbilitySystemComponentFromActorInfo();
	auto* Avatar = GetAvatarActorFromActorInfo();
	const auto* Spec = OwnASC ? OwnASC->FindAbilitySpecFromHandle(CurrentSpecHandle) : nullptr;
	if (!Spec || Spec->GetPrimaryInstance() != this) { return; }
	auto* ASC = OwnASC->GetSet<UNF_PlayerStatusAttribute>() ? OwnASC
		: UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar);
	if (HealthASC.Get() == ASC && BonusASC.Get() == OwnASC && ObservedAvatar.Get() == Avatar && HealthDelegate.IsValid()) { return; }
	UnbindHealth();
	if (SkillType != ENF_SkillType::Passive || !IsValid(Avatar) || !ASC || ASC->GetAvatarActor_Direct() != Avatar
		|| !ASC->GetSet<UNF_PlayerStatusAttribute>() || !UNF_WeaponDamageLibrary::EnsureOffenseAttributes(OwnASC))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[Berserker] Player Health and offense attributes are required: %s"), *GetNameSafe(Avatar)); return;
	}
	HealthASC = ASC; BonusASC = OwnASC; ObservedAvatar = Avatar;
	HealthDelegate = ASC->GetGameplayAttributeValueChangeDelegate(UNF_PlayerStatusAttribute::GetHealthAttribute())
		.AddUObject(this, &ThisClass::HandleHealthChanged);
	HealthDeathDelegate = ASC->RegisterGameplayTagEvent(NFTag_Player::Player_State_Death)
		.AddUObject(this, &ThisClass::HandleDeathTag);
	if (ASC != OwnASC)
	{
		OwnDeathDelegate = OwnASC->RegisterGameplayTagEvent(NFTag_Player::Player_State_Death)
			.AddUObject(this, &ThisClass::HandleDeathTag);
	}
}
bool UNFGA_Berserker::IsDead() const
{
	return !HealthASC.IsValid() || HealthASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetHealthAttribute()) <= 0
		|| HealthASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)
		|| (BonusASC.IsValid() && BonusASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death));
}
void UNFGA_Berserker::HandleDeathTag(FGameplayTag Tag, int32 Count)
{
	if (Count > 0) { ResetStacks(); }
}
void UNFGA_Berserker::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (IsDead()) { ResetStacks(); return; }
	// Only executed GE health loss counts: shield loss, healing and direct setters do not.
	if (!Data.GEModData || !FMath::IsFinite(Data.OldValue) || !FMath::IsFinite(Data.NewValue)
		|| Data.OldValue <= Data.NewValue || Data.OldValue <= 0 || !BonusASC.IsValid() || !GetWorld()
		|| MaxStacks < 1 || !FMath::IsFinite(DamageBonusPerStack) || DamageBonusPerStack <= 0
		|| !FMath::IsFinite(StackDuration) || StackDuration < 0) { return; }
	auto* ASC = BonusASC.Get();
	const auto* AbilitySpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!AbilitySpec || AbilitySpec->GetPrimaryInstance() != this) { return; }
	if (!BonusEffect.IsValid() || !ASC->GetActiveGameplayEffect(BonusEffect)) { StackCount = 0; BonusEffect = FActiveGameplayEffectHandle(); }
	const int32 NextCount = StackCount >= MaxStacks ? MaxStacks : StackCount + 1;
	const bool bAddedStack = NextCount > StackCount;
	const float Magnitude = static_cast<float>(FMath::Min(static_cast<double>(DamageBonusPerStack) * NextCount, static_cast<double>(MAX_flt)));
	StackCount = NextCount;
	if (BonusEffect.IsValid())
	{
		ASC->UpdateActiveGameplayEffectSetByCallerMagnitude(BonusEffect, NFTag_Skill::Data_Weapon_AttackDamageBonus, Magnitude);
	}
	else
	{
		const auto Spec = MakeOutgoingGameplayEffectSpec(UNFGE_AttackDamageBonus::StaticClass(), GetAbilityLevel());
		if (!Spec.IsValid()) { StackCount = 0; return; }
		Spec.Data->SetSetByCallerMagnitude(NFTag_Skill::Data_Weapon_AttackDamageBonus, Magnitude);
		const auto Applied = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		AbilitySpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
		if (!AbilitySpec || AbilitySpec->GetPrimaryInstance() != this || BonusASC.Get() != ASC || IsDead())
		{
			ASC->RemoveActiveGameplayEffect(Applied); ResetStacks(); return;
		}
		BonusEffect = Applied;
		if (!Applied.IsValid()) { StackCount = 0; return; }
	}
	if (StackCount > 0 && StackDuration > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(ExpiryTimer, this, &ThisClass::ResetStacks, StackDuration, false);
	}
	if (StackCount > 0)
	{
		StartPassiveActiveCue();
		if (bAddedStack && StackCount > 0)
		{
			if (auto* Avatar = GetAvatarActorFromActorInfo()) { NotifyPassiveTriggered(Avatar, Avatar->GetActorLocation(), StackCount); }
		}
	}
}
