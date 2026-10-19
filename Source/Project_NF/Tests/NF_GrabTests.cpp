#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "Monster/NF_Monster.h"
#include "GamePlay/Combat/Skill/Active/Utility/NFGA_Grab.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_GrabTest, "NF.Skills.Grab.ValidationMovementAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_GrabTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Source = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator, Params);
	ANF_Monster* Target = World->SpawnActor<ANF_Monster>(FVector(500, 0, 100), FRotator::ZeroRotator, Params);
	Source->GetCharacterMovement()->Activate(true); Source->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	Target->GetCharacterMovement()->Activate(true); Target->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	Target->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	UAbilitySystemComponent* SourceASC = NewObject<UAbilitySystemComponent>(Source);
	Source->AddInstanceComponent(SourceASC); SourceASC->RegisterComponent(); SourceASC->InitAbilityActorInfo(Source, Source);
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent(); TargetASC->InitAbilityActorInfo(Target, Target);
	const auto Handle = SourceASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Grab::StaticClass(), 1));
	auto* Grab = Cast<UNFGA_Grab>(SourceASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	TestTrue(TEXT("AI target accepted without player controller"), Grab->ValidateTarget(Target));
	FVector Destination;
	TestTrue(TEXT("Destination fits capsules"), Grab->CalculateDestination(Target, Destination));
	TestTrue(TEXT("Destination leaves room in front of caster"), Destination.X >= 120.0f);
	AActor* FloorActor = World->SpawnActor<AActor>();
	auto* FloorBox = NewObject<UBoxComponent>(FloorActor);
	FloorActor->AddInstanceComponent(FloorBox); FloorActor->SetRootComponent(FloorBox);
	FloorBox->SetBoxExtent(FVector(1000, 1000, 20));
	FloorBox->SetCollisionObjectType(ECC_WorldStatic);
	FloorBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	FloorBox->SetCollisionResponseToAllChannels(ECR_Block);
	FloorBox->RegisterComponent(); FloorActor->SetActorLocation(FVector(0, 0, -20));
	Source->SetActorLocation(FVector(0, 0, 200));
	FVector GroundDestination;
	TestTrue(TEXT("Destination finds floor beneath elevated caster"), Grab->CalculateDestination(Target, GroundDestination));
	TestTrue(TEXT("Destination rests above floor rather than inheriting caster height"),
		FMath::IsNearlyEqual(GroundDestination.Z, Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0f, 1.0f));
	FloorActor->Destroy(); Source->SetActorLocation(FVector(0, 0, 100));
	Target->SetActorLocation(FVector(900, 0, 100));
	TestFalse(TEXT("Out of range AI target rejected"), Grab->ValidateTarget(Target));
	Target->SetActorLocation(FVector(500, 0, 100));
	FNF_CharacterPullSettings Settings; Settings.Speed = 1000.0f;
	auto* Pull = World->SpawnActor<ANF_CharacterPullActor>();
	bool bReached = false; Pull->OnPullEnded.AddLambda([&](bool bResult) { bReached = bResult; });
	TestTrue(TEXT("Server pull starts"), Pull->StartPull(Source, Target, Destination, Settings, SourceASC, 1));
	TestFalse(TEXT("Second pull on same target rejected"), ANF_CharacterPullActor::CanPull(Target, Settings));
	TestFalse(TEXT("Caster is not moved"), Source->GetCharacterMovement()->HasRootMotionSources());
	FHitResult WalkableHit;
	WalkableHit.bBlockingHit = true;
	WalkableHit.ImpactNormal = FVector::UpVector;
	WalkableHit.Normal = FVector::UpVector;
	// Even a downward pull must not interpret a walkable surface as a wall.
	const FVector SavedDestination = Pull->Motion.Destination;
	Pull->Motion.Destination = FVector(120, 0, -300);
	Pull->HandleHit(Target, nullptr, FVector::ZeroVector, WalkableHit);
	TestTrue(TEXT("Walkable floor does not cancel downward pull"), Pull->Motion.bActive);
	Pull->Motion.Destination = SavedDestination;
	const auto Simulate = [&](ANF_CharacterPullActor* Actor, int32 Frames)
	{
		for (int32 Frame = 0; Frame < Frames; ++Frame)
		{
			World->Tick(LEVELTICK_TimeOnly, 0.025f);
			Target->GetCharacterMovement()->TickComponent(0.025f, LEVELTICK_All, nullptr);
			Actor->Tick(0.025f);
		}
	};
	Simulate(Pull, 20);
	TestTrue(TEXT("Pull reaches fixed destination"), bReached);
	TestTrue(TEXT("Caster stays at original location"), Source->GetActorLocation().Equals(FVector(0, 0, 100), 0.1));
	Settings.Speed = 100.0f; Settings.MaxDuration = 0.1f;
	const FVector BeforeTimeout = Target->GetActorLocation();
	auto* TimeoutPull = World->SpawnActor<ANF_CharacterPullActor>();
	bool bTimeoutFinished = false;
	TimeoutPull->OnPullEnded.AddLambda([&](bool bResult) { bTimeoutFinished = !bResult; });
	TestTrue(TEXT("Limited pull starts"), TimeoutPull->StartPull(Source, Target, FVector(5000, 0, 100), Settings, SourceASC, 1));
	Simulate(TimeoutPull, 10);
	TestTrue(TEXT("Max duration ends pull"), bTimeoutFinished);
	TestTrue(TEXT("Timeout has distinct result"), TimeoutPull->GetResult() == ENF_CharacterPullResult::TimedOut);
	TestTrue(TEXT("Timeout does not increase configured speed"), FVector::Dist2D(BeforeTimeout, Target->GetActorLocation()) < 40.0f);
	Target->SetActorLocation(FVector(600, 0, 100));
	AActor* Wall = World->SpawnActor<AActor>();
	auto* Box = NewObject<UBoxComponent>(Wall); Wall->AddInstanceComponent(Box); Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(10, 150, 300)); Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent(); Wall->SetActorLocation(FVector(300, 0, 100));
	TestFalse(TEXT("Wall blocks AI target validation"), Grab->ValidateTarget(Target));
	Settings.Speed = 1000.0f; Settings.MaxDuration = 1.5f;
	auto* WallPull = World->SpawnActor<ANF_CharacterPullActor>();
	bool bWallStopped = false; WallPull->OnPullEnded.AddLambda([&](bool bResult) { bWallStopped = !bResult; });
	TestTrue(TEXT("Common pull can approach wall"), WallPull->StartPull(Source, Target, FVector(120, 0, 100), Settings, SourceASC, 1));
	Simulate(WallPull, 30);
	TestTrue(TEXT("Collision ends pull"), bWallStopped);
	TestTrue(TEXT("Capsule does not cross wall"), Target->GetActorLocation().X > 300.0f);
	Wall->Destroy();
	Target->SetActorLocation(FVector(500, 0, 100));
	auto* CleanupPull = World->SpawnActor<ANF_CharacterPullActor>();
	TestTrue(TEXT("Cleanup pull starts"), CleanupPull->StartPull(Source, Target, FVector(120, 0, 100), Settings, SourceASC, 1));
	auto* Effect = NewObject<UGameplayEffect>(); Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayEffectSpec Spec(Effect, TargetASC->MakeEffectContext(), 1);
	const auto OtherEffect = TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	const auto OwnEffect = TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
	CleanupPull->StunHandle = OwnEffect;
	CleanupPull->StopPull();
	TestNull(TEXT("Own stun handle removed"), TargetASC->GetActiveGameplayEffect(OwnEffect));
	TestNotNull(TEXT("Unrelated effect preserved"), TargetASC->GetActiveGameplayEffect(OtherEffect));
	TargetASC->RemoveActiveGameplayEffect(OtherEffect);
	Simulate(CleanupPull, 2);
	Grab->SetGrabTarget(Target);
	TestTrue(TEXT("AI can activate grab GA"), SourceASC->TryActivateAbility(Handle));
	TestTrue(TEXT("Grab waits for pull completion"), Grab->IsActive());
	SourceASC->CancelAbilityHandle(Handle);
	TestFalse(TEXT("Cancelling GA ends grab"), Grab->IsActive());
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
