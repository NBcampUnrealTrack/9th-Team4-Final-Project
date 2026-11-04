#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GAS/Effects/NFGE_EMP.h"
#include "Monster/NF_Monster.h"
#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Acceleration.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_AccelerationTest, "NF.Skills.Acceleration.LifetimeEMPAndFactorCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_AccelerationTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ANF_Monster* Avatar = World->SpawnActor<ANF_Monster>();
	auto* ASC = Avatar->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(Avatar, Avatar);
	const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Acceleration::StaticClass(), 1));
	auto* Skill = Cast<UNFGA_Acceleration>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	if (!TestNotNull(TEXT("Granted acceleration instance"), Skill))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	auto* Movement = Avatar->GetCharacterMovement();
	Movement->MaxWalkSpeed = 400.0f;
	Movement->MaxWalkSpeedCrouched = 200.0f;
	Skill->GetMutableActiveDefaults().ExecutionDuration = 0.1f;
	TestTrue(TEXT("Acceleration activates"), ASC->TryActivateAbility(Handle));
	TestTrue(TEXT("Standard TPS works without MotionMatching"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 520.0f));
	TestTrue(TEXT("Crouched speed is multiplied too"), FMath::IsNearlyEqual(Movement->MaxWalkSpeedCrouched, 260.0f));
	World->Tick(LEVELTICK_All, 0.2f);
	TestFalse(TEXT("Duration ends the ability"), Skill->IsActive());
	TestTrue(TEXT("Expiry restores walking speed"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 400.0f));
	TestTrue(TEXT("Expiry restores crouched speed"), FMath::IsNearlyEqual(Movement->MaxWalkSpeedCrouched, 200.0f));
	Skill->GetMutableActiveDefaults().ExecutionDuration = 5.0f;
	TestTrue(TEXT("Acceleration can activate again"), ASC->TryActivateAbility(Handle));
	// A separately applied multiplicative slow must survive cancellation of this acceleration.
	Movement->MaxWalkSpeed *= 0.5f;
	Movement->MaxWalkSpeedCrouched *= 0.5f;
	const auto Spec = ASC->MakeOutgoingSpec(UNFGE_EMP::StaticClass(), 1, ASC->MakeEffectContext());
	const auto EMP = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	TestFalse(TEXT("EMP interrupts acceleration"), Skill->IsActive());
	TestTrue(TEXT("EMP preserves the other multiplicative effect"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 200.0f));
	ASC->CancelAbilityHandle(Handle);
	TestTrue(TEXT("Repeated cleanup cannot divide twice"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 200.0f));
	TestFalse(TEXT("EMP blocks reactivation"), ASC->TryActivateAbility(Handle));
	ASC->RemoveActiveGameplayEffect(EMP);
	Skill->GetMutableActiveDefaults().CastTime = 1.0f;
	TestTrue(TEXT("Cast delay starts"), ASC->TryActivateAbility(Handle));
	TestTrue(TEXT("No boost during cast delay"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 200.0f));
	ASC->CancelAbilityHandle(Handle);
	World->Tick(LEVELTICK_All, 1.1f);
	TestTrue(TEXT("Cancelled cast never applies a boost"), FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 200.0f));
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
