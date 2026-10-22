#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/State/NF_PlayerState.h"
#include "UI/Minimap/NF_MinimapWidget.h"
#include "UI/Minimap/NF_MinimapRevealRegistry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNF_MinimapTeamTest, "NF.UI.Minimap.TeammatesAndClairvoyanceReveal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNF_MinimapTeamTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
		.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Local = World->SpawnActor<ANF_PlayerState>();
	auto* Ally = World->SpawnActor<ANF_PlayerState>();
	auto* Enemy = World->SpawnActor<ANF_PlayerState>();
	TestFalse(TEXT("Two unassigned players are not teammates"), UNF_MinimapWidget::ArePlayerStatesTeammates(Local, Ally));
	Local->SetNFTeamID(1); Ally->SetNFTeamID(1); Enemy->SetNFTeamID(2);
	TestTrue(TEXT("Matching valid team IDs show teammate"), UNF_MinimapWidget::ArePlayerStatesTeammates(Local, Ally));
	TestFalse(TEXT("Different team IDs do not show teammate"), UNF_MinimapWidget::ArePlayerStatesTeammates(Local, Enemy));
	TestFalse(TEXT("Missing PlayerState is not a teammate"), UNF_MinimapWidget::ArePlayerStatesTeammates(Local, nullptr));
	auto* PlainState = World->SpawnActor<APlayerState>();
	TestFalse(TEXT("Basic TPS PlayerState has no NF team"), UNF_MinimapWidget::ArePlayerStatesTeammates(Local, PlainState));
	Ally->SetNFTeamID(2);
	TestFalse(TEXT("Team change immediately updates predicate"), UNF_MinimapWidget::ArePlayerStatesTeammates(Local, Ally));
	Ally->SetNFTeamID(1);
	auto* PC = World->SpawnActor<APlayerController>();
	auto* AllyPawn = World->SpawnActor<APawn>(); AllyPawn->SetPlayerState(Ally);
	auto* EnemyPawn = World->SpawnActor<APawn>(); EnemyPawn->SetPlayerState(Enemy);
	auto* Registry = NewObject<UNF_MinimapRevealRegistry>(PC); Registry->Initialize(PC);
	const FGuid Scan = FGuid::NewGuid(), Key = FGuid::NewGuid();
	TArray<FNF_MinimapRevealMarker> Snapshot;
	Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("No reveal source publishes enemy positions"), Snapshot.Num(), 0);
	Registry->UpdateRevealSource(Scan, { AllyPawn, EnemyPawn }, ENF_MinimapRevealKind::Clairvoyance, 5.0f);
	Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Authorized clairvoyance snapshot contains both teams"), Snapshot.Num(), 2);
	bool bHasAlly = false, bHasEnemy = false;
	for (const auto& Marker : Snapshot)
	{
		bHasAlly |= Marker.PlayerState == Ally && Marker.PlayerTeamID == 1;
		bHasEnemy |= Marker.PlayerState == Enemy && Marker.PlayerTeamID == 2;
	}
	TestTrue(TEXT("Ally metadata supports blue marker and deduplication"), bHasAlly);
	TestTrue(TEXT("Enemy metadata accompanies authorized reveal"), bHasEnemy);
	Registry->RemoveRevealSource(Scan); Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Ending clairvoyance removes non-team reveal positions"), Snapshot.Num(), 0);
	TestTrue(TEXT("Ending clairvoyance does not change teammate relation"), UNF_MinimapWidget::ArePlayerStatesTeammates(Local, Ally));
	Registry->RegisterReveal(Key, EnemyPawn, ENF_MinimapRevealKind::EscapeKey);
	Registry->UpdateRevealSource(Scan, { EnemyPawn }, ENF_MinimapRevealKind::Clairvoyance, 5.0f);
	Registry->RemoveRevealSource(Scan); Registry->BuildServerSnapshot(Snapshot);
	TestEqual(TEXT("Existing escape-key disclosure survives clairvoyance ending"), Snapshot.Num(), 1);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return true;
}
#endif
