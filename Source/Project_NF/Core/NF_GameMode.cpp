#include "Core/NF_GameMode.h"

#include "Core/NF_GameState.h"
#include "TimerManager.h"

ANF_GameMode::ANF_GameMode()
{
	GameStateClass = ANF_GameState::StaticClass();
}

void ANF_GameMode::BeginPlay()
{
	Super::BeginPlay();

	ANF_GameState* NFGameState = GetNFGameState();
	if (!NFGameState)return;

	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Waiting);
}

void ANF_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	TryStartBuild();
}

ANF_GameState* ANF_GameMode::GetNFGameState() const
{
	return GetGameState<ANF_GameState>();
}

void ANF_GameMode::NotifyTeamsReady()
{
	bTeamsReady = true;
	TryStartBuild();
}

void ANF_GameMode::TryStartBuild()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState) return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Waiting)return;;
	if (GetNumPlayers() != RequiredPlayers)return;
	if (!bTeamsReady) return;

	EnterBuild();
}

void ANF_GameMode::EnterBuild()
{
	ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState)return;
	if (!NFGameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Build, BuildDuration)) return;

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ANF_GameMode::EnterFarming,
		BuildDuration,
		false
	);
}

void ANF_GameMode::EnterFarming()
{
	ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState)return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Build)return;
	if (!NFGameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Farming, FarmingDuration)) return;

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ANF_GameMode::EnterBoss,
		FarmingDuration,
		false
	);
}

void ANF_GameMode::EnterBoss()
{
	ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState) return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Farming) return;


	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Boss);

	// 보스 출현 시스템에 서버 이벤트 전달
}

void ANF_GameMode::NotifyBossResolved()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState) return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Boss) return;

	EnterEscape();
}

void ANF_GameMode::EnterEscape()
{
	ANF_GameState* NFGameState = GetNFGameState();
	
	if (!NFGameState) return;
	
	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Escape);

	//부활 시스템에 부활 불가 상태 전달
}

void ANF_GameMode::NotifyEscapeSucceeded()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState) return;;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Escape) return;

	EnterFinished();
}

void ANF_GameMode::EnterFinished()
{
	ANF_GameState* NFGameState = GetNFGameState();
	
	if (!NFGameState) return;
	
	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Finished);
	

	// 승리 팀 기록, 결과 UI용 데이터 복제, 매치 정리 등
}
