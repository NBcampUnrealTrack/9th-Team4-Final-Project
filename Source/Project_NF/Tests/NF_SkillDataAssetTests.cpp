#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Texture2D.h"
#include "GamePlay/Combat/Skill/Data/NF_SkillDataAsset.h"
#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Acceleration.h"
#include "GamePlay/Combat/Skill/Passive/Attack/NFGA_MagicBullet.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// Skill-owned icon getter and the unmodified build source contract.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_SkillDataAssetTest, "NF.Skills.Data.SourceIcon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_SkillDataAssetTest::RunTest(const FString& Parameters)
{
	auto* Active = NewObject<UNF_ActiveSkillDataAsset>();
	auto* Passive = NewObject<UNF_PassiveSkillDataAsset>();
	auto* ActiveIcon = NewObject<UTexture2D>();
	auto* PassiveIcon = NewObject<UTexture2D>();
	Active->AbilityClass = UNFGA_Acceleration::StaticClass();
	Active->Icon = ActiveIcon;
	Active->BuildCost = 7;
	Passive->AbilityClass = UNFGA_MagicBullet::StaticClass();
	Passive->Icon = PassiveIcon;
	Passive->BuildCost = 3;
	Passive->AbilityLevel = 2;
	FNF_BuildItemData Data;
	TestTrue(TEXT("Active source implements existing build interface"), INF_BuildItemSource::Execute_GetBuildItemData(Active, Data));
	TestTrue(TEXT("Active item type derived from GA"), Data.ItemType == ENF_ItemType::ActiveSkill);
	TestEqual(TEXT("Cost comes directly from DA"), Data.Cost, 7);
	TestTrue(TEXT("Active icon available via source getter"), Active->GetSkillIcon() == ActiveIcon);
	TestTrue(TEXT("Passive source implements existing build interface"), INF_BuildItemSource::Execute_GetBuildItemData(Passive, Data));
	TestTrue(TEXT("Passive type derived from GA"), Data.ItemType == ENF_ItemType::PassiveSkill);
	TestEqual(TEXT("Tier-specific ability level forwarded"), Data.AbilityLevel, 2);
	TestTrue(TEXT("Passive source owns its own icon"), Passive->GetSkillIcon() == PassiveIcon);
	TestTrue(TEXT("Original getter returns same icon"), Passive->GetSkillIcon() == PassiveIcon);
	Passive->Icon = nullptr;
	TestTrue(TEXT("Missing icon is allowed"), INF_BuildItemSource::Execute_GetBuildItemData(Passive, Data));
	TestTrue(TEXT("Getter reflects icon removal"), Passive->GetSkillIcon() == nullptr);
	Passive->AbilityClass = nullptr;
	TestFalse(TEXT("Missing skill fails closed"), INF_BuildItemSource::Execute_GetBuildItemData(Passive, Data));
	TestTrue(TEXT("Failed source clears class"), !Data.AbilityClass);
	TestEqual(TEXT("Failed source is not treated as free"), Data.Cost, -1);
	Active->AbilityLevel = 0;
	TestFalse(TEXT("Invalid ability level rejected"), INF_BuildItemSource::Execute_GetBuildItemData(Active, Data));
	return true;
}
#endif
