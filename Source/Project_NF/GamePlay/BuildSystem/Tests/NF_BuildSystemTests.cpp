#if WITH_DEV_AUTOMATION_TESTS

#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponentConfigDataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"

#include "AbilitySystemComponent.h"
#include "GamePlay/BuildSystem/Tests/NF_BuildTestItemSource.h"
#include "Abilities/GameplayAbility.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GamePlay/BuildSystem/Player/NF_BuildInputComponent.h"
#include "GamePlay/BuildSystem/Core/NF_BuildMatchSubsystem.h"
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
	UNF_BuildItemDefinitionDataAsset* MakeItem(UNF_BuildRules* Rules, FName Id, ENF_ItemType Type, int32 Cost)
	{
		UNF_BuildItemDefinitionDataAsset* Item = NewObject<UNF_BuildItemDefinitionDataAsset>(Rules);
		Item->ItemId = Id;
		// 스킬은 빌드가 스킬 DA의 GrantTo로 부여하므로 스킬 DA 계열 테스트 원본을 사용한다.
		UDataAsset* Source = nullptr;
		FNF_BuildItemData* Data = nullptr;
		if (Type == ENF_ItemType::ActiveSkill)
		{
			UNF_BuildTestActiveSkillSource* Skill = NewObject<UNF_BuildTestActiveSkillSource>(Rules);
			Source = Skill;
			Data = &Skill->Data;
		}
		else if (Type == ENF_ItemType::PassiveSkill)
		{
			UNF_BuildTestPassiveSkillSource* Skill = NewObject<UNF_BuildTestPassiveSkillSource>(Rules);
			Source = Skill;
			Data = &Skill->Data;
		}
		else
		{
			UNF_BuildTestItemSource* Generic = NewObject<UNF_BuildTestItemSource>(Rules);
			Source = Generic;
			Data = &Generic->Data;
		}
		Data->ItemType = Type;
		Data->Cost = Cost;
		Rules->Catalog.Add(Item);
		Rules->ItemSources.Add(Id, Source);
		return Item;
	}

	FNF_BuildItemData& SourceDataOf(UObject* Source)
	{
		if (UNF_BuildTestActiveSkillSource* Active = Cast<UNF_BuildTestActiveSkillSource>(Source)) return Active->Data;
		if (UNF_BuildTestPassiveSkillSource* Passive = Cast<UNF_BuildTestPassiveSkillSource>(Source)) return Passive->Data;
		return CastChecked<UNF_BuildTestItemSource>(Source)->Data;
	}

	FNF_BuildItemData& SourceData(UNF_BuildRules* Rules, const UNF_BuildItemDefinitionDataAsset* Item)
	{
		return SourceDataOf(Rules->ItemSources.FindChecked(Item->ItemId).Get());
	}

	void SetSourceAvailable(UObject* Source, bool bAvailable)
	{
		if (UNF_BuildTestActiveSkillSource* Active = Cast<UNF_BuildTestActiveSkillSource>(Source)) { Active->bAvailable = bAvailable; return; }
		if (UNF_BuildTestPassiveSkillSource* Passive = Cast<UNF_BuildTestPassiveSkillSource>(Source)) { Passive->bAvailable = bAvailable; return; }
		CastChecked<UNF_BuildTestItemSource>(Source)->bAvailable = bAvailable;
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
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, {Select(TEXT("Rifle"))}).Result,
	          ENF_BuildResult::Success);
	TestEqual(TEXT("Shared weapon component has no second equipment slot"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, {Select(TEXT("Rifle"), 1)}).Result,
	          ENF_BuildResult::InvalidSlot);
	SourceData(Rules, Rules->Catalog[1]).WeaponClass = ANF_WeaponBase::StaticClass();
	TestEqual(TEXT("Skill cannot spawn a weapon"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8,
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
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12, FullBuild).Result,
	          ENF_BuildResult::Success);
	TestEqual(TEXT("상의는 머리 슬롯에 넣을 수 없다"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12,
	                                       {Select(TEXT("Rifle")), Select(TEXT("Vest"), 0)}).Result,
	          ENF_BuildResult::InvalidSlot);
	TestEqual(TEXT("패시브는 두 번째 슬롯을 사용할 수 없다"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12,
	                                       {Select(TEXT("Rifle")), Select(TEXT("HealthUp"), 1)}).Result,
	          ENF_BuildResult::InvalidSlot);
	UNF_BuildItemDefinitionDataAsset* OtherHead = MakeItem(Rules, TEXT("OtherHelmet"), ENF_ItemType::Armor, 1);
	SourceData(Rules, OtherHead).ArmorSlot = ENF_ArmorSlot::HeadArmor;
	TestEqual(TEXT("같은 부위 두 개의 동시 장착을 거부한다"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12,
	                                       {
		                                       Select(TEXT("Rifle")), Select(TEXT("Helmet")),
		                                       Select(TEXT("OtherHelmet"))
	                                       }).Result,
	          ENF_BuildResult::InvalidSlot);
	SourceData(Rules, Head).ArmorSlot = ENF_ArmorSlot::None;
	TestEqual(TEXT("부위 미지정 방어구가 있어도 다른 항목의 빌드는 막지 않는다"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12, {Select(TEXT("Rifle"))}).Result,
	          ENF_BuildResult::Success);
	TestEqual(TEXT("부위 미지정 방어구를 선택하면 거부한다"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12,
	                                       {Select(TEXT("Rifle")), Select(TEXT("Helmet"), 0)}).Result,
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
	FNF_BuildValidation Result = UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, ExactLimit);
	TestEqual(TEXT("Exact cost limit accepted"), Result.Result, ENF_BuildResult::Success);
	TestEqual(TEXT("Unified cost includes weapon and skills"), Result.TotalCost, 8);
	TArray<FNF_BuildSelection> OverLimit = ExactLimit;
	OverLimit.Add(Select(TEXT("HealthUp")));
	TestEqual(
		TEXT("Over limit rejected"),
		UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, OverLimit).Result,
		ENF_BuildResult::CostLimitExceeded);
	TestEqual(
		TEXT("서버 상한이 증가하면 더 큰 빌드 허용"),
		UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12, OverLimit).Result,
		ENF_BuildResult::Success);
	TestEqual(TEXT("음수 상한 거절"), UNF_BuildMatchSubsystem::ValidateSelections(Rules, -1, ExactLimit).Result,
	          ENF_BuildResult::InvalidRules);
	TestEqual(
		TEXT("상한 0에서는 유료 빌드 거절"), UNF_BuildMatchSubsystem::ValidateSelections(Rules, 0, ExactLimit).Result,
		ENF_BuildResult::CostLimitExceeded);
	TestEqual(TEXT("개인 목록 없이도 전체 카탈로그 선택 허용"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, ExactLimit).Result,
	          ENF_BuildResult::Success);
	TestEqual(TEXT("전체 공개여도 카탈로그 외 항목 거절"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, {Select(TEXT("UnknownCatalogItem"))}).Result,
	          ENF_BuildResult::UnknownItem);
	const FNF_BuildValidation OwnedResult =
		UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, OverLimit, nullptr, &ExactLimit);
	TestEqual(TEXT("이미 장착한 항목은 코스트에서 제외"), OwnedResult.Result, ENF_BuildResult::Success);
	TestEqual(TEXT("새로 고른 항목만 코스트에 포함"), OwnedResult.TotalCost,
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 12, OverLimit).TotalCost - 8);

	SourceData(Rules, Rules->Catalog[0]).Cost = MAX_int32;
	SourceData(Rules, Rules->Catalog[1]).Cost = MAX_int32;
	TestEqual(TEXT("Cost sum cannot wrap to a cheap build"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, MAX_int32,
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
		TestEqual(Name, UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, Build).Result, Expected);
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
	// 엔진은 한 프레임(GFrameCounter) 안에서 같은 tick 함수를 한 번만 실행한다.
	// 테스트는 엔진 루프 밖에서 World를 직접 돌리므로 매번 프레임을 넘겨 컴포넌트 tick이 실행되게 한다.
	auto TickWorld = [World](float DeltaSeconds)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, DeltaSeconds);
	};

	ACharacter* Player = World->SpawnActor<ACharacter>();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	// 리슨 서버 호스트처럼 서버 권한과 로컬 조작을 함께 가진 플레이어로 만든다.
	// Draft 편집과 슬롯 키 변경은 로컬 플레이어에서만 허용된다.
	Controller->SetAsLocalPlayerController();
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
	// 실제 스킬 DA처럼 조회값의 GA를 원본 DA의 GA와 맞춘다.
	SourceData(Rules, Rules->Catalog[1]).AbilityClass = UNFGA_ActiveSkillBase::StaticClass();
	SourceData(Rules, Rules->Catalog[2]).AbilityClass = UNFGA_ActiveSkillBase::StaticClass();
	TestTrue(TEXT("Initialize authoritative ASC"), Build->InitializeAbilitySystem(ASC));
	TestTrue(TEXT("Configure initial rules"), Shop->ConfigureRules(Rules));
	TestEqual(TEXT("개인별 조건 초기화 전에는 적용 불가"),
	          Build->PreviewBuild({Select(TEXT("Rifle"))}).Result, ENF_BuildResult::InvalidRules);
	TestTrue(TEXT("서버 테스트 권한 초기화"), Build->UpdateBuildCostLimit(8));
	// The interaction is server-side; no overlap event is required.
	Player->DispatchBeginPlay();
	Shop->DispatchBeginPlay();
	TickWorld(1.f / 60.f);
	TestTrue(TEXT("Player in interaction distance"), Shop->CanBuild(Player));
	Build->RequestApplyBuild({Select(TEXT("Rifle")), Select(TEXT("Grenade"))}, Shop);
	TestEqual(TEXT("Proximity without interaction cannot apply a build"), Build->GetCurrentBuild().Num(), 0);
	Shop->NF_Interaction(Controller);
	TestTrue(TEXT("Interface interaction opens authorized session"), Build->GetCurrentShop() == Shop);
	// 검증이 첫 항목에서 중단돼도 표시 금액과 교체 예상액은 전체 Draft를 기준으로 한다.
	FNF_BuildItemData& PassiveData = SourceData(Rules, Rules->FindItemDefinition(TEXT("HealthUp")));
	PassiveData.AbilityLevel = 0;
	TestTrue(TEXT("패시브 선택"), Build->SelectDraftItem(TEXT("HealthUp")));
	TestTrue(TEXT("패시브 뒤에 무기 선택"), Build->SelectDraftItem(TEXT("Rifle")));
	const FNF_BuildShopPreview Preview = Build->GetShopPreview();
	TestEqual(TEXT("적용 불가한 패시브"), Preview.Validation.Result, ENF_BuildResult::InvalidGrant);
	TestEqual(TEXT("검증 실패 시에도 뒤에 선택한 무기까지 표시 금액에 포함"), Preview.DisplayCost, 3);
	const FNF_BuildPreviewItem* PassiveRow = Preview.Catalog.FindByPredicate([](const FNF_BuildPreviewItem& Row)
	{
		return Row.Definition->ItemId == TEXT("HealthUp");
	});
	if (TestNotNull(TEXT("카탈로그에 선택한 패시브가 표시된다"), PassiveRow))
	{
		TestEqual(TEXT("같은 항목과 슬롯을 교체할 때 한 번만 차감"), PassiveRow->CostAfterSelection, 3);
	}
	PassiveData.AbilityLevel = 1;
	Build->RevertDraftItem(TEXT("HealthUp"));
	Build->RevertDraftItem(TEXT("Rifle"));
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
	// 장착 중인 Rifle·Grenade는 코스트에서 빠지므로, 새로 고른 Dash+HealthUp(5)가 상한 4를 넘게 한다.
	TestTrue(TEXT("예산 초과 확인용 상한 축소"), Build->UpdateBuildCostLimit(4));
	Build->RequestApplyBuild(
		{Select(TEXT("Rifle")), Select(TEXT("Grenade")), Select(TEXT("Dash"), 1), Select(TEXT("HealthUp"))}, Shop);
	TestEqual(TEXT("Over budget request preserves build"), Build->GetCurrentBuild().Num(), 2);
	TestTrue(TEXT("상한 복원"), Build->UpdateBuildCostLimit(8));
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
	TickWorld(1.f / 60.f);

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
	TestFalse(TEXT("음수 코스트 상한 갱신 거절"), Build->UpdateBuildCostLimit(-1));
	TestTrue(TEXT("외부 서버에서 코스트 상한 갱신"), Build->UpdateBuildCostLimit(12));
	TickWorld(0.25f);
	TestTrue(TEXT("거리 안의 상점을 조회할 수 있다"), Build->GetShopsInRange().Contains(Shop));
	TestTrue(TEXT("현재 카탈로그의 액티브 스킬을 표시한다"),
	         Build->GetCatalogItems(ENF_ItemType::ActiveSkill, ENF_ArmorSlot::None).Num() > 0);
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
	TickWorld(0.25f);
	TestFalse(TEXT("사용 세션이 없어도 범위 이탈을 감지한다"), Build->GetShopsInRange().Contains(Shop));
	Player->SetActorLocation(FVector::ZeroVector);
	TickWorld(0.25f);
	TestTrue(TEXT("상호작용 전에도 범위 재진입을 감지한다"), Build->GetShopsInRange().Contains(Shop));
	Build->RequestApplyBuild({Select(TEXT("Rifle"))}, Shop);
	TestEqual(TEXT("Closed session cannot apply another build"), Build->GetCurrentBuild().Num(), 2);
	Shop->NF_Interaction(Controller);
	Player->SetActorLocation(FVector(2000.f, 0.f, 0.f));
	TickWorld(0.25f);
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
	         Build->UpdateBuildCostLimit(8));
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
	const TArray<FNF_BuildSelection> Build = {Select(TEXT("Rifle")), Select(TEXT("Grenade"))};
	TMap<FName, FNF_BuildItemData> Snapshot;
	TestEqual(TEXT("원본 비용으로 서버 검증"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, Build, &Snapshot).TotalCost, 4);
	const FNF_BuildItemData* GrenadeData = Snapshot.Find(TEXT("Grenade"));
	if (!TestNotNull(TEXT("검증한 선택 항목의 장착용 스냅샷"), GrenadeData)) return false;
	TestEqual(TEXT("장착용 스냅샷도 같은 원본 비용"), GrenadeData->Cost, 2);
	SourceData(Rules, Rules->Catalog[1]).Cost = 7;
	TestEqual(TEXT("원본만 수정하면 기존 ID의 비용 검증에 반영"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, Build, &Snapshot).Result,
	          ENF_BuildResult::CostLimitExceeded);
	TestTrue(TEXT("실패 시 장착용 스냅샷을 남기지 않는다"), Snapshot.IsEmpty());

	UDataAsset* Original = Rules->ItemSources.FindChecked(TEXT("Grenade"));
	Rules->ItemSources.Remove(TEXT("Grenade"));
	FNF_BuildItemData Readback;
	Readback.Cost = 99;
	TestFalse(TEXT("원본 누락을 거절"), Rules->ResolveItemData(TEXT("Grenade"), Readback));
	TestEqual(TEXT("실패 시 이전 조회값을 재사용하지 않는다"), Readback.Cost, -1);
	TestEqual(TEXT("미연결 항목을 무료로 통과시키지 않는다"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, Build).Result, ENF_BuildResult::InvalidRules);
	Rules->ItemSources.Add(TEXT("Grenade"), NewObject<UNF_BuildItemDefinitionDataAsset>(Rules));
	TestFalse(TEXT("인터페이스 미구현 원본 거절"), Rules->ResolveItemData(TEXT("Grenade"), Readback));
	Rules->ItemSources.Add(TEXT("Grenade"), Original);
	SetSourceAvailable(Original, false);
	TestFalse(TEXT("원본이 준비되지 않았으면 거절"), Rules->ResolveItemData(TEXT("Grenade"), Readback));
	SetSourceAvailable(Original, true);
	SourceData(Rules, Rules->Catalog[1]).Cost = 0;
	TestEqual(TEXT("원본에서 명시한 무료 항목은 허용"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, Build).Result, ENF_BuildResult::Success);
	SourceData(Rules, Rules->Catalog[1]).ItemType = static_cast<ENF_ItemType>(255);
	TestEqual(TEXT("원본의 잘못된 분류 거절"),
	          UNF_BuildMatchSubsystem::ValidateSelections(Rules, 8, Build).Result, ENF_BuildResult::InvalidRules);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_BuildServiceTest, "NF.Build.Service.IsolatedQueriesAndShopLifetime",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_BuildServiceTest::RunTest(const FString& Parameters)
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
	UNF_BuildMatchSubsystem* Service = World->GetSubsystem<UNF_BuildMatchSubsystem>();
	if (!TestNotNull(TEXT("월드에 빌드 서비스 생성"), Service)) return false;

	AActor* Player = World->SpawnActor<AActor>();
	ANF_BuildShopZone* Shop = World->SpawnActor<ANF_BuildShopZone>();
	if (!TestNotNull(TEXT("범위 조회 대상"), Player) || !TestNotNull(TEXT("등록할 상점"), Shop)) return false;
	TestTrue(TEXT("동적으로 생성한 상점 자동 등록"), Service->GetShopsInRange(Player).Contains(Shop));
	Service->RegisterShop(Shop);
	TestEqual(TEXT("초기화와 BeginPlay의 중복 등록 방지"), Service->GetShopsInRange(Player).Num(), 1);
	Player->SetActorLocation(FVector(10000.f, 0.f, 0.f));
	TestTrue(TEXT("기존 거리 기준으로 범위 밖 제외"), Service->GetShopsInRange(Player).IsEmpty());
	Player->SetActorLocation(FVector::ZeroVector);
	Shop->Destroy();
	TestTrue(TEXT("상점 종료 시 등록 해제"), Service->GetShopsInRange(Player).IsEmpty());

	UNF_BuildRules* FirstRules = MakeRules();
	UNF_BuildRules* SecondRules = MakeRules();
	SourceData(SecondRules, SecondRules->Catalog[0]).Cost = 7;
	const TArray<FNF_BuildSelection> Current = {Select(TEXT("Rifle"))};
	const TArray<FNF_BuildSelection> Draft = {Select(TEXT("Rifle")), Select(TEXT("HealthUp"))};
	const TSet<FName> Picked = {FName(TEXT("HealthUp"))};
	const TArray<FNF_BuildEquippedItem> Equipped;
	const FNF_BuildQueryContext First = {FirstRules, 8, true, Current, Draft, Picked, Equipped};
	const FNF_BuildQueryContext Second = {SecondRules, 2, true, Current, Draft, Picked, Equipped};
	TestEqual(TEXT("장착품은 무료, 이번에 고른 항목만 표시"), Service->GetShopPreview(First).DisplayCost, 1);
	TestEqual(TEXT("다른 플레이어의 코스트 상한 사용"), Service->GetShopPreview(Second).Validation.CostLimit, 2);
	const FNF_BuildShopPreview FirstAgain = Service->GetShopPreview(First);
	TestEqual(TEXT("다른 요청 이후에도 첫 플레이어 상한 유지"), FirstAgain.Validation.CostLimit, 8);
	if (TestEqual(TEXT("첫 플레이어의 선택 두 항목 유지"), FirstAgain.Draft.Num(), 2))
	{
		TestEqual(TEXT("다른 상점 원본과 섞이지 않음"), FirstAgain.Draft[0].Data.Cost, 2);
	}
	const TSet<FName> PickedAgain = {FName(TEXT("Rifle")), FName(TEXT("HealthUp"))};
	const FNF_BuildQueryContext Reselected = {SecondRules, 2, true, Current, Draft, PickedAgain, Equipped};
	TestEqual(TEXT("재선택한 장착품은 해당 상점 비용 반영"), Service->GetShopPreview(Reselected).DisplayCost, 8);
	TestEqual(TEXT("재선택 비용의 상한 초과 검증"), Service->GetShopPreview(Reselected).Validation.Result,
	          ENF_BuildResult::CostLimitExceeded);

	FNF_BuildEquippedItem Snapshot;
	Snapshot.Definition = FirstRules->Catalog[0];
	Snapshot.Source = FirstRules->ItemSources.FindRef(TEXT("Rifle"));
	FirstRules->ResolveItemData(TEXT("Rifle"), Snapshot.ItemData);
	const TArray<FNF_BuildEquippedItem> SavedItems = {Snapshot};
	const FNF_BuildQueryContext Closed = {nullptr, 8, true, Current, Draft, Picked, SavedItems};
	TestTrue(TEXT("상점 종료 후 HUD 원본 조회"), Service->GetItemSource(Closed, TEXT("Rifle")) == Snapshot.Source);
	TestTrue(TEXT("상점 종료 후 새 카탈로그는 비어 있음"), Service->GetCatalogItems(
		Closed, ENF_ItemType::Weapon, ENF_ArmorSlot::None).IsEmpty());
	return true;
}

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
	         UNF_BuildMatchSubsystem::IsArmorDefinitionCompatible(Data.FindChecked(TEXT("Head.T2"))));
	FNF_BuildItemData WrongSlot = Data.FindChecked(TEXT("Head.T2"));
	WrongSlot.ArmorSlot = ENF_ArmorSlot::BodyArmor;
	TestFalse(TEXT("Source/slot mismatch rejected"), UNF_BuildMatchSubsystem::IsArmorDefinitionCompatible(WrongSlot));
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
		Build->ApplyWeaponData(Probe, WeaponData));
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
