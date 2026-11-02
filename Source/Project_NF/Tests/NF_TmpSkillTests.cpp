#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpSkillDataAsset.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpActiveSkillBase.h"
#include "GamePlay/Combat/Skill/Temporary/NF_TmpPassiveSkillBase.h"
#include "GAS/Effects/NFGE_DoubleJump.h"
#include "GAS/Tags/NFTag_Skill.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-30 | 기존 입력·플레이어를 사용하지 않는 독립 검증.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_TmpSkillTest, "NF.Skills.Temporary.DataAndStateRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_TmpSkillTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Owner = World->SpawnActor<AActor>();
	auto* ASC = NewObject<UAbilitySystemComponent>(Owner);
	Owner->AddInstanceComponent(ASC); ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Owner, Owner);
	auto* Active = NewObject<UNF_TmpActiveSkillDataAsset>();
	Active->AbilityClass = UNF_TmpActiveSkillBase::StaticClass();
	Active->InputMethod = ENF_TmpSkillInput::Hold;
	Active->Cost = 3;
	TestTrue(TEXT("Active data has valid matching ability class"), Active->HasValidSettings());
	const auto ActiveHandle = Active->GrantTo(ASC);
	auto* ActiveSpec = ASC->FindAbilitySpecFromHandle(ActiveHandle);
	auto* ActiveGA = ActiveSpec ? Cast<UNF_TmpActiveSkillBase>(ActiveSpec->GetPrimaryInstance()) : nullptr;
	if (!TestNotNull(TEXT("Active instance granted with data"), ActiveGA))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	TestTrue(TEXT("GA refers to exact DA source, not copied settings"), ActiveGA->GetSkillData() == Active);
	TestTrue(TEXT("Active starts on server"), ASC->TryActivateAbility(ActiveHandle));
	ActiveGA->SetCanBeCanceled(false);
	ASC->AddLooseGameplayTag(NFTag_Skill::State_Debuff_EMP);
	TestFalse(TEXT("Unified forced EMP rule ends even non-cancellable hold"), ActiveGA->IsActive());
	TestFalse(TEXT("EMP rule blocks new activation"), ASC->TryActivateAbility(ActiveHandle));
	ASC->RemoveLooseGameplayTag(NFTag_Skill::State_Debuff_EMP);
	TestTrue(TEXT("Removing state permits a new use"), ASC->TryActivateAbility(ActiveHandle));
	ASC->AbilitySpecInputReleased(*ASC->FindAbilitySpecFromHandle(ActiveHandle));
	TestFalse(TEXT("Hold ends on server input release"), ActiveGA->IsActive());
	ASC->ClearAbility(ActiveHandle);
	auto* Passive = NewObject<UNF_TmpPassiveSkillDataAsset>();
	Passive->AbilityClass = UNF_TmpActiveSkillBase::StaticClass();
	TestFalse(TEXT("Passive DA rejects active class"), Passive->HasValidSettings());
	Passive->AbilityClass = UNF_TmpPassiveSkillBase::StaticClass();
	Passive->EquippedEffects.Add(UNFGE_DoubleJump::StaticClass());
	const auto PassiveHandle = Passive->GrantTo(ASC);
	World->Tick(LEVELTICK_TimeOnly, 0.01f);
	TestTrue(TEXT("Passive equip applies its owned GE"), ASC->HasMatchingGameplayTag(NFTag_Skill::State_DoubleJumpEnabled));
	ASC->ClearAbility(PassiveHandle);
	TestFalse(TEXT("Unequip removes its owned GE"), ASC->HasMatchingGameplayTag(NFTag_Skill::State_DoubleJumpEnabled));
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif
