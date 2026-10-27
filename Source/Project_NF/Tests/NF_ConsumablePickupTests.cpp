#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/ConsumableItems/NF_ConsumableComponent.h"
#include "GamePlay/ConsumableItems/NF_ConsumablePickup.h"
#include "GamePlay/ConsumableItems/Data/NF_ConsumableItemDataAsset.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-27
// Existing interaction entry, all-or-nothing pickup, stacking and server distance/occlusion checks.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_ConsumablePickupTest, "NF.Consumables.Pickup.InteractionAndStorage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_ConsumablePickupTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Pawn = World->SpawnActor<ACharacter>();
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Pawn);
	auto* Inventory = NewObject<UNF_ConsumableComponent>(Pawn);
	Pawn->AddInstanceComponent(Inventory);
	Inventory->RegisterComponent();
	auto* Data = NewObject<UNF_ConsumableItemDataAsset>();
	auto* First = World->SpawnActor<ANF_ConsumablePickup>(FVector(100, 0, 0), FRotator::ZeroRotator);
	TestTrue(TEXT("Pickup initialized on server"), First->InitializePickup(Data, 3));
	TestFalse(TEXT("Invalid quantity rejected"), First->InitializePickup(Data, 0));
	TestFalse(TEXT("Invalid controller rejected"), First->TryPickup(nullptr));
	INF_InteractionInterface* Interaction = Cast<INF_InteractionInterface>(First);
	if (TestNotNull(TEXT("Existing interaction interface implemented"), Interaction))
	{
		Interaction->NF_Interaction(Controller);
	}
	FNF_ConsumableSlot Slot;
	Inventory->GetSlot(0, Slot);
	TestTrue(TEXT("F interaction stores original DA"), Slot.ItemData.Get() == Data);
	TestEqual(TEXT("F interaction stores entire quantity"), Slot.Quantity, 3);
	TestEqual(TEXT("Successful world pickup is exhausted"), First->GetQuantity(), 0);
	TestFalse(TEXT("Same pickup cannot be claimed twice"), First->TryPickup(Controller));
	TestTrue(TEXT("Same source stacks"), Inventory->TryAddItem(Data, 2));
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("Stack count added"), Slot.Quantity, 5);
	for (int32 Index = 1; Index < 5; ++Index)
	{
		Inventory->SetSlot(Index, NewObject<UNF_ConsumableItemDataAsset>(), 1);
	}
	auto* OtherData = NewObject<UNF_ConsumableItemDataAsset>();
	auto* Full = World->SpawnActor<ANF_ConsumablePickup>(FVector(100, 0, 0), FRotator::ZeroRotator);
	Full->InitializePickup(OtherData, 2);
	TestFalse(TEXT("Full inventory rejects different item"), Full->TryPickup(Controller));
	TestEqual(TEXT("Failed pickup keeps world quantity"), Full->GetQuantity(), 2);
	TestFalse(TEXT("Failed pickup does not destroy actor"), Full->IsActorBeingDestroyed());
	TestTrue(TEXT("Existing stack can grow even with five occupied slots"), Inventory->TryAddItem(Data, 1));
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("Existing contents retained and stacked"), Slot.Quantity, 6);
	Inventory->ClearSlot(4);
	auto* Far = World->SpawnActor<ANF_ConsumablePickup>(FVector(10000, 0, 0), FRotator::ZeroRotator);
	Far->InitializePickup(OtherData, 1);
	TestFalse(TEXT("Far interaction rejected by server"), Far->TryPickup(Controller));
	FVector Eyes;
	FRotator EyeRotation;
	Pawn->GetActorEyesViewPoint(Eyes, EyeRotation);
	auto* Wall = World->SpawnActor<AActor>();
	auto* Box = NewObject<UBoxComponent>(Wall);
	Wall->AddInstanceComponent(Box);
	Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(10, 60, 100));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Box->RegisterComponent();
	Wall->SetActorLocation((Eyes + Full->GetActorLocation()) * 0.5);
	TestFalse(TEXT("Occluded interaction rejected by server"), Full->TryPickup(Controller));
	TestEqual(TEXT("Occlusion does not consume world item"), Full->GetQuantity(), 2);
	Wall->SetActorEnableCollision(false);
	TestTrue(TEXT("Free slot accepts pickup after occlusion removed"), Full->TryPickup(Controller));
	Inventory->GetSlot(4, Slot);
	TestTrue(TEXT("First available slot used"), Slot.ItemData.Get() == OtherData && Slot.Quantity == 2);
	Inventory->SetSlot(0, Data, MAX_int32);
	TestFalse(TEXT("Stack overflow rejected"), Inventory->TryAddItem(Data, 1));
	Inventory->GetSlot(0, Slot);
	TestEqual(TEXT("Overflow leaves existing count unchanged"), Slot.Quantity, MAX_int32);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
