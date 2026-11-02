#include "GamePlay/Combat/Skill/Passive/Utility/NFGA_Thorns.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectExtension.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/Effects/NFGE_ThornsDamage.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Project_NF.h"
#include "GAS/Attributes/NF_KeyHolderAttributeSet.h"

UNFGA_Thorns::UNFGA_Thorns()
{
	GetMutableSkillDefaults().SkillType = ENF_SkillType::Passive; GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Utility;
	GetMutableSkillDefaults().SkillActivationMethod = ENF_SkillActivationMethod::OnGranted; GetMutableSkillDefaults().BuildCost = 3;
	bActivatePassiveOnGrant = false; ReflectionEffectClass = UNFGE_ThornsDamage::StaticClass();
	ActivationCueTag = NFTag_Skill::GameplayCue_Skill_Thorns_Activate;
}
float UNFGA_Thorns::CalculateReflectedDamage(float OldHealth, float NewHealth, float Ratio)
{
	if (!FMath::IsFinite(OldHealth) || !FMath::IsFinite(NewHealth) || !FMath::IsFinite(Ratio)) { return 0; }
	const float Before = FMath::Max(OldHealth, 0.0f);
	const float ActualLoss = FMath::Clamp(Before - FMath::Max(NewHealth, 0.0f), 0.0f, Before);
	return ActualLoss * FMath::Clamp(Ratio, 0.0f, 1.0f);
}
void UNFGA_Thorns::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec); BindHealth();
}
void UNFGA_Thorns::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec); BindHealth();
}
void UNFGA_Thorns::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	UnbindHealth(); Super::OnRemoveAbility(ActorInfo, Spec);
}
void UNFGA_Thorns::UnbindHealth()
{
	if (HealthASC.IsValid() && HealthDelegate.IsValid())
	{
		HealthASC->GetGameplayAttributeValueChangeDelegate(UNF_PlayerStatusAttribute::GetHealthAttribute()).Remove(HealthDelegate);
	}
	if (HealthASC.IsValid() && CooldownReductionHandle.IsValid())
	{
		HealthASC->GetGameplayAttributeValueChangeDelegate(UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute()).Remove(CooldownReductionHandle);
	}
	HealthDelegate.Reset(); CooldownReductionHandle.Reset(); HealthASC.Reset();
}
void UNFGA_Thorns::BindHealth()
{
	const auto* Info = GetCurrentActorInfo();
	if (!Info || !Info->IsNetAuthority()) { return; }
	AActor* Avatar = GetAvatarActorFromActorInfo();
	auto* OwnASC = GetAbilitySystemComponentFromActorInfo();
	const auto* Spec = OwnASC ? OwnASC->FindAbilitySpecFromHandle(CurrentSpecHandle) : nullptr;
	if (!Spec || Spec->GetPrimaryInstance() != this) { return; }
	auto* ASC = OwnASC;
	if (!ASC || !ASC->GetSet<UNF_PlayerStatusAttribute>())
	{
		ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar);
	}
	if (HealthASC.Get() == ASC && HealthDelegate.IsValid() && CooldownReductionHandle.IsValid())
	{
		return;
	}
	UnbindHealth();
	if (GetSkillType() != ENF_SkillType::Passive || !IsValid(Avatar) || !ASC || ASC->GetAvatarActor_Direct() != Avatar
		|| !ASC->GetSet<UNF_PlayerStatusAttribute>())
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[Thorns] Player Health AttributeSet is required: %s"), *GetNameSafe(Avatar)); return;
	}
	HealthASC = ASC;
	HealthDelegate = ASC->GetGameplayAttributeValueChangeDelegate(UNF_PlayerStatusAttribute::GetHealthAttribute())
		.AddUObject(this, &ThisClass::HandleHealthChanged);
	CooldownReductionHandle = ASC->GetGameplayAttributeValueChangeDelegate(UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute()).AddUObject(this, &ThisClass::OnCooldownReductionRateChanged);
}

