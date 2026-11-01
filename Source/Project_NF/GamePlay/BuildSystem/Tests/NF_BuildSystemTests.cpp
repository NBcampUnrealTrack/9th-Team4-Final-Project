#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystemComponent.h"
#include "GamePlay/BuildSystem/Tests/NF_BuildTestItemSource.h"
#include "Abilities/GameplayAbility.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GamePlay/BuildSystem/Player/NF_BuildInputComponent.h"
#include "GamePlay/BuildSystem/Core/NF_BuildValidator.h"
#include "GamePlay/BuildSystem/Data/NF_BuildItemDefinitionDataAsset.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GAS/Tags/NFTag_Build.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GameplayEffect.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "Core/NF_GameState.h"
#include "Core/NF_GameMode.h"
#include "Core/Component/NF_BuildReadyComponent.h"
#include "Player/State/NF_PlayerState.h"
#include "Core/NF_CoreIntegrationTypes.h"
#include "UObject/UnrealType.h"

namespace
{
	// @Owner: 황준현 | 2026-10-20. 테스트 전용 권한 목록이며 실제 게임의 소유권을 우회하지 않는다.
	TArray<FName> MakeAllowedItemIds(const UNF_BuildRules* Rules)
	{
		TArray<FName> Result;
		if (IsValid(Rules))
		{
			for (const UNF_BuildItemDefinitionDataAsset* Definition : Rules->Catalog)
			{
				if (IsValid(Definition)) Result.AddUnique(Definition->ItemId);
			}
		}
		return Result;
	}

	UNF_BuildItemDefinitionDataAsset* MakeItem(UNF_BuildRules* Rules, FName Id, ENF_ItemType Type, int32 Cost)
	{
		UNF_BuildItemDefinitionDataAsset* Item = NewObject<UNF_BuildItemDefinitionDataAsset>(Rules);
		Item->ItemId = Id;
		UNF_BuildTestItemSource* Source = NewObject<UNF_BuildTestItemSource>(Rules);
		Source->Data.ItemType = Type;
		Source->Data.Cost = Cost;
		Rules->Catalog.Add(Item);
		Rules->ItemSources.Add(Id, Source);
		return Item;
	}

