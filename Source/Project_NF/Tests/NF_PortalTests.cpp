#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GamePlay/Combat/Skill/Portal/NF_Portal.h"

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 실제 충돌 월드에서 출구 바닥·캡슐 여유 공간과 쌍의 활성/종료를 검증한다.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_PortalPlacementTest, "NF.Skills.Portal.PlacementAndLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_PortalPlacementTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	const auto Obstacle = [World](FVector Location, FVector Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		auto* Box = NewObject<UBoxComponent>(Actor); Actor->AddInstanceComponent(Box); Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent); Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent(); Actor->SetActorLocation(Location); return Actor;
	};
	AActor* Floor = Obstacle(FVector(0, 0, -20), FVector(10000, 10000, 20));
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Character = World->SpawnActor<ACharacter>(FVector(0, 0, 100), FRotator::ZeroRotator, Params);
	ANF_Portal* Entrance = World->SpawnActor<ANF_Portal>(FVector(300, 0, 0), FRotator::ZeroRotator, Params);
	ANF_Portal* Exit = World->SpawnActor<ANF_Portal>(FVector(1500, 0, 0), FRotator::ZeroRotator, Params);
	Entrance->InitializePortal(Exit, false, Character); Exit->InitializePortal(Entrance, true, Character);
	World->Tick(LEVELTICK_All, 0.016f);
	FVector Arrival;
	TestTrue(TEXT("Portal fits in open area"), Exit->CanPlaceFor(Character));
	TestTrue(TEXT("Open exit has grounded capsule space"), Exit->FindArrivalLocation(Character, Arrival));
	AActor* Ceiling = Obstacle(FVector(1660, 0, 180), FVector(80, 80, 20));
	World->Tick(LEVELTICK_All, 0.016f);
	TestFalse(TEXT("Low ceiling blocks arrival"), Exit->FindArrivalLocation(Character, Arrival));
	Ceiling->Destroy(); Floor->Destroy(); World->Tick(LEVELTICK_All, 0.016f);
	TestFalse(TEXT("No floor blocks arrival"), Exit->FindArrivalLocation(Character, Arrival));
	TestTrue(TEXT("Exit starts before entrance"), Exit->StartPortal());
	TestTrue(TEXT("Entrance starts after exit"), Entrance->StartPortal());
	Entrance->ClosePortalPair();
	TestFalse(TEXT("Entrance closes"), Entrance->IsPortalActive());
	TestFalse(TEXT("Partner exit closes"), Exit->IsPortalActive());
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
