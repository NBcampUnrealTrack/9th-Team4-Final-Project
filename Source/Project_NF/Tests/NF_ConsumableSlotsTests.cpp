#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GamePlay/ConsumableItems/NF_ConsumableComponent.h"
#include "GamePlay/ConsumableItems/Data/NF_ConsumableItemDataAsset.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// Fixed capacity, source references, and failure atomicity of quantity operations.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_ConsumableSlotsTest, "NF.Consumables.Slots.Storage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_ConsumableSlotsTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Owner = World->SpawnActor<AActor>();
	auto* Component = NewObject<UNF_ConsumableComponent>(Owner);
	Owner->AddInstanceComponent(Component);
	Component->RegisterComponent();
	auto* Item = NewObject<UNF_ConsumableItemDataAsset>();
	Item->DisplayName = FText::FromString(TEXT("Health potion"));
	TestEqual(TEXT("Exactly five slots"), Component->GetSlotCount(), 5);
	TestEqual(TEXT("Storage starts with five entries"), Component->GetSlots().Num(), 5);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FNF_ConsumableSlot Slot;
		TestTrue(TEXT("Initial slot can be read"), Component->GetSlot(Index, Slot));
		TestTrue(TEXT("Initial item is empty"), Slot.ItemData == nullptr);
		TestEqual(TEXT("Initial quantity is zero"), Slot.Quantity, 0);
	}
	TestFalse(TEXT("Negative index rejected"), Component->SetSlot(-1, Item, 1));
	TestFalse(TEXT("Sixth slot rejected"), Component->SetSlot(5, Item, 1));
	TestFalse(TEXT("Negative quantity rejected"), Component->SetSlot(0, Item, -1));
	TestFalse(TEXT("Nonempty item with zero count rejected"), Component->SetSlot(0, Item, 0));
	TestFalse(TEXT("Quantity without item rejected"), Component->SetSlot(0, nullptr, 1));
	TestTrue(TEXT("Store item in last slot"), Component->SetSlot(4, Item, 3));
	FNF_ConsumableSlot Slot;
	Component->GetSlot(4, Slot);
	TestTrue(TEXT("Slot refers to original DA"), Slot.ItemData.Get() == Item);
	TestEqual(TEXT("Stored quantity"), Slot.Quantity, 3);
	TestFalse(TEXT("Insufficient quantity leaves slot unchanged"), Component->RemoveFromSlot(4, 4));
	TestFalse(TEXT("Negative removal rejected"), Component->RemoveFromSlot(4, -1));
	Component->GetSlot(4, Slot);
	TestEqual(TEXT("Rejected removals preserve quantity"), Slot.Quantity, 3);
	TestTrue(TEXT("Remove one"), Component->RemoveFromSlot(4));
	Component->GetSlot(4, Slot);
	TestEqual(TEXT("Remaining quantity"), Slot.Quantity, 2);
	TestTrue(TEXT("Consume remaining storage count"), Component->RemoveFromSlot(4, 2));
	Component->GetSlot(4, Slot);
	TestTrue(TEXT("Last removal clears original reference"), Slot.ItemData == nullptr);
	TestEqual(TEXT("Empty count"), Slot.Quantity, 0);
	TestTrue(TEXT("Explicit clear accepts occupied slot"), Component->SetSlot(0, Item, 1) && Component->ClearSlot(0));
	Slot.ItemData = Item;
	Slot.Quantity = 42;
	TestFalse(TEXT("Invalid read fails"), Component->GetSlot(5, Slot));
	TestTrue(TEXT("Invalid read clears stale output"), Slot.ItemData == nullptr && Slot.Quantity == 0);
	auto Copy = Component->GetSlots();
	Copy.Reset();
	TestEqual(TEXT("External snapshot cannot resize storage"), Component->GetSlots().Num(), 5);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