	FNF_BuildItemData& SourceData(UNF_BuildRules* Rules, const UNF_BuildItemDefinitionDataAsset* Item)
	{
		return CastChecked<UNF_BuildTestItemSource>(Rules->ItemSources.FindChecked(Item->ItemId).Get())->Data;
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
		MakeItem(Rules, TEXT("Rifle"), ENF_ItemType::Weapon, 2);
		MakeItem(Rules, TEXT("Grenade"), ENF_ItemType::ActiveSkill, 2);
		MakeItem(Rules, TEXT("Dash"), ENF_ItemType::ActiveSkill, 4);
		MakeItem(Rules, TEXT("HealthUp"), ENF_ItemType::PassiveSkill, 1);
		return Rules;
	}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildWeaponDefinitionTest, "NF.Build.WeaponDefinition",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildWeaponDefinitionTest::RunTest(const FString& Parameters)
{
	UNF_BuildRules* Rules = MakeRules();
	UNF_WeaponDataAsset* WeaponSource = NewObject<UNF_WeaponDataAsset>(Rules);
	WeaponSource->WeaponClass = ANF_WeaponBase::StaticClass();
	WeaponSource->Cost = 2;
	Rules->ItemSources.Add(TEXT("Rifle"), WeaponSource);
	TestEqual(TEXT("Linked weapon in first slot accepted"),
	          FNF_BuildValidator::Validate(Rules, 8, MakeAllowedItemIds(Rules), {Select(TEXT("Rifle"))}).Result,
	          ENF_BuildResult::Success);
	TestEqual(TEXT("Shared weapon component has no second equipment slot"),
	          FNF_BuildValidator::Validate(Rules, 8, MakeAllowedItemIds(Rules), {Select(TEXT("Rifle"), 1)}).Result,
	          ENF_BuildResult::InvalidSlot);
	SourceData(Rules, Rules->Catalog[1]).WeaponClass = ANF_WeaponBase::StaticClass();
	TestEqual(TEXT("Skill cannot spawn a weapon"),
	          FNF_BuildValidator::Validate(Rules, 8, MakeAllowedItemIds(Rules),
	                                       {Select(TEXT("Rifle")), Select(TEXT("Grenade"))}).Result,
	          ENF_BuildResult::InvalidGrant);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildArmorSlotsTest, "NF.Build.ArmorSlots",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildArmorSlotsTest::RunTest(const FString& Parameters)
{
	UNF_BuildRules* Rules = MakeRules();
	UNF_BuildItemDefinitionDataAsset* Head = MakeItem(Rules, TEXT("Helmet"), ENF_ItemType::Armor, 1);
	SourceData(Rules, Head).ArmorSlot = ENF_ArmorSlot::HeadArmor;
	UNF_BuildItemDefinitionDataAsset* Upper = MakeItem(Rules, TEXT("Vest"), ENF_ItemType::Armor, 1);
	SourceData(Rules, Upper).ArmorSlot = ENF_ArmorSlot::BodyArmor;
	UNF_BuildItemDefinitionDataAsset* Lower = MakeItem(Rules, TEXT("Legs"), ENF_ItemType::Armor, 1);
	SourceData(Rules, Lower).ArmorSlot = ENF_ArmorSlot::LegArmor;
	const TArray<FNF_BuildSelection> FullBuild = {
		Select(TEXT("Rifle")), Select(TEXT("Helmet"), 0), Select(TEXT("Vest"), 1), Select(TEXT("Legs"), 2),
		Select(TEXT("Grenade"), 0), Select(TEXT("Dash"), 1), Select(TEXT("HealthUp"))
	};
	TestEqual(TEXT("일곱 슬롯을 올바른 분류로 채울 수 있다"),
	          FNF_BuildValidator::Validate(Rules, 12, MakeAllowedItemIds(Rules), FullBuild).Result,
	          ENF_BuildResult::Success);
	TestEqual(TEXT("상의는 머리 슬롯에 넣을 수 없다"),
	          FNF_BuildValidator::Validate(Rules, 12, MakeAllowedItemIds(Rules),
	                                       {Select(TEXT("Rifle")), Select(TEXT("Vest"), 0)}).Result,
	          ENF_BuildResult::InvalidSlot);
	TestEqual(TEXT("패시브는 두 번째 슬롯을 사용할 수 없다"),
	          FNF_BuildValidator::Validate(Rules, 12, MakeAllowedItemIds(Rules),
	                                       {Select(TEXT("Rifle")), Select(TEXT("HealthUp"), 1)}).Result,
	          ENF_BuildResult::InvalidSlot);
	UNF_BuildItemDefinitionDataAsset* OtherHead = MakeItem(Rules, TEXT("OtherHelmet"), ENF_ItemType::Armor, 1);
	SourceData(Rules, OtherHead).ArmorSlot = ENF_ArmorSlot::HeadArmor;
	TestEqual(TEXT("같은 부위 두 개의 동시 장착을 거부한다"),
	          FNF_BuildValidator::Validate(Rules, 12, MakeAllowedItemIds(Rules),
	                                       {
		                                       Select(TEXT("Rifle")), Select(TEXT("Helmet")),
		                                       Select(TEXT("OtherHelmet"))
	                                       }).Result,
	          ENF_BuildResult::InvalidSlot);
	SourceData(Rules, Head).ArmorSlot = ENF_ArmorSlot::None;
	TestEqual(TEXT("방어구 부위 미지정 데이터를 거부한다"),
	          FNF_BuildValidator::Validate(Rules, 12, MakeAllowedItemIds(Rules), {Select(TEXT("Rifle"))}).Result,
	          ENF_BuildResult::InvalidRules);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildCostTest, "NF.Build.CostAndEntitlements",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildCostTest::RunTest(const FString& Parameters)
{
	UNF_BuildRules* Rules = MakeRules();
	const TArray<FNF_BuildSelection> ExactLimit = {
		Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Dash"), 1)
	};
	FNF_BuildValidation Result = FNF_BuildValidator::Validate(Rules, 8, MakeAllowedItemIds(Rules), ExactLimit);
	TestEqual(TEXT("Exact cost limit accepted"), Result.Result, ENF_BuildResult::Success);
	TestEqual(TEXT("Unified cost includes weapon and skills"), Result.TotalCost, 8);
	TArray<FNF_BuildSelection> OverLimit = ExactLimit;
	OverLimit.Add(Select(TEXT("HealthUp")));
	TestEqual(
		TEXT("Over limit rejected"),
		FNF_BuildValidator::Validate(Rules, 8, MakeAllowedItemIds(Rules), OverLimit).Result,
		ENF_BuildResult::CostLimitExceeded);
	TestEqual(
		TEXT("서버 상한이 증가하면 더 큰 빌드 허용"),
		FNF_BuildValidator::Validate(Rules, 12, MakeAllowedItemIds(Rules), OverLimit).Result,
		ENF_BuildResult::Success);
	TestEqual(TEXT("음수 상한 거절"), FNF_BuildValidator::Validate(Rules, -1, MakeAllowedItemIds(Rules), ExactLimit).Result,
	          ENF_BuildResult::InvalidRules);
	TestEqual(
		TEXT("상한 0에서는 유료 빌드 거절"), FNF_BuildValidator::Validate(Rules, 0, MakeAllowedItemIds(Rules), ExactLimit).Result,
		ENF_BuildResult::CostLimitExceeded);
	TestEqual(TEXT("카탈로그에 있어도 미획득 항목 거절"),
	          FNF_BuildValidator::Validate(Rules, 8, {FName(TEXT("Rifle"))}, ExactLimit).Result,
	          ENF_BuildResult::ItemNotUnlocked);

	SourceData(Rules, Rules->Catalog[0]).Cost = MAX_int32;
	SourceData(Rules, Rules->Catalog[1]).Cost = MAX_int32;
	TestEqual(TEXT("Cost sum cannot wrap to a cheap build"),
	          FNF_BuildValidator::Validate(Rules, MAX_int32, MakeAllowedItemIds(Rules),
	                                       {Select(TEXT("Rifle")), Select(TEXT("Grenade"))}).Result,
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
		TestEqual(Name, FNF_BuildValidator::Validate(Rules, 8, MakeAllowedItemIds(Rules), Build).Result, Expected);
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
	MakeItem(Rules, TEXT("Rifle"), ENF_ItemType::Weapon, 1);
	Check(TEXT("Duplicate catalog ID rejected"), {Select(TEXT("Rifle"))}, ENF_BuildResult::InvalidRules);
	Rules->Catalog.Pop();
	SourceData(Rules, Rules->Catalog[0]).Cost = -1;
	Check(TEXT("Negative cost data rejected"), {Select(TEXT("Rifle"))}, ENF_BuildResult::InvalidRules);
	SourceData(Rules, Rules->Catalog[0]).Cost = 2;
	SourceData(Rules, Rules->Catalog[1]).EquippedEffects.Add(UGameplayEffect::StaticClass());
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
	// 현재 Core 접근 정책에 맞는 서버 테스트 환경을 준비한다.
	ANF_GameState* GameState = World->SpawnActor<ANF_GameState>();
	World->SetGameState(GameState);
	GameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Farming, 60.f);
	FEnumProperty* ShopTypeProperty = FindFProperty<FEnumProperty>(Shop->GetClass(), TEXT("ShopType"));
	if (!TestNotNull(TEXT("상점 종류 설정 속성"), ShopTypeProperty)) return false;
	ShopTypeProperty->GetUnderlyingProperty()->SetIntPropertyValue(
		ShopTypeProperty->ContainerPtrToValuePtr<void>(Shop), static_cast<int64>(ENF_BuildShopType::Field));
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Player);
	Player->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Player, Player);
	UNF_BuildComponent* Build = NewObject<UNF_BuildComponent>(Player);
	Player->AddInstanceComponent(Build);
	Build->RegisterComponent();
	UNF_BuildRules* Rules = MakeRules();
	SourceData(Rules, Rules->Catalog[1]).AbilityClass = UGameplayAbility::StaticClass();
	SourceData(Rules, Rules->Catalog[2]).AbilityClass = UGameplayAbility::StaticClass();
	TestTrue(TEXT("Initialize authoritative ASC"), Build->InitializeAbilitySystem(ASC));
	TestTrue(TEXT("Configure initial rules"), Shop->ConfigureRules(Rules));
	TestEqual(TEXT("개인별 조건 초기화 전에는 적용 불가"),
	          Build->PreviewBuild({Select(TEXT("Rifle"))}).Result, ENF_BuildResult::InvalidRules);
	TestTrue(TEXT("서버 테스트 권한 초기화"), Build->UpdateBuildEntitlements(8, MakeAllowedItemIds(Rules)));
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
	TestEqual(TEXT("일반 빌드 성공 후 Ready를 한 번 부여한다"),
	          ASC->GetGameplayTagCount(NFTag_Build::State_Build_Ready), 1);
	UNF_BuildItemDefinitionDataAsset* HeadItem = MakeItem(Rules, TEXT("TestHead"), ENF_ItemType::Armor, 0);
	SourceData(Rules, HeadItem).ArmorSlot = ENF_ArmorSlot::HeadArmor;
	UNF_BuildItemDefinitionDataAsset* UpperItem = MakeItem(Rules, TEXT("TestUpper"), ENF_ItemType::Armor, 0);
	SourceData(Rules, UpperItem).ArmorSlot = ENF_ArmorSlot::BodyArmor;
	UNF_BuildItemDefinitionDataAsset* ReplacementHead = MakeItem(Rules, TEXT("TestHead2"), ENF_ItemType::Armor, 0);
	SourceData(Rules, ReplacementHead).ArmorSlot = ENF_ArmorSlot::HeadArmor;
	TestTrue(TEXT("추가 방어구의 획득 권한 갱신"), Build->UpdateBuildEntitlements(8, MakeAllowedItemIds(Rules)));
	TestTrue(TEXT("머리 방어구 선택"), Build->SelectDraftItem(TEXT("TestHead")));
	TestTrue(TEXT("상의 방어구 선택"), Build->SelectDraftItem(TEXT("TestUpper")));
	TestTrue(TEXT("같은 부위 교체"), Build->SelectDraftItem(TEXT("TestHead2")));
	TestTrue(TEXT("머리 칸만 교체된다"), Build->GetSlotItem(ENF_ItemType::Armor, 0, true) == ReplacementHead);
	TestTrue(TEXT("상의 칸은 유지된다"), Build->GetSlotItem(ENF_ItemType::Armor, 1, true) == UpperItem);
	TestNull(TEXT("빈 하의 칸은 nullptr"), Build->GetSlotItem(ENF_ItemType::Armor, 2, true));
	TestNull(TEXT("임시 방어구는 확정 상태창에 반영되지 않는다"), Build->GetSlotItem(ENF_ItemType::Armor, 0, false));
	TestEqual(TEXT("부위별 카탈로그 필터"), Build->GetCatalogItems(ENF_ItemType::Armor, ENF_ArmorSlot::HeadArmor).Num(), 2);
	Build->RemoveDraftItem(TEXT("TestHead2"));
	Build->RemoveDraftItem(TEXT("TestUpper"));
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
		TestEqual(TEXT("두 번째 슬롯의 InputID는 1"), Spec->InputID, 1);
	}
	Build->RequestApplyBuild(
		{Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Dash"), 1), Select(TEXT("HealthUp"))}, Shop);
	TestEqual(TEXT("Over budget request preserves build"), Build->GetCurrentBuild().Num(), 2);
	TestEqual(TEXT("반복 확정과 실패 요청은 Ready 카운트를 증가시키거나 제거하지 않는다"),
	          ASC->GetGameplayTagCount(NFTag_Build::State_Build_Ready), 1);
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
	SourceData(Rules, Rules->Catalog[2]).EquippedEffects.Add(UGameplayEffect::StaticClass());
	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Dash"))}, Shop);
	Effect->DurationPolicy = SavedDuration;
	Effect->Modifiers = SavedModifiers;
	SourceData(Rules, Rules->Catalog[2]).EquippedEffects.Empty();
	TestNotNull(TEXT("Failed replacement keeps original ability"), ASC->FindAbilitySpecFromHandle(OriginalHandle));
	TestEqual(TEXT("Failed replacement cleans staged ability"), ASC->GetActivatableAbilities().Num(), 1);
	TestEqual(TEXT("Failed replacement keeps original selection"), Build->GetCurrentBuild()[1].ItemId,
	          FName(TEXT("Grenade")));

	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Dash"))}, Shop);
	TestNull(TEXT("Replacement revokes only previous build ability"), ASC->FindAbilitySpecFromHandle(OriginalHandle));
	TestEqual(TEXT("Replacement selection committed"), Build->GetCurrentBuild()[1].ItemId, FName(TEXT("Dash")));
	TestFalse(TEXT("Rules cannot change while a build is equipped"), Shop->ConfigureRules(MakeRules()));
	TestFalse(TEXT("음수 코스트 상한 갱신 거절"), Build->UpdateBuildEntitlements(-1, MakeAllowedItemIds(Rules)));
	TestTrue(TEXT("외부 서버에서 코스트 상한 갱신"), Build->UpdateBuildEntitlements(12, MakeAllowedItemIds(Rules)));
	World->Tick(LEVELTICK_All, 0.25f);
	TestTrue(TEXT("거리 안의 상점을 조회할 수 있다"), Build->GetShopsInRange().Contains(Shop));
	TestTrue(TEXT("개인 해금 목록을 공용 카탈로그와 별도로 갱신한다"),
	         Build->UpdateBuildEntitlements(12, {FName(TEXT("Rifle"))}));
	TestEqual(TEXT("공용 DA에 있어도 미해금 액티브 스킬은 표시하지 않는다"),
	          Build->GetCatalogItems(ENF_ItemType::ActiveSkill, ENF_ArmorSlot::None).Num(), 0);
	TestTrue(TEXT("개인 해금 조건 복원"), Build->UpdateBuildEntitlements(12, MakeAllowedItemIds(Rules)));
	ANF_BuildShopZone* OtherShop = World->SpawnActor<ANF_BuildShopZone>();
	ShopTypeProperty->GetUnderlyingProperty()->SetIntPropertyValue(
		ShopTypeProperty->ContainerPtrToValuePtr<void>(OtherShop), static_cast<int64>(ENF_BuildShopType::Field));
	UNF_BuildRules* OtherRules = MakeRules();
	TestTrue(TEXT("다른 상점은 자신의 Rules를 설정한다"), OtherShop->ConfigureRules(OtherRules));
	TestTrue(TEXT("다른 상점 연결"), Build->BeginBuildSession(OtherShop));
	TestTrue(TEXT("플레이어는 현재 상점의 Rules를 조회한다"), Build->GetRules() == OtherRules);
	// 같은 ID에 다른 정의/원본을 연결한 상점은 기존 GAS를 잘못 재사용하지 못한다.
	const TArray<FNF_BuildSelection> BeforeShopChange = Build->GetCurrentBuild();
	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Dash"), 1)}, OtherShop);
	TestEqual(TEXT("다른 원본을 가진 동일 ID 교체는 기존 빌드를 유지한다"),
	          Build->GetCurrentBuild()[1].SlotIndex, BeforeShopChange[1].SlotIndex);
	TestTrue(TEXT("기존 상점으로 복귀"), Build->BeginBuildSession(Shop));
	Build->CloseBuildSession();
	TestNull(TEXT("상점을 닫으면 규칙 연결을 해제한다"), Build->GetRules());
	TestNotNull(TEXT("상점을 닫아도 장착 항목 정의가 유지된다"), Build->FindItemDefinition(TEXT("Dash")));
	FNF_BuildItemData EquippedData;
	TestTrue(TEXT("상점을 닫아도 장착 스킬 데이터가 유지된다"), Build->ResolveItemData(TEXT("Dash"), EquippedData));
	TestNotNull(TEXT("상점을 닫아도 장착 원본이 유지된다"), Build->GetItemSource(TEXT("Dash")));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildFallbackTest, "NF.Build.FallbackApplication",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildFallbackTest::RunTest(const FString& Parameters)
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
	ANF_GameState* GameState = World->SpawnActor<ANF_GameState>();
	World->SetGameState(GameState);
	ACharacter* Player = World->SpawnActor<ACharacter>();
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Player);
	Player->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Player, Player);
	UNF_BuildComponent* Build = NewObject<UNF_BuildComponent>(Player);
	Player->AddInstanceComponent(Build);
	Build->RegisterComponent();
	UNF_BuildRules* Rules = MakeRules();
	ANF_BuildShopZone* Shop = World->SpawnActor<ANF_BuildShopZone>();
	ANF_PlayerState* PlayerState = World->SpawnActor<ANF_PlayerState>();
	PlayerState->NF_TeamID = 1;
	FObjectProperty* PlayerStateProperty = FindFProperty<FObjectProperty>(APawn::StaticClass(), TEXT("PlayerState"));
	if (!TestNotNull(TEXT("플레이어 상태 속성"), PlayerStateProperty)) return false;
	PlayerStateProperty->SetObjectPropertyValue_InContainer(Player, PlayerState);
	FEnumProperty* TypeProperty = FindFProperty<FEnumProperty>(Shop->GetClass(), TEXT("ShopType"));
	FIntProperty* TeamProperty = FindFProperty<FIntProperty>(Shop->GetClass(), TEXT("OwnerTeamId"));
	if (!TestNotNull(TEXT("상점 종류"), TypeProperty) || !TestNotNull(TEXT("상점 팀"), TeamProperty)) return false;
	TypeProperty->GetUnderlyingProperty()->SetIntPropertyValue(
		TypeProperty->ContainerPtrToValuePtr<void>(Shop), static_cast<int64>(ENF_BuildShopType::TeamBase));
	TeamProperty->SetPropertyValue_InContainer(Shop, 1);
	Rules->FallbackBuild = {Select(TEXT("Rifle")), Select(TEXT("Grenade"))};
	TestTrue(TEXT("테스트 ASC 연결"), Build->InitializeAbilitySystem(ASC));
	TestTrue(TEXT("기본 구성 설정"), Shop->ConfigureRules(Rules));
	TestTrue(TEXT("기본 빌드 아이템도 서버 권한 목록에 포함"),
	         Build->UpdateBuildEntitlements(8, MakeAllowedItemIds(Rules)));
	TestFalse(TEXT("Build 외 페이즈에서 기본 적용을 거부한다"), Build->ApplyFallbackBuildForMatchStart(Shop));
	GameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Build, 10.f);
	Rules->FallbackBuild = {
		Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Dash"), 1), Select(TEXT("HealthUp"))
	};
	TestFalse(TEXT("기본 빌드도 코스트 초과를 거부한다"), Build->ApplyFallbackBuildForMatchStart(Shop));
	TestFalse(TEXT("실패 시 Ready를 부여하지 않는다"), ASC->HasMatchingGameplayTag(NFTag_Build::State_Build_Ready));
	Rules->FallbackBuild = {Select(TEXT("Rifle")), Select(TEXT("Grenade"))};
	ASC->AddLooseGameplayTag(NFTag_Build::State_Build_Locked);
	TestFalse(TEXT("기본 적용도 빌드 잠금을 유지한다"), Build->ApplyFallbackBuildForMatchStart(Shop));
	ASC->RemoveLooseGameplayTag(NFTag_Build::State_Build_Locked);
	TestTrue(TEXT("상점 세션 없이 유효한 기본 구성을 확정한다"), Build->ApplyFallbackBuildForMatchStart(Shop));
	TestEqual(TEXT("기본 구성 확정"), Build->GetCurrentBuild().Num(), 2);
	TestTrue(TEXT("반복 기본 요청은 성공 상태를 유지한다"), Build->ApplyFallbackBuildForMatchStart(Shop));
	TestEqual(TEXT("Ready는 누적되지 않는다"), ASC->GetGameplayTagCount(NFTag_Build::State_Build_Ready), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildRegistrationTest, "NF.Build.CoreHandlerRegistration",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildRegistrationTest::RunTest(const FString& Parameters)
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
	ANF_GameMode* GameMode = World->SpawnActor<ANF_GameMode>();
	ANF_PlayerState* PlayerState = World->SpawnActor<ANF_PlayerState>();
	AActor* FirstOwner = World->SpawnActor<AActor>();
	AActor* OtherOwner = World->SpawnActor<AActor>();
	UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
	if (!TestNotNull(TEXT("테스트 PlayerState의 ASC"), ASC)) return false;
	int32 CallCount = 0;
	const FNF_ApplyFallbackBuild Handler = FNF_ApplyFallbackBuild::CreateWeakLambda(
		FirstOwner, [&CallCount](ANF_PlayerState* Target)
		{
			++CallCount;
			Target->GetAbilitySystemComponent()->AddLooseGameplayTag(NFTag_Build::State_Build_Ready);
			return true;
		});
	TestFalse(TEXT("비어 있는 콜백은 등록하지 않는다"), GameMode->RegisterFallbackBuildHandler(FNF_ApplyFallbackBuild()));
	TestTrue(TEXT("Core 공개 API에 콜백 등록"), GameMode->RegisterFallbackBuildHandler(Handler));
	TestFalse(TEXT("기존 등록을 덮어쓰지 않는다"), GameMode->RegisterFallbackBuildHandler(Handler));
	GameMode->UnregisterFallbackBuildHandler(OtherOwner);
	// 테스트에서만 Core 실행부를 호출하여 공개 API가 실제 요청 경로에 연결됐는지 확인한다.
	UNF_BuildReadyComponent* Ready = GameMode->GetBuildReadyComponent();
	Ready->RegisterPlayer(PlayerState);
	TestTrue(TEXT("다른 객체의 해제 요청 뒤에도 Core가 등록된 콜백을 실행한다"), Ready->ResolveUnreadyPlayers(1));
	TestEqual(TEXT("요청은 한 번 실행된다"), CallCount, 1);
	GameMode->UnregisterFallbackBuildHandler(FirstOwner);
	TestTrue(TEXT("등록 소유자가 해제하면 재등록할 수 있다"), GameMode->RegisterFallbackBuildHandler(Handler));
	GameMode->UnregisterFallbackBuildHandler(FirstOwner);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildSourceContractTest, "NF.Build.SourceContract",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildSourceContractTest::RunTest(const FString& Parameters)
{
	UNF_BuildRules* Rules = MakeRules();
	const TArray<FName> Allowed = MakeAllowedItemIds(Rules);
	const TArray<FNF_BuildSelection> Build = {Select(TEXT("Rifle")), Select(TEXT("Grenade"))};
	TMap<FName, FNF_BuildItemData> Snapshot;
	TestEqual(TEXT("원본 비용으로 서버 검증"),
	          FNF_BuildValidator::Validate(Rules, 8, Allowed, Build, &Snapshot).TotalCost, 4);
	const FNF_BuildItemData* GrenadeData = Snapshot.Find(TEXT("Grenade"));
	if (!TestNotNull(TEXT("검증한 선택 항목의 장착용 스냅샷"), GrenadeData)) return false;
	TestEqual(TEXT("장착용 스냅샷도 같은 원본 비용"), GrenadeData->Cost, 2);
	SourceData(Rules, Rules->Catalog[1]).Cost = 7;
	TestEqual(TEXT("원본만 수정하면 기존 ID의 비용 검증에 반영"),
	          FNF_BuildValidator::Validate(Rules, 8, Allowed, Build, &Snapshot).Result,
	          ENF_BuildResult::CostLimitExceeded);
	TestTrue(TEXT("실패 시 장착용 스냅샷을 남기지 않는다"), Snapshot.IsEmpty());

	UDataAsset* Original = Rules->ItemSources.FindChecked(TEXT("Grenade"));
	Rules->ItemSources.Remove(TEXT("Grenade"));
	FNF_BuildItemData Readback;
	Readback.Cost = 99;
	TestFalse(TEXT("원본 누락을 거절"), Rules->ResolveItemData(TEXT("Grenade"), Readback));
	TestEqual(TEXT("실패 시 이전 조회값을 재사용하지 않는다"), Readback.Cost, -1);
	TestEqual(TEXT("미연결 항목을 무료로 통과시키지 않는다"),
	          FNF_BuildValidator::Validate(Rules, 8, Allowed, Build).Result, ENF_BuildResult::InvalidRules);
	Rules->ItemSources.Add(TEXT("Grenade"), NewObject<UDataAsset>(Rules));
	TestFalse(TEXT("인터페이스 미구현 원본 거절"), Rules->ResolveItemData(TEXT("Grenade"), Readback));
	Rules->ItemSources.Add(TEXT("Grenade"), Original);
	CastChecked<UNF_BuildTestItemSource>(Original)->bAvailable = false;
	TestFalse(TEXT("원본이 준비되지 않았으면 거절"), Rules->ResolveItemData(TEXT("Grenade"), Readback));
	CastChecked<UNF_BuildTestItemSource>(Original)->bAvailable = true;
	SourceData(Rules, Rules->Catalog[1]).Cost = 0;
	TestEqual(TEXT("원본에서 명시한 무료 항목은 허용"),
	          FNF_BuildValidator::Validate(Rules, 8, Allowed, Build).Result, ENF_BuildResult::Success);
	SourceData(Rules, Rules->Catalog[1]).ItemType = static_cast<ENF_ItemType>(255);
	TestEqual(TEXT("원본의 잘못된 분류 거절"),
	          FNF_BuildValidator::Validate(Rules, 8, Allowed, Build).Result, ENF_BuildResult::InvalidRules);
	return true;
}
#endif
