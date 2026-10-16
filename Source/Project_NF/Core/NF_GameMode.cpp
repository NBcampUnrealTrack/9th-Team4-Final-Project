#include "Core/NF_GameMode.h"

#include "Core/NF_GameState.h"
#include "TimerManager.h"
#include "Component/NF_BuildReadyComponent.h"
#include "Component/NF_MatchDepartureComponent.h"
#include "Player/State/NF_PlayerState.h"
#include "GameFramework/PlayerController.h"

ANF_GameMode::ANF_GameMode()
{
	GameStateClass = ANF_GameState::StaticClass();
	BuildReadyComponent = CreateDefaultSubobject<UNF_BuildReadyComponent>(TEXT("BuildReadyComponent"));
	MatchDepartureComponent = CreateDefaultSubobject<UNF_MatchDepartureComponent>(TEXT("MatchDepartureComponent"));
}

void ANF_GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->OnDepartureLimitReached.AddUObject(this,&ThisClass::AbortMatchDueToDepartures);
	}
	
	ANF_GameState* NFGameState = GetNFGameState();
	if (!NFGameState)return;

	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Waiting);
}

void ANF_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	
	if (IsValid(NewPlayer))
	{
		BuildReadyComponent->RegisterPlayer(NewPlayer->GetPlayerState<ANF_PlayerState>());
	}
	
	TryStartBuild();
}

void ANF_GameMode::Logout(AController* Exiting)
{
	ANF_PlayerState* LeavingPlayerState = nullptr;

	if (IsValid(Exiting))
	{
		LeavingPlayerState =Exiting->GetPlayerState<ANF_PlayerState>();
	}

	if (IsValid(LeavingPlayerState))
	{
		if (IsValid(BuildReadyComponent))
		{
			BuildReadyComponent->UnregisterPlayer(LeavingPlayerState);
		}

		if (IsValid(MatchDepartureComponent))
		{
			MatchDepartureComponent->NotifyPlayerLeft(TWeakObjectPtr<ANF_PlayerState>(LeavingPlayerState));
		}
	}

	Super::Logout(Exiting);
}

void ANF_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->StopTracking();

		MatchDepartureComponent->OnDepartureLimitReached.RemoveAll(this);
	}

	RequestMatchAbort.Unbind();
	
	Super::EndPlay(EndPlayReason);
}

ANF_GameState* ANF_GameMode::GetNFGameState() const
{
	return GetGameState<ANF_GameState>();
}

bool ANF_GameMode::CollectMatchParticipants(TArray<ANF_PlayerState*>& OutParticipants) const
{
	OutParticipants.Reset();

	for (FConstPlayerControllerIterator Iterator =GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* Controller = Iterator->Get();

		if (!IsValid(Controller)){return false;}

		ANF_PlayerState* PlayerState =Controller->GetPlayerState<ANF_PlayerState>();

		if (!IsValid(PlayerState)){return false;}

		// 현재 구조에서는 접속 플레이어 모두가 출격 대상이다.
		OutParticipants.AddUnique(PlayerState);
	}

	return OutParticipants.Num() == InitialRequiredPlayers;
}

UNF_BuildReadyComponent*ANF_GameMode::GetBuildReadyComponent() const
{
	return BuildReadyComponent.Get();
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
	if (GetNumPlayers() != InitialRequiredPlayers)return;
	if (!bTeamsReady) return;

	EnterBuild();
}

void ANF_GameMode::EnterBuild()
{
	ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState)) return;
	if (!IsValid(MatchDepartureComponent)) return;
	
	TArray<ANF_PlayerState*> Participants;

	if (!CollectMatchParticipants(Participants))
	{
		UE_LOG(LogTemp,Error,TEXT("Build 진입 실패: 매치 참가자 수집 실패"));
		return;
	}

	if (!MatchDepartureComponent->StartTracking(Participants))
	{
		UE_LOG(LogTemp,Error,TEXT("Build 진입 실패: 참가자 이탈 추적 시작 실패"));
		return;
	}

	if (!NFGameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Build,BuildDuration))
	{
		// 페이즈 전환 실패 시 시작한 추적 중지
		MatchDepartureComponent->StopTracking();
		return;
	}

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ThisClass::HandleBuildPhaseExpired,
		BuildDuration,
		false);
}

void ANF_GameMode::HandleBuildPhaseExpired()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState)) return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Build) return;
	
	CurrentPlayerCount = GetNumPlayers();
	if (CurrentPlayerCount <= 0)
	{
		UE_LOG(LogTemp,Warning,TEXT("접속 플레이어가 없어 Farming 전환을 중단합니다."));

		// 전원 이탈 시 매치 종료/서버 정리 정책을 연결할 위치.
		return;
	}
	
	if (!BuildReadyComponent->ResolveUnreadyPlayers(CurrentPlayerCount))
	{
		UE_LOG(LogTemp,Error,TEXT("출격 조건 미충족: Farming 전환 보류"));

		// 매치 실패/복구 정책을 연결할 위치.
		return;
	}

	EnterFarming();
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
	
	if (!IsValid(NFGameState)) return;
	
	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->StopTracking();
	}

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	
	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Finished);
	
	// 승리 팀 기록, 결과 UI용 데이터 복제, 매치 정리 등
}

void ANF_GameMode::AbortMatchDueToDepartures()
{
	if (bMatchAborting) return;
	
	bMatchAborting = true;

	MatchDepartureComponent->StopTracking();

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	if (ANF_GameState* NFGameState = GetNFGameState())
	{
		NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Finished);
	}

	// 정상 승리 종료인 EnterFinished()는 호출하지 않는다.
	const FString Reason = TEXT("참가자 이탈 한도에 도달하여 매치가 중단되었습니다.");

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Reason);

	if (!RequestMatchAbort.IsBound())
	{
		UE_LOG(LogTemp,Error,TEXT("매치 복귀 처리 실패: RequestMatchAbort 미바인딩"));

		return;
	}

	RequestMatchAbort.Execute(Reason);
}


