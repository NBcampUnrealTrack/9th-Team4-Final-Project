#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Acceleration.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Monster/NF_Monster.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_SkillDeathBlockTest, "NF.Skills.Death.BlockCastAndCancelPendingExecution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_SkillDeathBlockTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Avatar = World->SpawnActor<ANF_Monster>();
	auto* ASC = Avatar->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(Avatar, Avatar);
	const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Acceleration::StaticClass(), 1));
	auto* Skill = Cast<UNFGA_ActiveSkillBase>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	if (!TestNotNull(TEXT("Granted skill instance"), Skill))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	TestTrue(TEXT("Constructor inherits death activation block"), Skill->ActivationBlockedTags.HasTag(NFTag_Player::Player_State_Death));
	Skill->CastTime = 0.1f;
	ASC->AddLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestFalse(TEXT("Dead actor cannot activate"), ASC->TryActivateAbility(Handle));
	TestFalse(TEXT("Dead actor cannot continue delayed execution"), Skill->CanContinueSkillExecution());
	ASC->RemoveLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestTrue(TEXT("Living actor starts casting"), ASC->TryActivateAbility(Handle));
	ASC->AddLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestFalse(TEXT("Death interrupts cancellable cast"), Skill->IsActive());
	const float Before = Avatar->GetCharacterMovement()->MaxWalkSpeed;
	World->Tick(LEVELTICK_All, 0.2f);
	TestEqual(TEXT("Cancelled cast does not boost speed after death"), Avatar->GetCharacterMovement()->MaxWalkSpeed, Before);
	ASC->RemoveLooseGameplayTag(NFTag_Player::Player_State_Death);
	// Simulate a serialized BP override that removed the constructor's editable tag.
	Skill->ActivationBlockedTags.Reset();
	ASC->AddLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestFalse(TEXT("BP tag override cannot bypass mandatory death block"), ASC->TryActivateAbility(Handle));
	ASC->RemoveLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestTrue(TEXT("Removing death state permits casting again"), ASC->TryActivateAbility(Handle));
	ASC->CancelAbilityHandle(Handle);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif
