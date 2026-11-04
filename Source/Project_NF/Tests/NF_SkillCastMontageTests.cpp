#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Movement.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"
#include "GAS/Tags/NFTag_Player.h"
#include "UObject/UnrealType.h"

// @Owner: 이정준 (구현: Codex) | 2026-10-28
// Commit gating and lifetime only. Actual animation slots/poses and network playback require PIE.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_SkillCastMontageTest, "NF.Skills.Animation.ImmediateCast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_SkillCastMontageTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	auto* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Character = World->SpawnActor<ACharacter>();
	auto* ASC = NewObject<UAbilitySystemComponent>(Character);
	Character->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Character, Character);
	const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Movement::StaticClass(), 1));
	auto* Skill = Cast<UNFGA_ActiveSkillBase>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	if (!TestNotNull(TEXT("Skill granted"), Skill))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	const auto CountVisuals = [World]()
	{
		int32 Count = 0;
		for (TActorIterator<ANF_SkillCastVisual> It(World); It; ++It) { ++Count; }
		return Count;
	};
	TestTrue(TEXT("Unset montage keeps existing activation working"), ASC->TryActivateAbility(Handle));
	TestEqual(TEXT("Unset montage spawns nothing"), CountVisuals(), 0);
	ASC->CancelAbilityHandle(Handle);
	auto* Montage = NewObject<UAnimMontage>();
	// Synthetic duration suffices for relay creation, without loading a real character animation.
	auto* Length = FindFProperty<FFloatProperty>(UAnimSequenceBase::StaticClass(), TEXT("SequenceLength"));
	if (!TestNotNull(TEXT("Test montage duration property"), Length))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	Length->SetPropertyValue_InContainer(Montage, 1.f);
	Skill->GetMutableActiveDefaults().CastMontage = Montage;
	ASC->AddLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestFalse(TEXT("Blocked skill does not commit"), ASC->TryActivateAbility(Handle));
	TestEqual(TEXT("Blocked skill has no cast visual"), CountVisuals(), 0);
	ASC->RemoveLooseGameplayTag(NFTag_Player::Player_State_Death);
	TestTrue(TEXT("Configured skill activates"), ASC->TryActivateAbility(Handle));
	TestEqual(TEXT("Committed skill creates one replicated relay"), CountVisuals(), 1);
	World->Tick(LEVELTICK_TimeOnly, 0.35f);
	TestFalse(TEXT("Dash ends at its own 0.3 second duration, not the 1 second montage"), Skill->IsActive());
	TestTrue(TEXT("Same skill can cast again"), ASC->TryActivateAbility(Handle));
	TestEqual(TEXT("Next activation gets its own visual"), CountVisuals(), 2);
	ASC->CancelAbilityHandle(Handle);
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}
#endif
