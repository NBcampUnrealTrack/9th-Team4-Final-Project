#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Skill/Passive/Utility/NFGA_DoubleJump.h"
#include "GamePlay/Combat/Skill/Execution/NF_JumpCountEffect.h"
#include "GAS/Effects/NFGE_EMP.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "Monster/NF_Monster.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_DoubleJumpTest, "NF.Skills.DoubleJump.GrantRemoveOverlapAndAvatarChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_DoubleJumpTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ANF_Monster* First = World->SpawnActor<ANF_Monster>();
	auto* ASC = First->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(First, First);
	First->JumpMaxCount = 1;
	First->GetCharacterMovement()->Activate(true);
	First->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	First->JumpCurrentCount = 1;
	const auto One = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_DoubleJump::StaticClass(), 1));
	TestEqual(TEXT("Grant automatically enables double jump"), First->JumpMaxCount, 2);
	TestEqual(TEXT("Grant does not refill a consumed jump"), First->JumpCurrentCount, 1);
	TestTrue(TEXT("Existing Character jump logic allows the second airborne jump"), First->CanJump());
	TestTrue(TEXT("Equipped passive grants the existing state tag"), ASC->HasMatchingGameplayTag(NFTag_Skill::State_DoubleJumpEnabled));
	TestFalse(TEXT("Passive does not need key activation"), ASC->TryActivateAbility(One));
	const auto Two = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_DoubleJump::StaticClass(), 1));
	ASC->ClearAbility(One);
	TestEqual(TEXT("Removing one overlapping grant keeps double jump"), First->JumpMaxCount, 2);
	const auto EMPSpec = ASC->MakeOutgoingSpec(UNFGE_EMP::StaticClass(), 1, ASC->MakeEffectContext());
	ASC->ApplyGameplayEffectSpecToSelf(*EMPSpec.Data.Get());
	TestEqual(TEXT("EMP does not remove the passive"), First->JumpMaxCount, 2);
	ANF_Monster* Second = World->SpawnActor<ANF_Monster>(); Second->JumpMaxCount = 1;
	ASC->InitAbilityActorInfo(First, Second);
	TestEqual(TEXT("Changing avatar restores previous character"), First->JumpMaxCount, 1);
	TestEqual(TEXT("Changing avatar applies the passive to the new character"), Second->JumpMaxCount, 2);
	ASC->ClearAbility(Two);
	TestEqual(TEXT("Last removal restores baseline"), Second->JumpMaxCount, 1);
	TestFalse(TEXT("Last removal clears double jump state tag"), ASC->HasMatchingGameplayTag(NFTag_Skill::State_DoubleJumpEnabled));
	const auto Again = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_DoubleJump::StaticClass(), 1));
	TestEqual(TEXT("Rapid re-equip enables double jump"), Second->JumpMaxCount, 2);
	ASC->ClearAbility(Again);
	TestEqual(TEXT("Rapid removal restores the original baseline"), Second->JumpMaxCount, 1);
	Second->JumpMaxCount = 3;
	auto* Bridge = ANF_JumpCountEffect::FindOrCreate(Second);
	const auto Request = Bridge->AddRequest(2);
	TestEqual(TEXT("Existing higher jump count is preserved"), Second->JumpMaxCount, 3);
	Bridge->RemoveRequest(Request);
	TestEqual(TEXT("Higher baseline survives removal"), Second->JumpMaxCount, 3);
	Second->JumpMaxCount = 1;
	Bridge = ANF_JumpCountEffect::FindOrCreate(Second);
	const auto LowerRequest = Bridge->AddRequest(2);
	const auto HigherRequest = Bridge->AddRequest(3);
	TestEqual(TEXT("Highest requested count wins"), Second->JumpMaxCount, 3);
	Bridge->RemoveRequest(HigherRequest);
	TestEqual(TEXT("Removing higher request leaves double jump"), Second->JumpMaxCount, 2);
	Bridge->RemoveRequest(LowerRequest);
	TestEqual(TEXT("Removing all requests restores one jump"), Second->JumpMaxCount, 1);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
