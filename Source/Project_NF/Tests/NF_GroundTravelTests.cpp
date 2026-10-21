#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GamePlay/Combat/Components/NF_GroundTravelComponent.h"
#include "GamePlay/Combat/Components/NF_DetonationComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "Monster/NF_Monster.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_GroundTravelTest, "NF.Skills.GroundTravel.GroundWallDistanceAndComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_GroundTravelTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowActorCallbacks(GAllowActorScriptExecutionInEditor, true);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	const auto Box = [World](FVector Location, FVector Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		auto* Shape = NewObject<UBoxComponent>(Actor); Actor->AddInstanceComponent(Shape); Actor->SetRootComponent(Shape);
		Shape->SetBoxExtent(Extent); Shape->SetCollisionObjectType(ECC_WorldStatic);
		Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Shape->SetCollisionResponseToAllChannels(ECR_Block);
		Shape->RegisterComponent(); Actor->SetActorLocation(Location); return Actor;
	};
	AActor* Floor = Box(FVector(0, 0, -20), FVector(2000, 2000, 20));
	ANF_Monster* Caster = World->SpawnActor<ANF_Monster>(FVector(0, 0, 100), FRotator::ZeroRotator);
	auto* ASC = Caster->GetAbilitySystemComponent(); ASC->InitAbilityActorInfo(Caster, Caster);
	const auto Handle = ASC->GiveAbility(FGameplayAbilitySpec(UNFGA_SkillBase::StaticClass(), 1));
	auto* Skill = Cast<UNFGA_SkillBase>(ASC->FindAbilitySpecFromHandle(Handle)->GetPrimaryInstance());
	if (!TestNotNull(TEXT("Granted skill instance"), Skill))
	{
		GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false;
	}
	TestTrue(TEXT("Direction input is accepted"), Skill->PrepareConfirmedTargetData(
		UNFGA_SkillBase::MakeSkillTargetData(ENF_SkillTargetKind::Direction, FVector::ZeroVector, FVector::ForwardVector)));
	const auto Bomb = [World, Caster, Skill]()
	{
		auto* Actor = World->SpawnActor<ANF_SkillExecutionActor>();
		auto* Collision = NewObject<USphereComponent>(Actor); Actor->AddInstanceComponent(Collision);
		Collision->InitSphereRadius(15); Collision->RegisterComponent();
		auto* Travel = NewObject<UNF_GroundTravelComponent>(Actor); Actor->AddInstanceComponent(Travel); Travel->RegisterComponent();
		auto* Detonation = NewObject<UNF_DetonationComponent>(Actor); Actor->AddInstanceComponent(Detonation); Detonation->RegisterComponent();
		Travel->ConfigureComponents(); Actor->InitializeExecution(Caster, Skill); return Actor;
	};
	ANF_SkillExecutionActor* Actor = Bomb();
	auto* Travel = Actor->FindComponentByClass<UNF_GroundTravelComponent>();
	TestEqual(TEXT("Initial burst is twice cruise speed"), Travel->GetTravelSpeedAtTime(0), 2000.0f);
	TestTrue(TEXT("Boost eases down"), FMath::IsNearlyEqual(Travel->GetTravelSpeedAtTime(0.2f), 1250.0f));
	TestEqual(TEXT("Boost ends at cruise speed"), Travel->GetTravelSpeedAtTime(0.4f), 1000.0f);
	TestEqual(TEXT("Cruise remains constant after boost"), Travel->GetTravelSpeedAtTime(2), 1000.0f);
	const float Whole = Travel->CalculateTravelDistance(0, 0.6f);
	const float Split = Travel->CalculateTravelDistance(0, 0.1f) + Travel->CalculateTravelDistance(0.1f, 0.2f)
		+ Travel->CalculateTravelDistance(0.3f, 0.3f);
	TestTrue(TEXT("Substep partition preserves boost distance"), FMath::IsNearlyEqual(Whole, Split, 0.001f));
	Travel->BoostDuration = 0;
	TestEqual(TEXT("Zero duration disables boost"), Travel->GetTravelSpeedAtTime(0), 1000.0f);
	TestEqual(TEXT("Zero duration preserves constant speed distance"), Travel->CalculateTravelDistance(0, 0.1f), 100.0f);
	Travel->BoostDuration = 0.4f; Travel->InitialBoostMultiplier = 1;
	TestEqual(TEXT("Multiplier one disables boost"), Travel->GetTravelSpeedAtTime(0), 1000.0f);
	Travel->InitialBoostMultiplier = 2;
	Travel->MaxTravelDistance = 100;
	TestTrue(TEXT("Ground composition is valid"), Actor->ValidateComposition());
	TestTrue(TEXT("Ground actor prepares and starts"), Actor->StartExecution());
	const FVector Initial = Actor->GetActorLocation();
	Travel->TickComponent(0.016f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Bomb moves forward along ground"), Actor->GetActorLocation().X > Initial.X);
	TestTrue(TEXT("First movement uses burst speed"), Actor->GetActorLocation().X - Initial.X > Travel->TravelSpeed * 0.016f);
	TestTrue(TEXT("Bomb stays above ground"), FMath::IsNearlyEqual(Actor->GetActorLocation().Z, 17.0f, 0.5f));
	TestTrue(TEXT("Ground contact does not detonate"), Actor->GetLifeSpan() <= 0);
	Travel->TickComponent(0.2f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Travel limit detonates and schedules cleanup"), Actor->GetLifeSpan() > 0);
	Actor->Destroy();
	AActor* Wall = Box(FVector(400, 0, 150), FVector(20, 100, 150));
	Actor = Bomb(); Travel = Actor->FindComponentByClass<UNF_GroundTravelComponent>();
	TestTrue(TEXT("Wall test starts"), Actor->StartExecution());
	for (int32 Step = 0; Step < 40 && Actor->GetLifeSpan() <= 0; ++Step) { Travel->TickComponent(0.016f, LEVELTICK_All, nullptr); }
	TestTrue(TEXT("Wall triggers detonation"), Actor->GetLifeSpan() > 0);
	TestTrue(TEXT("Bomb cannot pass through the wall"), Actor->GetActorLocation().X < 400);
	Actor->Destroy(); Wall->Destroy(); Floor->Destroy();
	AActor* EdgeFloor = Box(FVector(0, 0, -20), FVector(250, 100, 20));
	Actor = Bomb(); Travel = Actor->FindComponentByClass<UNF_GroundTravelComponent>();
	TestTrue(TEXT("Edge test starts"), Actor->StartExecution());
	for (int32 Step = 0; Step < 40 && Actor->GetLifeSpan() <= 0; ++Step) { Travel->TickComponent(0.016f, LEVELTICK_All, nullptr); }
	TestTrue(TEXT("Missing ground ahead detonates instead of flying forward"), Actor->GetLifeSpan() > 0);
	Actor->Destroy(); EdgeFloor->Destroy();
	Actor = Bomb(); Travel = Actor->FindComponentByClass<UNF_GroundTravelComponent>();
	TestFalse(TEXT("Missing starting ground rejects placement"), Actor->PrepareExecution());
	Travel->TravelSpeed = 0;
	TestFalse(TEXT("Invalid travel speed is rejected"), Travel->HasValidSettings());
	Travel->TravelSpeed = 1000;
	auto* Conflicting = NewObject<UProjectileMovementComponent>(Actor);
	Actor->AddInstanceComponent(Conflicting); Conflicting->RegisterComponent();
	TestFalse(TEXT("Ground and projectile movements cannot be combined"), Actor->ValidateComposition());
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
