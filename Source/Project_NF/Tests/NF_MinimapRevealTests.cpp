#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Minimap/NF_MinimapRevealRegistry.h"

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 공개 출처 합산·우선순위·범위 이탈·만료·대상 소멸을 서버 월드에서 검증.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_MinimapRevealTest, "NF.UI.Minimap.RevealRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_MinimapRevealTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	AActor* Target = World->SpawnActor<AActor>();
	AActor* Other = World->SpawnActor<AActor>();
	auto* Registry = NewObject<UNF_MinimapRevealRegistry>(Controller); Registry->Initialize(Controller);
	const FGuid Scan = FGuid::NewGuid(), SecondScan = FGuid::NewGuid(), Key = FGuid::NewGuid();
	TArray<FNF_MinimapRevealMarker> Snapshot;
	TestFalse(TEXT("Invalid source rejected"), Registry->RegisterReveal(FGuid(), Target, ENF_MinimapRevealKind::Clairvoyance));
	TestTrue(TEXT("First scan registered"), Registry->RegisterReveal(Scan, Target, ENF_MinimapRevealKind::Clairvoyance, 5));
	Registry->RegisterReveal(SecondScan, Target, ENF_MinimapRevealKind::Clairvoyance, 5);
	Registry->RegisterReveal(Key, Target, ENF_MinimapRevealKind::EscapeKey);
	Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Three sources merge into one icon"), Snapshot.Num(), 1);
	if (!Snapshot.IsEmpty()) { TestTrue(TEXT("Key display wins"), Snapshot[0].Kind == ENF_MinimapRevealKind::EscapeKey); }
	Registry->RemoveRevealSource(Scan); Registry->RemoveRevealSource(SecondScan);
	Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Scan ending preserves key"), Snapshot.Num(), 1);
	Registry->RegisterReveal(Scan, Target, ENF_MinimapRevealKind::Clairvoyance, 5);
	Registry->RemoveReveal(Key, Target); Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Scan remains after key removal"), Snapshot.Num(), 1);
	if (!Snapshot.IsEmpty()) { TestTrue(TEXT("Key removal falls back to scan"), Snapshot[0].Kind == ENF_MinimapRevealKind::Clairvoyance); }
	TArray<AActor*> Targets { Other };
	const FName PreviousId = Snapshot.IsEmpty() ? NAME_None : Snapshot[0].TargetId;
	Registry->UpdateRevealSource(Scan, Targets, ENF_MinimapRevealKind::Clairvoyance, 0.05f);
	Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Range reconciliation replaces exited target"), Snapshot.Num(), 1);
	if (!Snapshot.IsEmpty()) { TestTrue(TEXT("Target identity changed"), Snapshot[0].TargetId != PreviousId); }
	World->Tick(LEVELTICK_TimeOnly, 0.1f); Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Expired source disappears"), Snapshot.Num(), 0);
	Registry->RegisterReveal(Key, Target, ENF_MinimapRevealKind::EscapeKey); Target->Destroy(); Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Destroyed target disappears"), Snapshot.Num(), 0);
	Registry->RegisterReveal(Key, Other, ENF_MinimapRevealKind::EscapeKey); Registry->Clear(); Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Cleanup empties registry"), Snapshot.Num(), 0);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return true;
}
#endif
