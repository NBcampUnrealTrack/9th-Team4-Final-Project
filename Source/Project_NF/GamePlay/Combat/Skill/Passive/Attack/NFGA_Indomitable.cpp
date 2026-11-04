#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_Indomitable.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "GAS/Attributes/NF_KeyHolderAttributeSet.h"
#include "GAS/Tags/NFTag_Player.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Project_NF.h"
#include "TimerManager.h"

UNFGA_Indomitable::UNFGA_Indomitable()
{
	GetMutableSkillDefaults().SkillType = ENF_SkillType::Passive;
	GetMutableSkillDefaults().SkillCategory = ENF_SkillCategory::Utility;
	GetMutableSkillDefaults().SkillActivationMethod = ENF_SkillActivationMethod::OnGranted;
	GetMutablePassiveDefaults().bActivatePassiveOnGrant = false;
	GetMutableSkillDefaults().BuildCost = 4;
}

void UNFGA_Indomitable::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	bRemoving = false;
	BindRetryCount = 0;
	LastBindFailure.Reset();
	Super::OnGiveAbility(ActorInfo, Spec);
	BindDamageQuery();
}

void UNFGA_Indomitable::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	BindRetryCount = 0;
	LastBindFailure.Reset();
	Super::OnAvatarSet(ActorInfo, Spec);
	BindDamageQuery();
}

void UNFGA_Indomitable::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	bRemoving = true;
	UnbindDamageQuery();
	Super::OnRemoveAbility(ActorInfo, Spec);
}

void UNFGA_Indomitable::BindDamageQuery()
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(BindRetryTimer); }
	const auto* Info = GetCurrentActorInfo();
	if (bRemoving || !Info || !Info->IsNetAuthority()) { return; }
	auto* OwnASC = GetAbilitySystemComponentFromActorInfo();
	auto* Avatar = GetAvatarActorFromActorInfo();
	const auto* Spec = OwnASC ? OwnASC->FindAbilitySpecFromHandle(CurrentSpecHandle) : nullptr;
	if (!Spec || Spec->GetPrimaryInstance() != this)
	{
		QueueBindRetry(TEXT("부여된 GA 인스턴스가 아직 준비되지 않음")); return;
	}
	auto* ASC = OwnASC->GetSet<UNF_PlayerStatusAttribute>() ? OwnASC
		: UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar);
	if (HealthASC.Get() == ASC && ObservedAvatar.Get() == Avatar && DamageQueryHandle.IsValid())
	{
		if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(BindRetryTimer); }
		return;
	}
	UnbindDamageQuery();
	if (GetSkillType() != ENF_SkillType::Passive)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[Indomitable][연결 실패] Skill Type을 Passive로 설정해야 합니다. GA=%s"), *GetName());
		return;
	}
	if (!IsValid(Avatar) || Avatar->IsActorBeingDestroyed())
	{
		QueueBindRetry(TEXT("현재 캐릭터 Avatar가 유효하지 않음")); return;
	}
	if (!ASC)
	{
		QueueBindRetry(TEXT("피해 GE를 검사할 ASC를 찾지 못함")); return;
	}
	if (ASC->GetAvatarActor_Direct() != Avatar)
	{
		QueueBindRetry(TEXT("ASC의 Avatar와 불굴의 현재 캐릭터가 다름")); return;
	}
	if (!ASC->GetSet<UNF_PlayerStatusAttribute>())
	{
		QueueBindRetry(TEXT("ASC에 NF_PlayerStatusAttribute가 아직 등록되지 않음")); return;
	}
	HealthASC = ASC;
	ObservedAvatar = Avatar;
	FGameplayEffectApplicationQuery Query = FGameplayEffectApplicationQuery::CreateUObject(
		this, &ThisClass::AllowGameplayEffect);
	DamageQueryHandle = Query.GetHandle();
	ASC->GameplayEffectApplicationQueries.Add(MoveTemp(Query));
	CooldownReductionHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute()).AddUObject(
			this, &ThisClass::OnCooldownReductionRateChanged);
	UE_LOG(LogProject_NF, Warning, TEXT("[Indomitable][연결 성공] 캐릭터=%s | ASC=%s | 재시도=%d | 공통 피해 GE 적용 전 검사 등록 완료."),
		*GetNameSafe(Avatar), *GetNameSafe(ASC), BindRetryCount);
	BindRetryCount = 0;
	LastBindFailure.Reset();
}

void UNFGA_Indomitable::QueueBindRetry(const TCHAR* Reason)
{
	if (bRemoving) { return; }
	auto* OwnASC = GetAbilitySystemComponentFromActorInfo();
	auto* Avatar = GetAvatarActorFromActorInfo();
	auto* AvatarASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar);
	if (LastBindFailure != Reason)
	{
		LastBindFailure = Reason;
		UE_LOG(LogProject_NF, Warning, TEXT("[Indomitable][연결 대기] 사유=%s | 캐릭터=%s | GA의 ASC=%s | GA ASC의 Avatar=%s | 캐릭터의 ASC=%s | 체력등록(GA/캐릭터)=%d/%d"),
			Reason, *GetNameSafe(Avatar), *GetNameSafe(OwnASC),
			*GetNameSafe(OwnASC ? OwnASC->GetAvatarActor_Direct() : nullptr), *GetNameSafe(AvatarASC),
			OwnASC && OwnASC->GetSet<UNF_PlayerStatusAttribute>() ? 1 : 0,
			AvatarASC && AvatarASC->GetSet<UNF_PlayerStatusAttribute>() ? 1 : 0);
	}
	if (!GetWorld()) { return; }
	if (BindRetryCount >= 50)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[Indomitable][연결 실패] 초기화 재시도 종료(최대 5초). 캐릭터=%s | 사유=%s"), *GetNameSafe(Avatar), Reason);
		return;
	}
	if (!GetWorld()->GetTimerManager().IsTimerActive(BindRetryTimer))
	{
		++BindRetryCount;
		GetWorld()->GetTimerManager().SetTimer(BindRetryTimer, this, &ThisClass::BindDamageQuery, 0.1f, false);
	}
}

void UNFGA_Indomitable::EndProtection()
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(ProtectionTimer); }
	ProtectionEndsAt = 0.0;
	StopPassiveActiveCue();
}

float UNFGA_Indomitable::GetCooldownReductionRate() const
{
	const auto* ASC = HealthASC.Get();
	if (!ASC || !ASC->GetSet<UNF_KeyHolderAttributeSet>()) { return 0.f; }
	const float Rate = ASC->GetNumericAttribute(UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute());
	return FMath::IsFinite(Rate) ? FMath::Clamp(Rate, 0.f, 1.f) : 0.f;
}

void UNFGA_Indomitable::OnCooldownReductionRateChanged(const FOnAttributeChangeData& Data)
{
	const auto* Info = GetCurrentActorInfo();
	// Match the common ASC policy: acquiring reduction (0 -> positive) shortens an active cooldown.
	// Losing the key does not lengthen it, and reacquiring it cannot subtract the same reduction twice.
	if (bRemoving || !Info || !Info->IsNetAuthority() || !GetWorld() || !bHasTriggeredCooldown
		|| !FMath::IsFinite(Data.OldValue) || !FMath::IsFinite(Data.NewValue)
		|| Data.OldValue > 0.f || Data.NewValue <= 0.f) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextTriggerTime) { return; }
	const double PreviousRemaining = NextTriggerTime - Now;
	const float Rate = FMath::Clamp(Data.NewValue, 0.f, 1.f);
	const double ReducedEndTime = CooldownStartTime + BaseCooldownAtTrigger * (1.0 - Rate);
	NextTriggerTime = FMath::Min(NextTriggerTime, ReducedEndTime);
	UE_LOG(LogProject_NF, Log, TEXT("[Indomitable][쿨감 적용] 캐릭터=%s | 기본쿨타임=%.2f초 | 쿨감=%.1f%% | 남은시간=%.2f -> %.2f초"),
		*GetNameSafe(ObservedAvatar.Get()), BaseCooldownAtTrigger, Rate * 100.f,
		PreviousRemaining, FMath::Max(0.0, NextTriggerTime - Now));
}

void UNFGA_Indomitable::UnbindDamageQuery()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(BindRetryTimer);
		GetWorld()->GetTimerManager().ClearTimer(ActivationCueTimer);
	}
	PendingTriggerDamage = 0.f;
	if (auto* ASC = HealthASC.Get(); ASC && CooldownReductionHandle.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UNF_KeyHolderAttributeSet::GetCooldownReductionRateAttribute())
			.Remove(CooldownReductionHandle);
	}
	CooldownReductionHandle.Reset();
	if (auto* ASC = HealthASC.Get(); ASC && DamageQueryHandle.IsValid())
	{
		const FDelegateHandle Handle = DamageQueryHandle;
		ASC->GameplayEffectApplicationQueries.RemoveAll([Handle](const FGameplayEffectApplicationQuery& Query)
		{
			return Query.GetHandle() == Handle;
		});
	}
	DamageQueryHandle.Reset();
	HealthASC.Reset();
	ObservedAvatar.Reset();
	EndProtection();
	// Keep the cooldown across avatar changes on this ability instance.
}

// Only preview the known, side-effect-free native calculation. Never execute arbitrary GE calculators here.
bool UNFGA_Indomitable::PredictCommonDamage(const FGameplayEffectSpec& Spec, float& OutHealthDamage, float& OutShieldDamage) const
{
	OutHealthDamage = 0.f;
	OutShieldDamage = 0.f;
	auto* ASC = HealthASC.Get();
	if (!ASC || !Spec.Def || !Spec.Def->IsA<UNFGE_CommonDamage>()
		|| Spec.Def->DurationPolicy != EGameplayEffectDurationType::Instant
		|| Spec.GetPeriod() != UGameplayEffect::NO_PERIOD || Spec.GetStackCount() != 1
		|| !Spec.Def->Modifiers.IsEmpty() || Spec.Def->Executions.Num() != 1) { return false; }
	const FGameplayEffectExecutionDefinition& Execution = Spec.Def->Executions[0];
	if (Execution.CalculationClass != UNFExecCalc_CommonDamage::StaticClass()) { return false; }

	// Query callbacks run before the engine captures target attributes/tags.
	FGameplayEffectSpec PreviewSpec(Spec);
	PreviewSpec.CaptureAttributeDataFromTarget(ASC);
	PreviewSpec.CapturedTargetTags.GetActorTags().Reset();
	ASC->GetOwnedGameplayTags(PreviewSpec.CapturedTargetTags.GetActorTags());
	FGameplayEffectCustomExecutionParameters Params(PreviewSpec, Execution.CalculationModifiers,
		ASC, Execution.PassedInTags, FPredictionKey());
	FGameplayEffectCustomExecutionOutput Output;
	GetDefault<UNFExecCalc_CommonDamage>()->Execute(Params, Output);
	for (const FGameplayModifierEvaluatedData& Modifier : Output.GetOutputModifiers())
	{
		if (!FMath::IsFinite(Modifier.Magnitude)) { return false; }
		if (Modifier.ModifierOp != EGameplayModOp::AddBase || Modifier.Magnitude >= 0.f) { continue; }
		if (Modifier.Attribute == UNF_PlayerStatusAttribute::GetHealthAttribute())
		{
			OutHealthDamage -= Modifier.Magnitude;
		}
		else if (Modifier.Attribute == UNF_PlayerStatusAttribute::GetShieldAttribute())
		{
			OutShieldDamage -= Modifier.Magnitude;
		}
	}
	return FMath::IsFinite(OutHealthDamage) && FMath::IsFinite(OutShieldDamage);
}

