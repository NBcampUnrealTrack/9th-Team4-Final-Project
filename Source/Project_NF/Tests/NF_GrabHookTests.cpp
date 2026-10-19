#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Monster/NF_Monster.h"
#include "GamePlay/Combat/Projectiles/NF_GrabHook.h"
#include "GamePlay/Combat/Projectiles/NF_GrabHookVisual.h"
#include "GamePlay/Combat/Skill/Active/Utility/NFGA_Grab.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_GrabHookTest, "NF.Skills.Grab.ProjectileTravelHitAndReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNF_GrabHookTest::RunTest(const FString& Parameters)
{
	// This isolated world does not run BeginPlay. Permit native Actor delegate callbacks
	// so swept hits execute through ProcessEvent just as they do in an initialized game world.
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
	auto* ASC = NewObject<UAbilitySystemComponent>(Source); Source->AddInstanceComponent(ASC);
	ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Source, Source);
	Target->GetAbilitySystemComponent()->InitAbilityActorInfo(Target, Target);
	const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_Grab::StaticClass(), 1));
	auto* Grab = Cast<UNFGA_Grab>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	const auto Step = [&](ANF_GrabHook* Hook)
	{
		World->Tick(LEVELTICK_TimeOnly, 0.025f);
		if (IsValid(Hook) && !Hook->IsActorBeingDestroyed())
		{
			// Match MovementComponent's engine scheduling: projectile first, owner actor second.
			if (Hook->Frame.Phase == ENF_GrabHookPhase::Flying) { Hook->ProjectileMovement->TickComponent(0.025f, LEVELTICK_All, nullptr); }
			Hook->Tick(0.025f);
		}
		if (Target->GetCharacterMovement()->HasRootMotionSources()) { Target->GetCharacterMovement()->TickComponent(0.025f, LEVELTICK_All, nullptr); }
		for (TActorIterator<ANF_CharacterPullActor> It(World); It; ++It) { It->Tick(0.025f); }
	};
	Grab->SetGrabTarget(Target);
	TestTrue(TEXT("AI launches toward supplied target"), ASC->TryActivateAbility(Handle));
	ANF_GrabHook* Hook = Grab->ActiveHook;
	if (!TestNotNull(TEXT("Hook spawned"), Hook)) { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false; }
	TestTrue(TEXT("Starts in flight"), Hook->GetHookPhase() == ENF_GrabHookPhase::Flying);
	TestFalse(TEXT("Target is not pulled at button press"), Target->GetCharacterMovement()->HasRootMotionSources());
	const FVector Origin = Hook->GetActorLocation(); Step(Hook);
	TestTrue(TEXT("Hook actually travels"), FVector::Dist(Origin, Hook->GetActorLocation()) > 1.0f);
	TestFalse(TEXT("No early hit before arrival"), Target->GetCharacterMovement()->HasRootMotionSources());
	for (int32 Frame = 0; Frame < 30 && Hook->GetHookPhase() == ENF_GrabHookPhase::Flying; ++Frame) { Step(Hook); }
	AddInfo(FString::Printf(TEXT("Impact state=%d pos=%s target=%s pending=%d projectileActive=%d updated=%s"),
		static_cast<int32>(Hook->GetHookPhase()), *Hook->GetActorLocation().ToString(), *Target->GetActorLocation().ToString(),
		Hook->bPendingCharacterHit, Hook->ProjectileMovement->IsActive(), *GetNameSafe(Hook->ProjectileMovement->UpdatedComponent)));
	TestTrue(TEXT("Physical collision latches target"), Hook->GetHookPhase() == ENF_GrabHookPhase::Attached);
	TestTrue(TEXT("Pull starts only after hook collision"), Target->GetCharacterMovement()->HasRootMotionSources());
	ASC->CancelAbilityHandle(Handle);
	TestTrue(TEXT("Cancellation clears hook"), Hook->GetHookPhase() == ENF_GrabHookPhase::Finished);
	Target->GetCharacterMovement()->TickComponent(0.025f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Cancellation removes pull source"), Target->GetCharacterMovement()->HasRootMotionSources());
	Target->GetCharacterMovement()->StopMovementImmediately(); Target->SetActorLocation(FVector(500, 0, 100));
	Grab->SetGrabTarget(Target); ASC->TryActivateAbility(Handle); Hook = Grab->ActiveHook;
	for (int32 Frame = 0; Frame < 30 && Hook->GetHookPhase() == ENF_GrabHookPhase::Flying; ++Frame) { Step(Hook); }
	TestTrue(TEXT("Second shot confirms pull before presentation test"), Target->GetCharacterMovement()->HasRootMotionSources());
	if (Hook->VisualActor.IsValid()) { Hook->VisualActor->Destroy(); }
	TestTrue(TEXT("Destroying presentation does not end gameplay"), Grab->IsActive());
	Hook->Abort();
	TestTrue(TEXT("Losing simulation hook after impact does not cancel confirmed pull"), Grab->IsActive());
	for (int32 Frame = 0; Frame < 100 && Grab->IsActive(); ++Frame) { Step(Hook); }
	AddInfo(FString::Printf(TEXT("Return state=%d pos=%s anchor=%s target=%s"), static_cast<int32>(Hook->GetHookPhase()),
		*Hook->GetActorLocation().ToString(), *Hook->GetSourceAnchor(Source).ToString(), *Target->GetActorLocation().ToString()));
	TestFalse(TEXT("Full hit and return cycle ends GA"), Grab->IsActive());
	TestTrue(TEXT("Hit pulls enemy toward caster"), Target->GetActorLocation().X < 200.0f);
	TestTrue(TEXT("Lost hook stays finished while pull completes independently"), Hook->GetHookPhase() == ENF_GrabHookPhase::Finished);
	Target->SetActorLocation(FVector(0, 1000, 100));
	Grab->GrabRange = 200.0f;
	TestTrue(TEXT("Empty aim still fires"), ASC->TryActivateAbility(Handle)); Hook = Grab->ActiveHook;
	const FVector MissOrigin = Hook->GetActorLocation();
	float MaximumTravel = 0.0f;
	for (int32 Frame = 0; Frame < 100 && Grab->IsActive(); ++Frame)
	{
		Step(Hook); MaximumTravel = FMath::Max(MaximumTravel, static_cast<float>(FVector::Distance(MissOrigin, Hook->GetActorLocation())));
	}
	TestTrue(TEXT("Projectile respects maximum range"), MaximumTravel <= 201.0f);
	TestFalse(TEXT("Miss return ends GA"), Grab->IsActive());
	Grab->GrabRange = 600.0f; Target->SetActorLocation(FVector(500, 0, 100));
	AActor* Wall = World->SpawnActor<AActor>();
	auto* Box = NewObject<UBoxComponent>(Wall); Wall->AddInstanceComponent(Box); Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(10, 150, 300)); Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent(); Wall->SetActorLocation(FVector(150, 0, 100));
	TestTrue(TEXT("Shot toward wall launches"), ASC->TryActivateAbility(Handle)); Hook = Grab->ActiveHook;
	bool bPulledThroughWall = false;
	for (int32 Frame = 0; Frame < 100 && Grab->IsActive(); ++Frame)
	{
		Step(Hook); bPulledThroughWall |= Target->GetCharacterMovement()->HasRootMotionSources();
	}
	TestFalse(TEXT("Wall intercepts projectile before enemy"), bPulledThroughWall);
	TestFalse(TEXT("Wall hit retracts hook and ends GA"), Grab->IsActive());
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
