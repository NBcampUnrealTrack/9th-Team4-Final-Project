#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Acceleration.h"
#include "GamePlay/Combat/Skill/Passive/NFGA_PassiveSkillBase.h"
#include "GamePlay/Combat/Skill/Passive/Utility/NFGA_DoubleJump.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "GAS/Effects/NFGE_DoubleJump.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_SkillDefinitionCompatibilityTest, "NF.Skills.Data.CompatibleDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_SkillDefinitionCompatibilityTest::RunTest(const FString& Parameters)
{
	TestNull(TEXT("Common base no longer owns preview settings"), FindFProperty<FProperty>(UNFGA_SkillBase::StaticClass(), TEXT("PreviewActorClass")));
	TestNull(TEXT("Common base no longer owns passive GE settings"), FindFProperty<FProperty>(UNFGA_SkillBase::StaticClass(), TEXT("PassiveEffects")));
	TestNull(TEXT("Common base no longer owns category scalar"), FindFProperty<FProperty>(UNFGA_SkillBase::StaticClass(), TEXT("SkillCategory")));
	TestNull(TEXT("Common base no longer owns cost scalar"), FindFProperty<FProperty>(UNFGA_SkillBase::StaticClass(), TEXT("BuildCost")));
	TestNull(TEXT("Active does not own editable preview settings"), FindFProperty<FProperty>(UNFGA_ActiveSkillBase::StaticClass(), TEXT("PreviewActorClass")));
	TestNull(TEXT("Active has no passive GE settings"), FindFProperty<FProperty>(UNFGA_ActiveSkillBase::StaticClass(), TEXT("PassiveEffects")));
	TestNull(TEXT("Passive does not own editable equip GE settings"), FindFProperty<FProperty>(UNFGA_PassiveSkillBase::StaticClass(), TEXT("PassiveEffects")));
	TestNull(TEXT("Passive has no preview settings"), FindFProperty<FProperty>(UNFGA_PassiveSkillBase::StaticClass(), TEXT("PreviewActorClass")));
	TestTrue(TEXT("Existing active class retains common-base compatibility"), UNFGA_Acceleration::StaticClass()->IsChildOf(UNFGA_ActiveSkillBase::StaticClass()));
	TestTrue(TEXT("Existing passive class has passive parent"), UNFGA_DoubleJump::StaticClass()->IsChildOf(UNFGA_PassiveSkillBase::StaticClass()));
	auto* LegacyData = NewObject<UNF_SkillDataAsset>();
	LegacyData->AbilityClass = UNFGA_Acceleration::StaticClass();
	LegacyData->BuildCost = 9;
	FNF_BuildItemData Item;
	TestFalse(TEXT("Old common-only DA requires explicit migration"), LegacyData->GetBuildItemData_Implementation(Item));
	TestEqual(TEXT("Unmigrated DA is not treated as a free skill"), Item.Cost, -1);

	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Test world"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Owner = World->SpawnActor<AActor>();
	auto* ASC = NewObject<UAbilitySystemComponent>(Owner);
	Owner->AddInstanceComponent(ASC); ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Owner, Owner);
	auto* Definition = NewObject<UNF_ActiveSkillDataAsset>();
	Definition->AbilityClass = UNFGA_Acceleration::StaticClass();
	Definition->BuildCost = 7;
	Definition->ExecutionMode = ENF_SkillExecutionMode::Timed;
	Definition->ExecutionDuration = 5.f;
	FNF_SkillStateRule Rule;
	Rule.StateTag = NFTag_Skill::State_DoubleJumpEnabled;
	Rule.bBlockActivation = true; Rule.bCancelRunning = false;
	Definition->StateRules.Add(Rule);
	const auto Handle = Definition->GrantTo(ASC);
	auto* Spec = ASC->FindAbilitySpecFromHandle(Handle);
	auto* Skill = Spec ? Cast<UNFGA_SkillBase>(Spec->GetPrimaryInstance()) : nullptr;
	if (TestNotNull(TEXT("DA grants existing native skill"), Skill))
	{
		TestTrue(TEXT("Instance retains exact skill DA"), Skill->GetSkillData() == Definition);
		TestEqual(TEXT("Instance uses DA cost"), Skill->GetBuildCost(), 7);
		Definition->BuildCost = 8;
		TestEqual(TEXT("DA changes are read directly without copying into the GA"), Skill->GetBuildCost(), 8);
		Definition->CastTime = 2.5f;
		TestEqual(TEXT("Active timing is read directly from DA"), Skill->GetCastTime(), 2.5f);
		TestTrue(TEXT("Class default was not modified"), GetDefault<UNFGA_Acceleration>()->GetBuildCost() != 7);
		TestTrue(TEXT("Valid definition permits activation before the restricted state"), Skill->CanActivateAbility(Handle, ASC->AbilityActorInfo.Get()));
		ASC->AddLooseGameplayTag(Rule.StateTag);
		TestFalse(TEXT("DA state rule blocks new activation"), Skill->CanActivateAbility(Handle, ASC->AbilityActorInfo.Get()));
		ASC->RemoveLooseGameplayTag(Rule.StateTag);
	}
	ASC->ClearAbility(Handle);

	// Existing build currently passes a non-DA source. It must continue working until its owner migrates it.
	const auto LegacyHandle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Acceleration::StaticClass(), 1, INDEX_NONE, Owner));
	auto* LegacySpec = ASC->FindAbilitySpecFromHandle(LegacyHandle);
	auto* LegacySkill = LegacySpec ? Cast<UNFGA_SkillBase>(LegacySpec->GetPrimaryInstance()) : nullptr;
	if (TestNotNull(TEXT("Non-DA source still grants"), LegacySkill))
	{
		TestNotNull(TEXT("Non-DA source uses a native typed DA preset"), LegacySkill->GetSkillData());
		TestEqual(TEXT("Legacy instance retains its original cost"), LegacySkill->GetBuildCost(), GetDefault<UNFGA_Acceleration>()->GetBuildCost());
	}
	ASC->ClearAbility(LegacyHandle);
	auto* PassiveData = NewObject<UNF_PassiveSkillDataAsset>();
	PassiveData->AbilityClass = UNFGA_Acceleration::StaticClass();
	TestFalse(TEXT("Passive DA rejects active GA"), PassiveData->HasValidSettings());
	PassiveData->AbilityClass = UNFGA_DoubleJump::StaticClass();
	PassiveData->PassiveEffects.Add(UNFGE_DoubleJump::StaticClass());
	const auto PassiveHandle = PassiveData->GrantTo(ASC);
	TestTrue(TEXT("New passive DA preserves existing default equip effects"), ASC->HasMatchingGameplayTag(NFTag_Skill::State_DoubleJumpEnabled));
	ASC->ClearAbility(PassiveHandle);
	TestFalse(TEXT("Unequip still removes owned passive effect"), ASC->HasMatchingGameplayTag(NFTag_Skill::State_DoubleJumpEnabled));
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
