#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/Effects/NFGE_EMP.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "Monster/NF_Monster.h"
#include "GamePlay/Combat/Skill/Active/Utility/NFGA_Grab.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GamePlay/Combat/Components/NF_DeployableComponent.h"
#include "GamePlay/Combat/Skill/Portal/NF_Portal.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_EMPTest, "NF.Skills.EMP.InterruptSealOverlapAndIndependentDeployment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_EMPTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ANF_Monster* Target = World->SpawnActor<ANF_Monster>();
	Target->GetCharacterMovement()->Activate(true); Target->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	UAbilitySystemComponent* ASC = Target->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(Target, Target);
	const auto SkillHandle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Grab::StaticClass(), 1));
	auto* Skill = Cast<UNFGA_ActiveSkillBase>(ASC->FindAbilitySpecFromHandle(SkillHandle)->GetPrimaryInstance());
	if (!TestNotNull(TEXT("Granted skill instance"), Skill))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	Skill->GetMutableActiveDefaults().CastTime = 30.0f;
	TestTrue(TEXT("Skill starts casting before EMP"), ASC->TryActivateAbility(SkillHandle));
	// Even a deployed result still retained by a Timed GA must detach instead of being removed by EMP.
	ANF_SkillExecutionActor* Independent = World->SpawnActor<ANF_SkillExecutionActor>();
	auto* Deployment = NewObject<UNF_DeployableComponent>(Independent);
	Independent->AddInstanceComponent(Deployment); Deployment->RegisterComponent();
	Independent->InitializeExecution(Target, Skill);
	TestTrue(TEXT("Independent deployment starts"), Independent->StartExecution());
	TestTrue(TEXT("Started stationary deployment is protected from caster EMP"), Independent->SurvivesCasterEMP());
	Skill->ActiveExecutionActor = Independent;
	Skill->bLocalInputHolding = true;
	// A completed portal GA no longer owns its pair; the deployments must watch the caster themselves.
	ANF_Portal* Entrance = World->SpawnActor<ANF_Portal>();
	ANF_Portal* Exit = World->SpawnActor<ANF_Portal>();
	Entrance->InitializePortal(Exit, false, Target, ASC);
	Exit->InitializePortal(Entrance, true, Target, ASC);
	TestTrue(TEXT("EMP-sensitive exit starts"), Exit->StartPortal());
	TestTrue(TEXT("EMP-sensitive entrance starts"), Entrance->StartPortal());
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UNFGE_EMP::StaticClass(), 1, ASC->MakeEffectContext());
	TestTrue(TEXT("Native EMP GE grants the seal tag"), Spec.Data->Def->GetGrantedTags().HasTag(NFTag_Skill::State_Debuff_EMP));
	const auto FirstEMP = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	TestTrue(TEXT("EMP applied"), FirstEMP.IsValid());
	TestFalse(TEXT("EMP interrupts casting synchronously"), Skill->IsActive());
	TestFalse(TEXT("EMP clears local holding state"), Skill->bLocalInputHolding);
	TestFalse(TEXT("EMP rejects a new cast"), ASC->TryActivateAbility(SkillHandle));
	TestTrue(TEXT("EMP preserves a detached deployment"), Deployment->IsDeploymentActive() && !Independent->IsActorBeingDestroyed());
	TestFalse(TEXT("EMP closes a deployed entrance"), Entrance->IsPortalActive());
	TestFalse(TEXT("EMP closes the linked exit"), Exit->IsPortalActive());
	ANF_Portal* SealedEntrance = World->SpawnActor<ANF_Portal>();
	ANF_Portal* SealedExit = World->SpawnActor<ANF_Portal>();
	SealedEntrance->InitializePortal(SealedExit, false, Target, ASC);
	SealedExit->InitializePortal(SealedEntrance, true, Target, ASC);
	TestFalse(TEXT("Existing caster tag immediately rejects a deployment"), SealedExit->StartPortal());
	TestFalse(TEXT("Rejected pair stays inactive"), SealedEntrance->IsPortalActive());
	Skill->GetMutableSkillDefaults().SkillType = ENF_SkillType::Passive;
	TestFalse(TEXT("Passive skills do not consider EMP a block"), Skill->IsBlockedByEMP());
	Skill->GetMutableSkillDefaults().SkillType = ENF_SkillType::Active;
	const auto SavedEMPRules = Skill->GetMutableActiveDefaults().StateRules;
	Skill->GetMutableActiveDefaults().StateRules.Reset();
	TestTrue(TEXT("Explicit EMP exemption allows activation checks"), Skill->CanActivateAbility(SkillHandle, ASC->AbilityActorInfo.Get()));
	Skill->GetMutableActiveDefaults().StateRules = SavedEMPRules;
	const auto SecondEMP = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	ASC->RemoveActiveGameplayEffect(FirstEMP);
	TestTrue(TEXT("Overlapping EMP retains the seal when one ends"), Skill->IsBlockedByEMP());
	ASC->RemoveActiveGameplayEffect(SecondEMP);
	TestFalse(TEXT("Final EMP removal clears seal"), Skill->IsBlockedByEMP());
	TestTrue(TEXT("New casting works again after EMP"), ASC->TryActivateAbility(SkillHandle));
	// A retained/maintained execution must be stopped, unlike the detached deployment above.
	ANF_SkillExecutionActor* Maintained = World->SpawnActor<ANF_SkillExecutionActor>();
	Maintained->bFollowSource = true;
	auto* MaintainedDeployment = NewObject<UNF_DeployableComponent>(Maintained);
	Maintained->AddInstanceComponent(MaintainedDeployment); MaintainedDeployment->RegisterComponent();
	Maintained->InitializeExecution(Target, Skill); Maintained->StartExecution();
	Skill->ActiveExecutionActor = Maintained;
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	TestFalse(TEXT("EMP stops the maintained execution"), MaintainedDeployment->IsDeploymentActive());
	TestTrue(TEXT("Independent deployment is still active"), Deployment->IsDeploymentActive());
	Independent->StopExecution();
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
