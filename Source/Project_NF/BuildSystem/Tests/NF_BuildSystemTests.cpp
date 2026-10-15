#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "BuildSystem/Player/NF_BuildComponent.h"
#include "BuildSystem/Player/NF_BuildInputComponent.h"
#include "BuildSystem/Core/NF_BuildValidator.h"
#include "BuildSystem/Data/NF_BuildItemDefinition.h"
#include "BuildSystem/Data/NF_BuildRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GAS/Tags/NFTag_Build.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"

namespace
{
	UNF_BuildItemDefinition* MakeItem(UNF_BuildRules* Rules, FName Id, ENF_BuildItemType Type, int32 Cost)
	{
		UNF_BuildItemDefinition* Item = NewObject<UNF_BuildItemDefinition>(Rules);
		Item->ItemId = Id;
		Item->ItemType = Type;
		Item->Cost = Cost;
		Rules->Catalog.Add(Item);
		return Item;
	}

	FNF_BuildSelection Select(FName Id, int32 SlotIndex = 0)
	{
		FNF_BuildSelection Selection;
		Selection.ItemId = Id;
		Selection.SlotIndex = SlotIndex;
		return Selection;
	}

	UNF_BuildRules* MakeRules()
	{
		UNF_BuildRules* Rules = NewObject<UNF_BuildRules>();
		Rules->CostLimitsByLevel = {8, 12};
		MakeItem(Rules, TEXT("Rifle"), ENF_BuildItemType::Weapon, 2);
		MakeItem(Rules, TEXT("Grenade"), ENF_BuildItemType::ActiveSkill, 2);
		MakeItem(Rules, TEXT("Dash"), ENF_BuildItemType::ActiveSkill, 4);
		MakeItem(Rules, TEXT("HealthUp"), ENF_BuildItemType::PassiveSkill, 1);
		return Rules;
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildWeaponDefinitionTest, "NF.Build.WeaponDefinition",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildWeaponDefinitionTest::RunTest(const FString& Parameters)
{
	UNF_BuildRules* Rules = MakeRules();
	Rules->Catalog[0]->WeaponClass = ANF_WeaponBase::StaticClass();
	TestEqual(TEXT("Linked weapon in first slot accepted"),
		FNF_BuildValidator::Validate(Rules, 1, {Select(TEXT("Rifle"))}).Result,
		ENF_BuildResult::Success);
	Rules->WeaponSlots = 2;
	TestEqual(TEXT("Shared weapon component has no second equipment slot"),
		FNF_BuildValidator::Validate(Rules, 1, {Select(TEXT("Rifle"), 1)}).Result,
		ENF_BuildResult::InvalidSlot);
	Rules->Catalog[1]->WeaponClass = ANF_WeaponBase::StaticClass();
	TestEqual(TEXT("Skill cannot spawn a weapon"),
		FNF_BuildValidator::Validate(Rules, 1, {Select(TEXT("Rifle")), Select(TEXT("Grenade"))}).Result,
		ENF_BuildResult::InvalidGrant);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildCostTest, "NF.Build.CostAndLevel",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildCostTest::RunTest(const FString& Parameters)
{
	UNF_BuildRules* Rules = MakeRules();
	const TArray<FNF_BuildSelection> ExactLimit = {
		Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Dash"), 1)
	};
	FNF_BuildValidation Result = FNF_BuildValidator::Validate(Rules, 1, ExactLimit);
	TestEqual(TEXT("Exact cost limit accepted"), Result.Result, ENF_BuildResult::Success);
	TestEqual(TEXT("Unified cost includes weapon and skills"), Result.TotalCost, 8);
	TArray<FNF_BuildSelection> OverLimit = ExactLimit;
	OverLimit.Add(Select(TEXT("HealthUp")));
	TestEqual(TEXT("Over limit rejected"), FNF_BuildValidator::Validate(Rules, 1, OverLimit).Result,
	          ENF_BuildResult::CostLimitExceeded);
	TestEqual(TEXT("Higher level unlocks larger budget"), FNF_BuildValidator::Validate(Rules, 2, OverLimit).Result,
	          ENF_BuildResult::Success);
	TestEqual(TEXT("Unconfigured level rejected"), FNF_BuildValidator::Validate(Rules, 3, ExactLimit).Result,
	          ENF_BuildResult::InvalidRules);
	TestEqual(TEXT("Invalid level rejected"), FNF_BuildValidator::Validate(Rules, 0, ExactLimit).Result,
	          ENF_BuildResult::InvalidRules);

	Rules->CostLimitsByLevel[0] = MAX_int32;
	Rules->Catalog[0]->Cost = MAX_int32;
	Rules->Catalog[1]->Cost = MAX_int32;
	TestEqual(TEXT("Cost sum cannot wrap to a cheap build"),
	          FNF_BuildValidator::Validate(Rules, 1, {Select(TEXT("Rifle")), Select(TEXT("Grenade"))}).Result,
	          ENF_BuildResult::CostLimitExceeded);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildSlotTest, "NF.Build.SlotsAndCatalog",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildSlotTest::RunTest(const FString& Parameters)
{
	UNF_BuildRules* Rules = MakeRules();
	auto Check = [this, Rules](const TCHAR* Name, TArray<FNF_BuildSelection> Build, ENF_BuildResult Expected)
	{
		TestEqual(Name, FNF_BuildValidator::Validate(Rules, 1, Build).Result, Expected);
	};
	Check(TEXT("Weapon required"), {Select(TEXT("Grenade"))}, ENF_BuildResult::RequiredWeaponMissing);
	Check(TEXT("Unknown client ID rejected"), {Select(TEXT("FakeItem"))}, ENF_BuildResult::UnknownItem);
	Check(TEXT("Same item cannot fill two slots"),
	      {Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Grenade"), 1)}, ENF_BuildResult::DuplicateItem);
	Check(TEXT("Two skills cannot occupy one slot"),
	      {Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Dash"))}, ENF_BuildResult::InvalidSlot);
	Check(TEXT("Out of range slot rejected"), {Select(TEXT("Rifle")), Select(TEXT("Grenade"), 2)},
	      ENF_BuildResult::InvalidSlot);
	Check(TEXT("Negative slot rejected"), {Select(TEXT("Rifle"), -1)}, ENF_BuildResult::InvalidSlot);
	Check(TEXT("Hostile slot value cannot overflow"), {Select(TEXT("Rifle"), MAX_int32)}, ENF_BuildResult::InvalidSlot);
	MakeItem(Rules, TEXT("Rifle"), ENF_BuildItemType::Weapon, 1);
	Check(TEXT("Duplicate catalog ID rejected"), {Select(TEXT("Rifle"))}, ENF_BuildResult::InvalidRules);
	Rules->Catalog.Pop();
	Rules->Catalog[0]->Cost = -1;
	Check(TEXT("Negative cost data rejected"), {Select(TEXT("Rifle"))}, ENF_BuildResult::InvalidRules);
	Rules->Catalog[0]->Cost = 2;
	Rules->Catalog[1]->EquippedEffects.Add(UGameplayEffect::StaticClass());
	Check(TEXT("Instant equipped GE rejected"), {Select(TEXT("Rifle")), Select(TEXT("Grenade"))},
	      ENF_BuildResult::InvalidGrant);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildServerTest, "NF.Build.ServerApplication",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildServerTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	const uint64 SavedFrameCounter = GFrameCounter;
	ON_SCOPE_EXIT
	{
		World->EndPlay(EEndPlayReason::Quit);
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		GFrameCounter = SavedFrameCounter;
	};
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	ACharacter* Player = World->SpawnActor<ACharacter>();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	ANF_BuildShopZone* Shop = World->SpawnActor<ANF_BuildShopZone>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Player);
	Player->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Player, Player);
	UNF_BuildComponent* Build = NewObject<UNF_BuildComponent>(Player);
	Player->AddInstanceComponent(Build);
	Build->RegisterComponent();
	UNF_BuildRules* Rules = MakeRules();
	Rules->Catalog[1]->AbilityClass = UGameplayAbility::StaticClass();
	Rules->Catalog[2]->AbilityClass = UGameplayAbility::StaticClass();
	TestTrue(TEXT("Initialize authoritative ASC"), Build->InitializeAbilitySystem(ASC));
	TestTrue(TEXT("Configure initial rules"), Build->ConfigureRules(Rules));
	// The interaction is server-side; no overlap event is required.
	Player->DispatchBeginPlay();
	Shop->DispatchBeginPlay();
	World->Tick(LEVELTICK_All, 1.f / 60.f);
	TestTrue(TEXT("Player in interaction distance"), Shop->CanBuild(Player));
	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Grenade"))}, Shop);
	TestEqual(TEXT("Proximity without interaction cannot apply a build"), Build->GetCurrentBuild().Num(), 0);
	Shop->NF_Interaction(Controller);
	TestTrue(TEXT("Interface interaction opens authorized session"), Build->GetCurrentShop() == Shop);
	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Grenade"))}, Shop);
	TestEqual(TEXT("Initial build committed"), Build->GetCurrentBuild().Num(), 2);
	TestEqual(TEXT("One ability granted"), ASC->GetActivatableAbilities().Num(), 1);
	if (Build->GetCurrentBuild().Num() != 2 || ASC->GetActivatableAbilities().Num() != 1)
	{
		AddInfo(FString::Printf(TEXT("Fixture authority=%d distance=%d preview=%d"), Player->HasAuthority(),
		                        Shop->CanBuild(Player),
		                        static_cast<int32>(Build->PreviewBuild({
			                        Select(TEXT("Rifle")), Select(TEXT("Grenade"))
		                        }).Result)));
		return false;
	}
	FGameplayAbilitySpecHandle OriginalHandle;
	if (!ASC->GetActivatableAbilities().IsEmpty())
	{
		OriginalHandle = ASC->GetActivatableAbilities()[0].Handle;
	}
	TestTrue(TEXT("Active skill selection waits for a slot"), Build->SelectDraftItem(TEXT("Grenade")));
	TestEqual(TEXT("Pending skill is exposed to UI"), Build->GetPendingActiveSkill(), FName(TEXT("Grenade")));
	TestTrue(TEXT("Pending skill can move to slot two"), Build->AssignSelectedActiveSkill(1));
	TestTrue(TEXT("Pending choice cleared after assignment"), Build->GetPendingActiveSkill().IsNone());
	TestEqual(TEXT("Draft slot movement does not change equipped slot"), Build->GetCurrentBuild()[1].SlotIndex, 0);
	// Use a retained copy: GetDraftBuild returns by value.
	const TArray<FNF_BuildSelection> Draft = Build->GetDraftBuild();
	const FNF_BuildSelection* DraftGrenade = Draft.FindByPredicate([](const FNF_BuildSelection& Selection)
	{
		return Selection.ItemId == FName(TEXT("Grenade"));
	});
	TestTrue(TEXT("Draft has slot two assignment"), DraftGrenade && DraftGrenade->SlotIndex == 1);

	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Grenade"), 1)}, Shop);
	TestNotNull(TEXT("Moving skill slot preserves original handle"), ASC->FindAbilitySpecFromHandle(OriginalHandle));
	if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(OriginalHandle))
	{
		TestEqual(TEXT("Input follows new slot"), Spec->InputID, 2);
	}
	Build->RequestApplyBuild(
		{Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Dash"), 1), Select(TEXT("HealthUp"))}, Shop);
	TestEqual(TEXT("Over budget request preserves build"), Build->GetCurrentBuild().Num(), 2);
	ASC->AddLooseGameplayTag(NFTag_Build::State_Build_Locked);
	Build->RequestApplyBuild({Select(TEXT("Rifle"))}, Shop);
	TestEqual(TEXT("Locked state preserves build"), Build->GetCurrentBuild().Num(), 2);
	ASC->RemoveLooseGameplayTag(NFTag_Build::State_Build_Locked);
	Shop->SetShopEnabled(false);
	Build->RequestApplyBuild({Select(TEXT("Rifle"))}, Shop);
	TestEqual(TEXT("Disabled shop preserves build"), Build->GetCurrentBuild().Num(), 2);
	Shop->SetShopEnabled(true);
	Player->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	Build->RequestApplyBuild({Select(TEXT("Rifle"))}, Shop);
	TestEqual(TEXT("Outside shop preserves build"), Build->GetCurrentBuild().Num(), 2);
	Player->SetActorLocation(FVector::ZeroVector);
	World->Tick(LEVELTICK_All, 1.f / 60.f);

	// Temporarily make the engine's plain GE Infinite with an invalid modifier, then restore its CDO.
	UGameplayEffect* Effect = GetMutableDefault<UGameplayEffect>();
	const EGameplayEffectDurationType SavedDuration = Effect->DurationPolicy;
	const TArray<FGameplayModifierInfo> SavedModifiers = Effect->Modifiers;
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	Effect->Modifiers.AddDefaulted();
	Rules->Catalog[2]->EquippedEffects.Add(UGameplayEffect::StaticClass());
	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Dash"))}, Shop);
	Effect->DurationPolicy = SavedDuration;
	Effect->Modifiers = SavedModifiers;
	Rules->Catalog[2]->EquippedEffects.Empty();
	TestNotNull(TEXT("Failed replacement keeps original ability"), ASC->FindAbilitySpecFromHandle(OriginalHandle));
	TestEqual(TEXT("Failed replacement cleans staged ability"), ASC->GetActivatableAbilities().Num(), 1);
	TestEqual(TEXT("Failed replacement keeps original selection"), Build->GetCurrentBuild()[1].ItemId,
	          FName(TEXT("Grenade")));

	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Dash"))}, Shop);
	TestNull(TEXT("Replacement revokes only previous build ability"), ASC->FindAbilitySpecFromHandle(OriginalHandle));
	TestEqual(TEXT("Replacement selection committed"), Build->GetCurrentBuild()[1].ItemId, FName(TEXT("Dash")));
	TestFalse(TEXT("Rules cannot change while a build is equipped"), Build->ConfigureRules(MakeRules()));
	TestFalse(TEXT("Unconfigured growth level rejected"), Build->SetPlayerLevel(3));
	TestTrue(TEXT("Configured growth level accepted"), Build->SetPlayerLevel(2));
	World->Tick(LEVELTICK_All, 0.25f);
	TestTrue(TEXT("거리 안의 상점을 조회할 수 있다"), Build->GetShopsInRange().Contains(Shop));
	Build->CloseBuildSession();
	TestTrue(TEXT("상점을 닫아도 범위 안 상태는 유지된다"), Build->GetShopsInRange().Contains(Shop));
	Player->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	World->Tick(LEVELTICK_All, 0.25f);
	TestFalse(TEXT("사용 세션이 없어도 범위 이탈을 감지한다"), Build->GetShopsInRange().Contains(Shop));
	Player->SetActorLocation(FVector::ZeroVector);
	World->Tick(LEVELTICK_All, 0.25f);
	TestTrue(TEXT("상호작용 전에도 범위 재진입을 감지한다"), Build->GetShopsInRange().Contains(Shop));
	Build->RequestApplyBuild({Select(TEXT("Rifle"))}, Shop);
	TestEqual(TEXT("Closed session cannot apply another build"), Build->GetCurrentBuild().Num(), 2);
	Shop->NF_Interaction(Controller);
	Player->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	World->Tick(LEVELTICK_All, 0.25f);
	TestNull(TEXT("Leaving distance closes session"), Build->GetCurrentShop());

	UNF_BuildInputComponent* Input = NewObject<UNF_BuildInputComponent>(Controller);
	TestEqual(TEXT("Slot one defaults to keyboard 1"), Input->GetSlotKey(0), EKeys::One);
	TestEqual(TEXT("Slot two defaults to keyboard 2"), Input->GetSlotKey(1), EKeys::Two);
	TestFalse(TEXT("Duplicate slot key rejected"), Input->SetSlotKey(0, EKeys::Two));
	TestFalse(TEXT("Invalid slot rejected"), Input->SetSlotKey(2, EKeys::Q));
	TestFalse(TEXT("Escape is reserved for closing UI"), Input->SetSlotKey(0, EKeys::Escape));
	TestTrue(TEXT("Digital key can be rebound"), Input->SetSlotKey(0, EKeys::Q));
	TestEqual(TEXT("Key binding updated locally"), Input->GetSlotKey(0), EKeys::Q);

	return true;
}

#endif