float UNFGA_Thorns::GetCooldownReductionRate() const
{
	const auto* ASC = HealthASC.Get();
	if (!ASC || !ASC->GetSet<UNF_KeyHolderAttributeSet>())
	{
		return 0.0f;
	}
	const float Rate = ASC->GetNumericAttribute(UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute());
	return FMath::IsFinite(Rate) ? FMath::Clamp(Rate, 0.f, 1.f) : 0.f;
}

void UNFGA_Thorns::OnCooldownReductionRateChanged(const FOnAttributeChangeData& Data)
{
	const auto* Info = GetCurrentActorInfo();
	if (!Info || !Info->IsNetAuthority() || !GetWorld() || !bHasTriggeredCooldown || !FMath::IsFinite(Data.OldValue) || !FMath::IsFinite(Data.NewValue) || Data.OldValue > 0.f || Data.NewValue <= 0.f)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextReflectionTime)
	{
		return;
	}
	const float Rate = FMath::Clamp(Data.NewValue, 0.f, 1.f);
	const double ReducedEndTime = CooldownStartTime + BaseCooldownAtTrigger * (1.0 - Rate);
	NextReflectionTime = FMath::Min(NextReflectionTime, ReducedEndTime);
}

void UNFGA_Thorns::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// Mod callback data is only consumed synchronously, on the server. Replication/SetHealth alone is not a hit.
	if (bReflecting || !HealthASC.IsValid() || !GetWorld() || !Data.GEModData || !ReflectionEffectClass
		|| !FMath::IsFinite(InternalCooldown) || InternalCooldown < 0 || !FMath::IsFinite(ReflectionRatio)
		|| ReflectionEffectClass->GetDefaultObject<UGameplayEffect>()->DurationPolicy != EGameplayEffectDurationType::Instant) { return; }
	const auto& Incoming = Data.GEModData->EffectSpec;
	FGameplayTagContainer Tags; Incoming.GetAllAssetTags(Tags);
	if (Tags.HasTag(NFTag_Skill::Damage_Reflected)) { return; }
	const float Damage = CalculateReflectedDamage(Data.OldValue, Data.NewValue, ReflectionRatio);
	if (Damage <= 0 || GetWorld()->GetTimeSeconds() < NextReflectionTime) { return; }
	UAbilitySystemComponent* AttackerASC = Incoming.GetContext().GetOriginalInstigatorAbilitySystemComponent();
	AActor* Attacker = AttackerASC ? AttackerASC->GetAvatarActor_Direct() : nullptr;
	AActor* Victim = GetAvatarActorFromActorInfo();
	if (!IsValid(Attacker) || Attacker->IsActorBeingDestroyed() || Attacker == Victim || AttackerASC == HealthASC.Get()) { return; }
	const auto Spec = HealthASC->MakeOutgoingSpec(ReflectionEffectClass, GetAbilityLevel(), HealthASC->MakeEffectContext());
	if (!Spec.IsValid()) { return; }
	Spec.Data->AddDynamicAssetTag(NFTag_Skill::Damage_Reflected);
	Spec.Data->SetSetByCallerMagnitude(NFTag_Skill::Data_Damage_Reflected, Damage);
	const double Now = GetWorld()->GetTimeSeconds();
	CooldownStartTime = Now;
	BaseCooldownAtTrigger = InternalCooldown;
	bHasTriggeredCooldown = true;
	// Reserve cooldown before applying: synchronous GE callbacks must never recursively reflect.
	NextReflectionTime = CooldownStartTime + BaseCooldownAtTrigger * (1.0 - GetCooldownReductionRate());
	TGuardValue<bool> ReflectGuard(bReflecting, true);
	const auto Applied = HealthASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), AttackerASC);
	if (Applied.WasSuccessfullyApplied())
	{
		NotifyPassiveTriggered(Attacker, Victim->GetActorLocation(), Damage);
	}
}
