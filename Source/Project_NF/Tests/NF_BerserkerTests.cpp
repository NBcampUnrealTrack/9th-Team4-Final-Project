#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameplayEffect.h"
#include "GAS/Attributes/NF_OffenseAttributeSet.h"
#include "GAS/Tags/NFTag_Player.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_Berserker.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_AttackPower.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BerserkerTest, "NF.Skills.Berserker.HealthOnlyStacksAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_BerserkerTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Actor = World->SpawnActor<AActor>();
	auto* ASC = NewObject<UAbilitySystemComponent>(Actor); Actor->AddInstanceComponent(ASC);
	ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Actor, Actor);
	ASC->AddAttributeSetSubobject(NewObject<UNF_PlayerStatusAttribute>(Actor));
	const auto Health = UNF_PlayerStatusAttribute::GetHealthAttribute();
	const auto Bonus = UNF_OffenseAttributeSet::GetAttackDamageBonusAttribute();
	const auto Hit = [ASC](FGameplayAttribute Attribute, float Amount)
	{
		auto* Effect = NewObject<UGameplayEffect>(GetTransientPackage());
		Effect->DurationPolicy = EGameplayEffectDurationType::Instant;
		auto& Modifier = Effect->Modifiers.AddDefaulted_GetRef(); Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::AddBase; Modifier.ModifierMagnitude = FScalableFloat(Amount);
		FGameplayEffectSpec Spec(Effect, ASC->MakeEffectContext(), 1);
		ASC->ApplyGameplayEffectSpecToSelf(Spec);
	};
	ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_AttackPower::StaticClass(), 1));
	const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Berserker::StaticClass(), 1));
	auto* AbilitySpec = ASC->FindAbilitySpecFromHandle(Handle);
	auto* GA = AbilitySpec ? Cast<UNFGA_Berserker>(AbilitySpec->GetPrimaryInstance()) : nullptr;
	if (!TestNotNull(TEXT("Passive instance exists"), GA))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	Hit(UNF_PlayerStatusAttribute::GetShieldAttribute(), -20);
	// Reuse a registered cue tag to test lifecycle without requiring a visual asset.
	GA->GetMutablePassiveDefaults().ActiveCueTag = NFTag_Skill::GameplayCue_Skill_Berserker_Activate;
	Hit(Health, 10);
	ASC->SetNumericAttributeBase(Health, 90);
	TestEqual(TEXT("Shield, healing and direct setter do not stack"), ASC->GetNumericAttribute(Bonus), 10.0f);
	Hit(Health, -1);
	TestEqual(TEXT("First health hit adds three alongside attack passive"), ASC->GetNumericAttribute(Bonus), 13.0f);
	TestEqual(TEXT("First stack starts one persistent cue"), ASC->GetTagCount(GA->GetMutablePassiveDefaults().ActiveCueTag), 1);
	Hit(Health, -10);
	TestEqual(TEXT("Different damage amount still adds one stack"), ASC->GetNumericAttribute(Bonus), 16.0f);
	for (int32 Index = 0; Index < 6; ++Index) { Hit(Health, -1); }
	TestEqual(TEXT("Five stack cap"), ASC->GetNumericAttribute(Bonus), 25.0f);
	TestEqual(TEXT("Further stacks do not add duplicate persistent cues"), ASC->GetTagCount(GA->GetMutablePassiveDefaults().ActiveCueTag), 1);
	Hit(Health, -1000);
	TestEqual(TEXT("Lethal hit clears stacks without removing other passive"), ASC->GetNumericAttribute(Bonus), 10.0f);
	TestEqual(TEXT("Death removes persistent cue"), ASC->GetTagCount(GA->GetMutablePassiveDefaults().ActiveCueTag), 0);
	ASC->SetNumericAttributeBase(Health, 100);
	GA->StackDuration = 1.0f;
	Hit(Health, -1);
	World->Tick(LEVELTICK_TimeOnly, 0.6f);
	Hit(Health, -1);
	World->Tick(LEVELTICK_TimeOnly, 0.6f);
	TestEqual(TEXT("New hit refreshes entire duration"), ASC->GetNumericAttribute(Bonus), 16.0f);
	World->Tick(LEVELTICK_TimeOnly, 0.6f);
	TestEqual(TEXT("Timed stacks expire together"), ASC->GetNumericAttribute(Bonus), 10.0f);
	TestEqual(TEXT("Expiry removes persistent cue"), ASC->GetTagCount(GA->GetMutablePassiveDefaults().ActiveCueTag), 0);
	Hit(Health, -1);
	ASC->AddLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestEqual(TEXT("Death tag immediately resets stacks"), ASC->GetNumericAttribute(Bonus), 10.0f);
	ASC->RemoveLooseGameplayTag(NFTag_Player::Player_State_Death);
	Hit(Health, -1);
	ASC->ClearAbility(Handle);
	TestEqual(TEXT("Unequip removes persistent cue"), ASC->GetTagCount(NFTag_Skill::GameplayCue_Skill_Berserker_Activate), 0);
	Hit(Health, -1);
	TestEqual(TEXT("Unequip removes own bonus and listener"), ASC->GetNumericAttribute(Bonus), 10.0f);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif
