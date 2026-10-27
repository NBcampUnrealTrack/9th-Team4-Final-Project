#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GamePlay/ConsumableItems/NF_ConsumableComponent.h"
#include "GamePlay/ConsumableItems/Data/NF_ConsumableItemDataAsset.h"
#include "GamePlay/ConsumableItems/Items/NF_HealthPotion.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-27
// Actual GE healing and inventory charge, including rejected use and UI source reference.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_HealthPotionTest, "NF.Consumables.HealthPotion.UseAndCharge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_HealthPotionTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* User = World->SpawnActor<AActor>();
	auto* ASC = NewObject<UAbilitySystemComponent>(User);
	User->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(User, User);
	ASC->AddAttributeSetSubobject(NewObject<UNF_PlayerStatusAttribute>(User));
	auto* Inventory = NewObject<UNF_ConsumableComponent>(User);
	User->AddInstanceComponent(Inventory);
	Inventory->RegisterComponent();
	auto* Data = NewObject<UNF_ConsumableItemDataAsset>();
	Data->ItemClass = ANF_HealthPotion::StaticClass();
	Data->Icon = NewObject<UTexture2D>();
	Inventory->SetSlot(0, Data, 3);
	const auto Health = UNF_PlayerStatusAttribute::GetHealthAttribute();
	const auto Shield = UNF_PlayerStatusAttribute::GetShieldAttribute();
	FNF_ConsumableSlot Slot;
	TestFalse(TEXT("Full health does not use potion"), Inventory->TryUseItem(0));
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("Full health does not charge"), Slot.Quantity, 3);
	TestTrue(TEXT("VM can read icon through original slot DA"), Slot.ItemData->GetItemIcon() == Data->Icon.Get());
	ASC->SetNumericAttributeBase(Health, 80.f);
	ASC->SetNumericAttributeBase(Shield, 17.f);
	INF_UseItemInterface::Execute_NF_UseItem(Inventory, 0);
	TestEqual(TEXT("Use interface heals up to maximum"), ASC->GetNumericAttribute(Health), 100.f);
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("One successful interface use charges one"), Slot.Quantity, 2);
	TestEqual(TEXT("Potion does not change shield"), ASC->GetNumericAttribute(Shield), 17.f);
	ASC->SetNumericAttributeBase(Health, 40.f);
	TestTrue(TEXT("Partial health use succeeds"), Inventory->TryUseItem(0));
	TestEqual(TEXT("Native potion heals 30"), ASC->GetNumericAttribute(Health), 70.f);
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("Quantity reduced once"), Slot.Quantity, 1);

	FGameplayEffectApplicationQuery Immunity = FGameplayEffectApplicationQuery::CreateLambda(
		[](const FActiveGameplayEffectsContainer&, const FGameplayEffectSpec&) { return false; });
	const FDelegateHandle ImmunityHandle = Immunity.GetHandle();
	ASC->GameplayEffectApplicationQueries.Add(MoveTemp(Immunity));
	TestFalse(TEXT("Rejected healing does not succeed"), Inventory->TryUseItem(0));
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("Rejected healing preserves quantity"), Slot.Quantity, 1);
	ASC->GameplayEffectApplicationQueries.RemoveAll([ImmunityHandle](const FGameplayEffectApplicationQuery& Query)
	{
		return Query.GetHandle() == ImmunityHandle;
	});
	ASC->AddLooseGameplayTag(NFTag_LifeState::State_Life_Downed);
	TestFalse(TEXT("Downed player cannot drink"), Inventory->TryUseItem(0));
	ASC->RemoveLooseGameplayTag(NFTag_LifeState::State_Life_Downed);
	ASC->SetNumericAttributeBase(Health, 0.f);
	TestFalse(TEXT("Potion is not a revive"), Inventory->TryUseItem(0));
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("Blocked life states preserve quantity"), Slot.Quantity, 1);
	ASC->SetNumericAttributeBase(Health, 99.f);
	TestTrue(TEXT("Last potion heals remaining point"), Inventory->TryUseItem(0));
	Inventory->GetSlot(0, Slot);
	TestTrue(TEXT("Last success empties reference and count"), Slot.ItemData == nullptr && Slot.Quantity == 0);
	TestFalse(TEXT("Empty slot cannot use again"), Inventory->TryUseItem(0));
	TestFalse(TEXT("Invalid index rejected"), Inventory->TryUseItem(5));
	Data->ItemClass = AActor::StaticClass();
	Inventory->SetSlot(1, Data, 1);
	TestFalse(TEXT("Unsupported class rejected"), Inventory->TryUseItem(1));
	Inventory->GetSlot(1, Slot);
	TestEqual(TEXT("Invalid effect class does not spend item"), Slot.Quantity, 1);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
