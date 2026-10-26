#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "GamePlay/Combat/Skill/Passive/Utility/NFGA_Thorns.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_ThornsTest, "NF.Skills.Thorns.ActualHealthLossCooldownAndNoRecursion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_ThornsTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	TestEqual(TEXT("100 health against 1000 damage reflects only actual 100 loss"), UNFGA_Thorns::CalculateReflectedDamage(100, -900, 0.3f), 30.0f);
	TestTrue(TEXT("100 to 70 reflects nine"), FMath::IsNearlyEqual(UNFGA_Thorns::CalculateReflectedDamage(100, 70, 0.3f), 9.0f));
	TestEqual(TEXT("Healing does not reflect"), UNFGA_Thorns::CalculateReflectedDamage(70, 100, 0.3f), 0.0f);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	const auto MakeASC = [World]()
	{
		auto* Actor = World->SpawnActor<AActor>();
		auto* ASC = NewObject<UAbilitySystemComponent>(Actor); Actor->AddInstanceComponent(ASC);
		ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Actor, Actor);
		ASC->AddAttributeSetSubobject(NewObject<UNF_PlayerStatusAttribute>(Actor)); return ASC;
	};
	auto* Attacker = MakeASC(); auto* Victim = MakeASC();
	const auto Health = UNF_PlayerStatusAttribute::GetHealthAttribute();
	const auto Shield = UNF_PlayerStatusAttribute::GetShieldAttribute();
	const auto Hit = [](UAbilitySystemComponent* Source, UAbilitySystemComponent* Target, FGameplayAttribute Attribute, float Amount)
	{
		auto* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		auto& Modifier = Effect->Modifiers.AddDefaulted_GetRef(); Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::AddBase; Modifier.ModifierMagnitude = FScalableFloat(Amount);
		FGameplayEffectSpec Spec(Effect, Source->MakeEffectContext(), 1);
		Source->ApplyGameplayEffectSpecToTarget(Spec, Target);
	};
	const auto Handle = Victim->GiveAbility(FGameplayAbilitySpec(UNFGA_Thorns::StaticClass(), 1));
	Hit(Attacker, Victim, Health, -1000);
	TestEqual(TEXT("Overkill clamps victim health to zero"), Victim->GetNumericAttribute(Health), 0.0f);
	TestTrue(TEXT("Real GE callback reflects 30, not 300"), FMath::IsNearlyEqual(Attacker->GetNumericAttribute(Health), 70.0f));
	Victim->SetNumericAttributeBase(Health, 100);
	Hit(Attacker, Victim, Health, -10);
	TestTrue(TEXT("Internal cooldown suppresses another reflection"), FMath::IsNearlyEqual(Attacker->GetNumericAttribute(Health), 70.0f));
	World->Tick(LEVELTICK_TimeOnly, 1.1f);
	Hit(Attacker, Victim, Shield, -50);
	TestTrue(TEXT("Shield-only damage does not reflect"), FMath::IsNearlyEqual(Attacker->GetNumericAttribute(Health), 70.0f));
	Hit(Attacker, Victim, Health, 10);
	TestTrue(TEXT("Healing GE does not reflect"), FMath::IsNearlyEqual(Attacker->GetNumericAttribute(Health), 70.0f));
	Attacker->GiveAbility(FGameplayAbilitySpec(UNFGA_Thorns::StaticClass(), 1));
	Hit(Attacker, Victim, Health, -30);
	TestTrue(TEXT("Cooldown ending permits next reflection"), FMath::IsNearlyEqual(Attacker->GetNumericAttribute(Health), 61.0f));
	TestTrue(TEXT("Two thorns passives cannot recursively reflect"), FMath::IsNearlyEqual(Victim->GetNumericAttribute(Health), 70.0f));
	Victim->ClearAbility(Handle);
	World->Tick(LEVELTICK_TimeOnly, 1.1f);
	Hit(Attacker, Victim, Health, -10);
	TestTrue(TEXT("Unequipping stops observation and reflection"), FMath::IsNearlyEqual(Attacker->GetNumericAttribute(Health), 61.0f));
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif
