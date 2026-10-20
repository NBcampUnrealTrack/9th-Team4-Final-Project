#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GamePlay/Combat/Skill/Active/Movement/NFGA_Movement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_MovementAbilityTest, "NF.Skills.Movement.DirectionAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_MovementAbilityTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Character = World->SpawnActor<ACharacter>(FVector(0, 0, 300), FRotator(0, 90, 0), Params);
	ACharacter* Target = World->SpawnActor<ACharacter>(FVector(500, 0, 600), FRotator::ZeroRotator, Params);
	UAbilitySystemComponent* ASC = NewObject<UAbilitySystemComponent>(Character);
	Character->AddInstanceComponent(ASC); ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Character, Character);
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	Character->GetCharacterMovement()->Activate(true);
	Character->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Movement::StaticClass(), 1));
	UNFGA_Movement* Ability = Cast<UNFGA_Movement>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	if (!TestNotNull(TEXT("Granted movement instance"), Ability))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	FVector Direction;
	Ability->DirectionSource = ENF_MovementDirection::Avatar;
	TestTrue(TEXT("Avatar direction resolves"), Ability->ResolveDirection(Character, nullptr, Direction));
	TestTrue(TEXT("Local forward follows avatar yaw"), Direction.Equals(FVector::RightVector, 0.001));
	Ability->DirectionSource = ENF_MovementDirection::Target;
	TestFalse(TEXT("Missing AI target rejected"), Ability->ResolveDirection(Character, nullptr, Direction));
	TestTrue(TEXT("AI target direction resolves"), Ability->ResolveDirection(Character, Target, Direction));
	TestTrue(TEXT("Horizontal targeting ignores target height"), Direction.Equals(FVector::ForwardVector, 0.001));
	TestFalse(TEXT("Natural inertia is the default"), Ability->bStopVelocityOnEnd);
	Ability->bStopVelocityOnEnd = true;
	Ability->SetMovementTarget(Target);
	TestTrue(TEXT("Server AI can activate without a PlayerController"), ASC->TryActivateAbility(Handle));
	TestFalse(TEXT("Impulse dash does not add root motion"), Character->GetCharacterMovement()->HasRootMotionSources());
	TestTrue(TEXT("Dash remains active"), Ability->IsActive());
	ASC->CancelAbilityHandle(Handle);
	TestFalse(TEXT("Cancel ends GA"), Ability->IsActive());
	Character->GetCharacterMovement()->TickComponent(0.016f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Hard-stop cancellation removes queued impulse"), FMath::IsNearlyZero(Character->GetCharacterMovement()->Velocity.X));
	Character->GetCharacterMovement()->StopMovementImmediately();
	TestTrue(TEXT("Cancel stops velocity"), Character->GetCharacterMovement()->Velocity.IsNearlyZero());
	ASC->TryActivateAbility(Handle);
	TestFalse(TEXT("Missing consumed target ends the activation"), Ability->IsActive());
	TestFalse(TEXT("Missing target does not start a dash"), Ability->bDashStarted);
	Ability->bStopVelocityOnEnd = false;
	Ability->SetMovementTarget(Target);
	TestTrue(TEXT("Dash can activate again after cancellation"), ASC->TryActivateAbility(Handle));
	Character->GetCharacterMovement()->TickComponent(0.05f, LEVELTICK_All, nullptr);
	const float InitialForwardSpeed = Character->GetCharacterMovement()->Velocity.X;
	TestTrue(TEXT("Impulse pushes toward target"), InitialForwardSpeed > 0.0f);
	TestTrue(TEXT("Gravity continues during dash"), Character->GetCharacterMovement()->Velocity.Z < 0.0f);
	// No force is reapplied on subsequent frames. Duration only controls the GA window.
	{
		// TimeOnly ticks skip TimerManager. Simulate timer frames explicitly in this isolated world.
		TGuardValue<uint64> RestoreFrameCounter(GFrameCounter, GFrameCounter);
		for (int32 Frame = 0; Frame < 20 && Ability->IsActive(); ++Frame)
		{
			++GFrameCounter;
			World->Tick(LEVELTICK_TimeOnly, 0.05f);
			World->GetTimerManager().Tick(0.05f);
			Character->GetCharacterMovement()->TickComponent(0.05f, LEVELTICK_All, nullptr);
		}
	}
	TestFalse(TEXT("Duration expiry ends dash"), Ability->IsActive());
	TestFalse(TEXT("Duration expiry clears dash state"), Ability->bDashStarted);
	TestTrue(TEXT("Natural momentum remains after duration"), Character->GetCharacterMovement()->Velocity.X > 0.0f);
	TestTrue(TEXT("Dash does not continuously accelerate"), Character->GetCharacterMovement()->Velocity.X <= InitialForwardSpeed + 0.1f);
	Ability->MovementMethod = ENF_MovementMethod::Launch;
	Ability->DirectionSource = ENF_MovementDirection::World;
	Ability->DirectionAxis = FVector::UpVector;
	Ability->bHorizontalOnly = false;
	TestTrue(TEXT("High jump can activate"), ASC->TryActivateAbility(Handle));
	TestFalse(TEXT("Launch GA ends immediately"), Ability->IsActive());
	TestTrue(TEXT("Jump remains pending after GA cleanup"), Character->GetCharacterMovement()->HandlePendingLaunch());
	TestTrue(TEXT("High jump launches upward"), Character->GetCharacterMovement()->Velocity.Z > 0.0f);
	AActor* Floor = World->SpawnActor<AActor>();
	UBoxComponent* FloorBox = NewObject<UBoxComponent>(Floor);
	Floor->AddInstanceComponent(FloorBox); Floor->SetRootComponent(FloorBox);
	FloorBox->SetBoxExtent(FVector(10000, 10000, 20));
	FloorBox->SetCollisionObjectType(ECC_WorldStatic);
	FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FloorBox->SetCollisionResponseToAllChannels(ECR_Block); FloorBox->RegisterComponent();
	Floor->SetActorLocation(FVector(0, 0, -20));
	Character->GetCharacterMovement()->ClearAccumulatedForces();
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->SetActorLocation(FVector(0, 1000, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f));
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Ability->MovementMethod = ENF_MovementMethod::Dash;
	Ability->DirectionAxis = FVector::ForwardVector;
	Ability->bHorizontalOnly = true;
	TestTrue(TEXT("Ground dash activates"), ASC->TryActivateAbility(Handle));
	Character->GetCharacterMovement()->TickComponent(0.016f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Horizontal impulse stays grounded"), Character->GetCharacterMovement()->IsMovingOnGround());
	TestTrue(TEXT("Ground friction permits forward impulse movement"), Character->GetCharacterMovement()->Velocity.X > 0.0f);
	ASC->CancelAbilityHandle(Handle);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
