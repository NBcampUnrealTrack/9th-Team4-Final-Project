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

	if (ANF_GameState* NFGameState = GetNFGameState())
	{
		NFGameState->AuthoritySetUntimedMatchPhase(
			ENF_MatchPhase::Waiting
		);
	}
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

	if (!NFGameState ||
		NFGameState->GetMatchPhase() != ENF_MatchPhase::Waiting ||
		GetNumPlayers() != RequiredPlayers ||
		!bTeamsReady)
	{
		return;
	}

	EnterBuild();
}

void ANF_GameMode::EnterBuild()
{
	ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState ||
		!NFGameState->AuthoritySetTimedMatchPhase(
			ENF_MatchPhase::Build,
			BuildDuration))
	{
		return;
	}

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

	if (!NFGameState ||
		NFGameState->GetMatchPhase() != ENF_MatchPhase::Build ||
		!NFGameState->AuthoritySetTimedMatchPhase(
			ENF_MatchPhase::Farming,
			FarmingDuration))
	{
		return;
	}

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

	if (!NFGameState ||
		NFGameState->GetMatchPhase() != ENF_MatchPhase::Farming)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	NFGameState->AuthoritySetUntimedMatchPhase(
		ENF_MatchPhase::Boss
	);

	// 여기서 명우의 보스 출현 시스템에 서버 이벤트 전달
}

void ANF_GameMode::NotifyBossResolved()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState ||
		NFGameState->GetMatchPhase() != ENF_MatchPhase::Boss)
	{
		return;
	}

	EnterEscape();
}

void ANF_GameMode::EnterEscape()
{
	if (ANF_GameState* NFGameState = GetNFGameState())
	{
		NFGameState->AuthoritySetUntimedMatchPhase(
			ENF_MatchPhase::Escape
		);
	}

	// 지우의 부활 시스템에는 '이제 부활 불가'를 전달
}

void ANF_GameMode::NotifyEscapeSucceeded()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState ||
		NFGameState->GetMatchPhase() != ENF_MatchPhase::Escape)
	{
		return;
	}

	EnterFinished();
}

void ANF_GameMode::EnterFinished()
{
	if (ANF_GameState* NFGameState = GetNFGameState())
	{
		NFGameState->AuthoritySetUntimedMatchPhase(
			ENF_MatchPhase::Finished
		);
	}

	// 다음 작업: 승리 팀 기록, 결과 UI용 데이터 복제, 매치 정리
}