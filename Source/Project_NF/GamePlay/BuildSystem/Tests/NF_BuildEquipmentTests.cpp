#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GamePlay/BuildSystem/Player/NF_BuildEquipmentAdapter.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponentConfigDataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildEquipmentTransactionTest, "NF.Build.EquipmentTransaction",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildEquipmentTransactionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->EndPlay(EEndPlayReason::Quit);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	};
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	ACharacter* Player = World->SpawnActor<ACharacter>();
	if (!TestNotNull(TEXT("Player"), Player)) return false;
	UNF_ArmorComponent* Armors = NewObject<UNF_ArmorComponent>(Player);
	Player->AddInstanceComponent(Armors);
	UNF_ArmorComponentConfigDataAsset* Config = NewObject<UNF_ArmorComponentConfigDataAsset>(Armors);
	FObjectPropertyBase* ConfigProperty = FindFProperty<FObjectPropertyBase>(Armors->GetClass(), TEXT("Config"));
	if (!TestNotNull(TEXT("Armor config property"), ConfigProperty)) return false;
	ConfigProperty->SetObjectPropertyValue_InContainer(Armors, Config);
	Armors->RegisterComponent();
	UNF_BuildComponent* Build = NewObject<UNF_BuildComponent>(Player);
	Player->AddInstanceComponent(Build);
	Build->RegisterComponent();

	UNF_BuildRules* Rules = NewObject<UNF_BuildRules>();
	auto MakeArmor = [&](FName Id, ENF_ArmorSlot Slot, float Defence)
	{
		UNF_ArmorDataAsset* Source = NewObject<UNF_ArmorDataAsset>(Rules);
		Source->ArmorSlot = Slot;
		Source->ArmorClass = ANF_ArmorBase::StaticClass();
		Source->BaseDefence = Defence;
		Rules->ItemSources.Add(Id, Source);
		return Source;
	};
	UNF_ArmorDataAsset* T1 = MakeArmor(TEXT("Head.T1"), ENF_ArmorSlot::HeadArmor, 10.f);
	UNF_ArmorDataAsset* T2 = MakeArmor(TEXT("Head.T2"), ENF_ArmorSlot::HeadArmor, 20.f);
	MakeArmor(TEXT("Body.T1"), ENF_ArmorSlot::BodyArmor, 30.f);
	TMap<FName, FNF_BuildItemData> Data;
	for (FName Id : {FName(TEXT("Head.T1")), FName(TEXT("Head.T2")), FName(TEXT("Body.T1"))})
	{
		FNF_BuildItemData Resolved;
		if (!TestTrue(TEXT("Resolve selected native source"), Rules->ResolveItemData(Id, Resolved))) return false;
		Data.Add(Id, Resolved);
	}
	TestTrue(TEXT("Rules carries exact selected DA"), Data.FindChecked(TEXT("Head.T2")).ArmorData == T2);
	TestTrue(TEXT("CDO without default DA is valid with selected source"),
	         FNF_BuildEquipmentAdapter::IsArmorDefinitionCompatible(Data.FindChecked(TEXT("Head.T2"))));
	FNF_BuildItemData WrongSlot = Data.FindChecked(TEXT("Head.T2"));
	WrongSlot.ArmorSlot = ENF_ArmorSlot::BodyArmor;
	TestFalse(TEXT("Source/slot mismatch rejected"), FNF_BuildEquipmentAdapter::IsArmorDefinitionCompatible(WrongSlot));
	auto Selection = [](FName Id, int32 Slot = 0)
	{
		FNF_BuildSelection Result;
		Result.ItemId = Id;
		Result.SlotIndex = Slot;
		return Result;
	};
	TestTrue(TEXT("Equip initial two slots"), Build->ApplyEquipmentSelection(
		         {Selection(TEXT("Head.T1")), Selection(TEXT("Body.T1"), 1)}, Data));
	ANF_ArmorBase* Head = Armors->GetEquippedArmor(ENF_ArmorSlot::HeadArmor);
	ANF_ArmorBase* Body = Armors->GetEquippedArmor(ENF_ArmorSlot::BodyArmor);
	if (!TestNotNull(TEXT("Head equipped"), Head) || !TestNotNull(TEXT("Body equipped"), Body)) return false;
	TestTrue(TEXT("Selected T1 applied"), Head->GetArmorData() == T1);
	TestTrue(TEXT("Unchanged selection accepted"), Build->ApplyEquipmentSelection(
		         {Selection(TEXT("Head.T1")), Selection(TEXT("Body.T1"), 1)}, Data));
	TestTrue(TEXT("Unchanged actor retained"), Head == Armors->GetEquippedArmor(ENF_ArmorSlot::HeadArmor));
	TestTrue(TEXT("Shared class tier replacement"), Build->ApplyEquipmentSelection(
		         {Selection(TEXT("Head.T2")), Selection(TEXT("Body.T1"), 1)}, Data));
	Head = Armors->GetEquippedArmor(ENF_ArmorSlot::HeadArmor);
	if (!TestNotNull(TEXT("T2 equipped"), Head)) return false;
	TestTrue(TEXT("T2 uses chosen source"), Head->GetArmorData() == T2);
	TestEqual(TEXT("T2 defence reflected"), Armors->GetTotalBaseDefence(), 50.f);
	TestTrue(TEXT("Other slot retained"), Body == Armors->GetEquippedArmor(ENF_ArmorSlot::BodyArmor));

	UNF_WeaponDataAsset* WeaponSource = NewObject<UNF_WeaponDataAsset>(Rules);
	WeaponSource->WeaponClass = ANF_WeaponBase::StaticClass();
	Rules->ItemSources.Add(TEXT("Weapon.T2"), WeaponSource);
	FNF_BuildItemData WeaponData;
	TestTrue(TEXT("Resolve weapon source"), Rules->ResolveItemData(TEXT("Weapon.T2"), WeaponData));
	TestTrue(TEXT("Exact weapon source retained"), WeaponData.WeaponData == WeaponSource);
	Data.Add(TEXT("Weapon.T2"), WeaponData);
	ANF_WeaponBase* Probe = World->SpawnActor<ANF_WeaponBase>();
	TestFalse(
		TEXT("No receiver cannot silently apply wrong tier"),
		FNF_BuildEquipmentAdapter::ApplyWeaponData(Probe, WeaponData));
	if (Probe) Probe->Destroy();
	// No weapon component: failure occurs after replacing the head and removing the body.
	TestFalse(TEXT("Weapon failure rejects equipment transaction"), Build->ApplyEquipmentSelection(
		          {Selection(TEXT("Head.T1")), Selection(TEXT("Weapon.T2"))}, Data));
	TestTrue(TEXT("Previous head actor restored"), Head == Armors->GetEquippedArmor(ENF_ArmorSlot::HeadArmor));
	TestTrue(TEXT("Removed body restored"), Body == Armors->GetEquippedArmor(ENF_ArmorSlot::BodyArmor));
	TestEqual(TEXT("Defence restored"), Armors->GetTotalBaseDefence(), 50.f);
	TestTrue(TEXT("Remove one slot"), Build->ApplyEquipmentSelection({Selection(TEXT("Head.T2"))}, Data));
	TestNull(TEXT("Body unequipped"), Armors->GetEquippedArmor(ENF_ArmorSlot::BodyArmor));
	TestTrue(TEXT("Head retained after body removal"), Head == Armors->GetEquippedArmor(ENF_ArmorSlot::HeadArmor));
	Build->ReleaseBuildArmors();
	TestNull(TEXT("Cleanup unequips owned armor"), Armors->GetEquippedArmor(ENF_ArmorSlot::HeadArmor));
	return true;
}

#endif
