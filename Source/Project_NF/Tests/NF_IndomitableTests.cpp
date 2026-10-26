#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GAS/Attributes/NF_KeyHolderAttributeSet.h"
#include "GAS/Effects/NF_CommonDamageEffects.h"
#include "GAS/Tags/NFTag_BossCombat.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_Indomitable.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// Real GE application: shield/defence/reduction prediction, cooldown and query ownership.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_IndomitableTest, "NF.Skills.Indomitable.ApplicationQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_IndomitableTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	const auto MakeASC = [World]()
	{
		auto* Actor = World->SpawnActor<AActor>();
		auto* ASC = NewObject<UAbilitySystemComponent>(Actor);
		Actor->AddInstanceComponent(ASC);
		ASC->RegisterComponent();
		ASC->InitAbilityActorInfo(Actor, Actor);
		ASC->AddAttributeSetSubobject(NewObject<UNF_PlayerStatusAttribute>(Actor));
		ASC->AddAttributeSetSubobject(NewObject<UNF_KeyHolderAttributeSet>(Actor));
		return ASC;
	};
	auto* Attacker = MakeASC();
	auto* Victim = MakeASC();
	const auto Health = UNF_PlayerStatusAttribute::GetHealthAttribute();
	const auto Shield = UNF_PlayerStatusAttribute::GetShieldAttribute();
	Victim->SetNumericAttributeBase(UNF_PlayerStatusAttribute::GetDefenceAttribute(), 5.f);
	Victim->SetNumericAttributeBase(UNF_KeyHolderAttributeSet::GetDamageReductionRateAttribute(), 0.25f);
	Victim->SetNumericAttributeBase(Shield, 20.f);
	Victim->GameplayEffectApplicationQueries.Add(FGameplayEffectApplicationQuery::CreateLambda(
		[](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&) { return true; }));
	const auto Ability = Victim->GiveAbility(FGameplayAbilitySpec(UNFGA_Indomitable::StaticClass(), 1));
	auto* Spec = Victim->FindAbilitySpecFromHandle(Ability);
	auto* GA = Spec ? Cast<UNFGA_Indomitable>(Spec->GetPrimaryInstance()) : nullptr;
	if (!TestNotNull(TEXT("Passive instance granted"), GA))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	GA->ProtectionDuration = 0.1f;
	GA->CooldownDuration = 0.5f;
	GA->BindDamageQuery();
	TestEqual(TEXT("Own query registers once and retains unrelated query"), Victim->GameplayEffectApplicationQueries.Num(), 2);
	const auto Hit = [Attacker, Victim](float Damage)
	{
		auto DamageSpec = Attacker->MakeOutgoingSpec(UNFGE_CommonDamage::StaticClass(), 1.f, Attacker->MakeEffectContext());
		DamageSpec.Data->SetSetByCallerMagnitude(NFTag_BossCombat::Data_Damage, Damage);
		return Attacker->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), Victim).WasSuccessfullyApplied();
	};
	TestTrue(TEXT("Nonlethal shield hit allowed"), Hit(25.f));
	TestEqual(TEXT("Defence/reduction yield 15 shield damage, not two executions"), Victim->GetNumericAttribute(Shield), 5.f);
	TestEqual(TEXT("Shield prevents health loss"), Victim->GetNumericAttribute(Health), 100.f);
	Victim->SetNumericAttributeBase(Health, 10.f);
	Victim->SetNumericAttributeBase(Shield, 20.f);
	TestFalse(TEXT("Lethal residual health damage rejects GE"), Hit(45.f));
	TestEqual(TEXT("First lethal hit leaves health unchanged"), Victim->GetNumericAttribute(Health), 10.f);
	TestEqual(TEXT("Entire GE rejected: shield unchanged too"), Victim->GetNumericAttribute(Shield), 20.f);
	TestFalse(TEXT("Protection blocks even a shield-only common hit"), Hit(6.f));

	auto* Healing = NewObject<UGameplayEffect>(GetTransientPackage());
	Healing->DurationPolicy = EGameplayEffectDurationType::Instant;
	auto& Modifier = Healing->Modifiers.AddDefaulted_GetRef();
	Modifier.Attribute = Health;
	Modifier.ModifierOp = EGameplayModOp::AddBase;
	Modifier.ModifierMagnitude = FScalableFloat(10.f);
	FGameplayEffectSpec HealSpec(Healing, Victim->MakeEffectContext(), 1.f);
	Victim->ApplyGameplayEffectSpecToSelf(HealSpec);
	TestEqual(TEXT("Healing GE remains allowed"), Victim->GetNumericAttribute(Health), 20.f);
	Victim->SetNumericAttributeBase(Health, 10.f);
	Victim->SetNumericAttributeBase(Shield, 0.f);
	World->Tick(LEVELTICK_TimeOnly, 0.2f);
	TestFalse(TEXT("Protection window ended"), World->GetTimeSeconds() < GA->ProtectionEndsAt);
	TestTrue(TEXT("Cooldown allows lethal damage through"), Hit(100.f));
	TestEqual(TEXT("Cooldown does not prevent health reaching zero"), Victim->GetNumericAttribute(Health), 0.f);
	Victim->SetNumericAttributeBase(Health, 10.f);
	World->Tick(LEVELTICK_TimeOnly, 0.4f);
	TestFalse(TEXT("After cooldown, lethal damage blocked again"), Hit(100.f));
	Victim->ClearAbility(Ability);
	TestEqual(TEXT("Unequip removes only own query"), Victim->GameplayEffectApplicationQueries.Num(), 1);
	TestTrue(TEXT("Unequipped passive no longer blocks"), Hit(100.f));
	TestEqual(TEXT("Damage applies after unequip"), Victim->GetNumericAttribute(Health), 0.f);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