bool UNFGA_Indomitable::AllowGameplayEffect(const FActiveGameplayEffectsContainer& Container, const FGameplayEffectSpec& Spec)
{
	const auto* Info = GetCurrentActorInfo();
	auto* ASC = HealthASC.Get();
	auto* OwnASC = GetAbilitySystemComponentFromActorInfo();
	auto* Avatar = ObservedAvatar.Get();
	if (bRemoving || bHandlingDamage || !Info || !Info->IsNetAuthority() || !GetWorld()
		|| !ASC || !OwnASC || !IsValid(Avatar) || Avatar->IsActorBeingDestroyed()
		|| Avatar != GetAvatarActorFromActorInfo() || ASC->GetAvatarActor_Direct() != Avatar
		|| !CanContinueSkillExecution() || ASC->HasMatchingGameplayTag(NFTag_Player::Player_State_Death)) { return true; }
	const auto* AbilitySpec = OwnASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!AbilitySpec || AbilitySpec->GetPrimaryInstance() != this) { return true; }
	const float Health = ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetHealthAttribute());
	if (!FMath::IsFinite(Health) || Health <= 0.f
		|| !FMath::IsFinite(ProtectionDuration) || ProtectionDuration < 0.f
		|| !FMath::IsFinite(CooldownDuration) || CooldownDuration < 0.f) { return true; }

	TGuardValue<bool> HandlingGuard(bHandlingDamage, true);
	float HealthDamage = 0.f;
	float ShieldDamage = 0.f;
	if (!PredictCommonDamage(Spec, HealthDamage, ShieldDamage)
		|| (HealthDamage <= 0.f && ShieldDamage <= 0.f)) { return true; }
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < ProtectionEndsAt)
	{
		UE_LOG(LogProject_NF, Log, TEXT("[Indomitable][보호 중 차단] 캐릭터=%s | GE=%s | 체력피해=%.2f | 쉴드피해=%.2f"),
			*GetNameSafe(Avatar), *GetNameSafe(Spec.Def), HealthDamage, ShieldDamage);
		return false;
	}
	if (HealthDamage < Health || Now < NextTriggerTime) { return true; }

	CooldownStartTime = Now;
	BaseCooldownAtTrigger = CooldownDuration;
	bHasTriggeredCooldown = true;
	NextTriggerTime = CooldownStartTime + BaseCooldownAtTrigger * (1.0 - GetCooldownReductionRate());
	ProtectionEndsAt = Now + ProtectionDuration;
	PendingTriggerDamage = HealthDamage;
	if (ProtectionDuration > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(ProtectionTimer, this, &ThisClass::EndProtection, ProtectionDuration, false);
	}
	// No GE application or BP/Cue callbacks while the ASC iterates its query array.
	// Unequip/avatar changes cancel this deferred cosmetic notification.
	ActivationCueTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::EmitActivationCues);
	UE_LOG(LogProject_NF, Warning, TEXT("[Indomitable][불굴 발동] 캐릭터=%s | 현재체력=%.2f | 예상체력피해=%.2f | 예상쉴드피해=%.2f | 보호시간=%.2f초 | 쿨타임=%.2f초 | GE 적용 거부"),
		*GetNameSafe(Avatar), Health, HealthDamage, ShieldDamage, ProtectionDuration, NextTriggerTime - Now);
	return false;
}

void UNFGA_Indomitable::EmitActivationCues()
{
	auto* ASC = HealthASC.Get();
	auto* OwnASC = GetAbilitySystemComponentFromActorInfo();
	auto* Avatar = ObservedAvatar.Get();
	const auto* AbilitySpec = OwnASC ? OwnASC->FindAbilitySpecFromHandle(CurrentSpecHandle) : nullptr;
	if (bRemoving || !ASC || !IsValid(Avatar) || Avatar->IsActorBeingDestroyed()
		|| Avatar != GetAvatarActorFromActorInfo() || ASC->GetAvatarActor_Direct() != Avatar
		|| !AbilitySpec || AbilitySpec->GetPrimaryInstance() != this || !CanContinueSkillExecution()
		|| ASC->GetNumericAttribute(UNF_PlayerStatusAttribute::GetHealthAttribute()) <= 0.f) { return; }
	const float Damage = PendingTriggerDamage;
	PendingTriggerDamage = 0.f;
	if (GetWorld() && GetWorld()->GetTimeSeconds() < ProtectionEndsAt) { StartPassiveActiveCue(); }
	if (!bRemoving && HealthASC.Get() == ASC && ObservedAvatar.Get() == Avatar)
	{
		NotifyPassiveTriggered(Avatar, Avatar->GetActorLocation(), Damage);
	}
}
