#include "Core/NF_GameMode.h"

#include "Core/NF_GameState.h"
#include "Core/Match/NF_DedicatedMatchSubsystem.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Component/NF_BossEncounterComponent.h"
#include "Component/NF_BuildReadyComponent.h"
#include "Component/NF_MatchDepartureComponent.h"
#include "Player/State/NF_PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"


ANF_GameMode::ANF_GameMode()
{
	GameStateClass = ANF_GameState::StaticClass();
	PlayerStateClass = ANF_PlayerState::StaticClass();
	BuildReadyComponent = CreateDefaultSubobject<UNF_BuildReadyComponent>(TEXT("BuildReadyComponent"));
	MatchDepartureComponent = CreateDefaultSubobject<UNF_MatchDepartureComponent>(TEXT("MatchDepartureComponent"));
	BossEncounterComponent = CreateDefaultSubobject<UNF_BossEncounterComponent>(TEXT("BossEncounterComponent"));
}

void ANF_GameMode::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UNF_DedicatedMatchSubsystem* MatchSubsystem = GameInstance->GetSubsystem<UNF_DedicatedMatchSubsystem>())
			{
				RequestMatchAbort.BindUObject(MatchSubsystem, &UNF_DedicatedMatchSubsystem::HandleMatchAbort);

				MatchSubsystem->OnServerTeamIdsAssigned.AddUObject(this, &ThisClass::HandleServerTeamIdsAssigned);
			}
		}
	}

	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->OnDepartureLimitReached.AddUObject(this, &ThisClass::AbortMatchDueToDepartures);
	}

	ANF_GameState* NFGameState = GetNFGameState();
	if (!NFGameState)return;

	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Waiting);

	BuildReadyComponent->OnBuildResolveSucceeded.AddUObject(this, &ThisClass::HandleBuildResolveSucceeded);
	BuildReadyComponent->OnBuildResolveFailed.AddUObject(this, &ThisClass::HandleBuildResolveFailed);

	BossEncounterComponent->OnBossSpawnSucceeded.AddUObject(this, &ThisClass::HandleBossSpawnSucceeded);
	BossEncounterComponent->OnBossSpawnFailed.AddUObject(this, &ThisClass::HandleBossSpawnFailed);
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
		LeavingPlayerState = Exiting->GetPlayerState<ANF_PlayerState>();
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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNF_DedicatedMatchSubsystem* MatchSubsystem = GameInstance->GetSubsystem<UNF_DedicatedMatchSubsystem>())
		{
			MatchSubsystem->OnServerTeamIdsAssigned.RemoveAll(this);
		}
	}
	if (IsValid(BossEncounterComponent))
	{
		BossEncounterComponent->CancelEncounter();

		BossEncounterComponent->OnBossSpawnSucceeded.RemoveAll(this);
		BossEncounterComponent->OnBossSpawnFailed.RemoveAll(this);
	}
	
	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->StopTracking();

		MatchDepartureComponent->OnDepartureLimitReached.RemoveAll(this);
	}

	RequestMatchAbort.Unbind();

	BuildReadyComponent->CancelResolve();

	BuildReadyComponent->OnBuildResolveSucceeded.RemoveAll(this);
	BuildReadyComponent->OnBuildResolveFailed.RemoveAll(this);

	Super::EndPlay(EndPlayReason);
}

ANF_GameState* ANF_GameMode::GetNFGameState() const
{
	return GetGameState<ANF_GameState>();
}

bool ANF_GameMode::CollectMatchParticipants(TArray<ANF_PlayerState*>& OutParticipants) const
{
	OutParticipants.Reset();

	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* Controller = Iterator->Get();

		if (!IsValid(Controller)) { return false; }

		ANF_PlayerState* PlayerState = Controller->GetPlayerState<ANF_PlayerState>();

		if (!IsValid(PlayerState)) { return false; }

		// 현재 구조에서는 접속 플레이어 모두가 출격 대상이다.
		OutParticipants.AddUnique(PlayerState);
	}

	return OutParticipants.Num() == InitialRequiredPlayers;
}

UNF_BuildReadyComponent* ANF_GameMode::GetBuildReadyComponent() const
{
	return BuildReadyComponent.Get();
}

bool ANF_GameMode::RegisterFallbackBuildHandler(const FNF_ApplyFallbackBuild& Handler)
{
	if (!HasAuthority() || !IsValid(BuildReadyComponent) || !Handler.IsBound()) { return false; }

	// 다른 시스템이 등록한 핸들러를 덮어쓰지 않는다.
	if (BuildReadyComponent->ApplyFallbackBuild.IsBound()) { return false; }

	BuildReadyComponent->ApplyFallbackBuild = Handler;
	return true;
}

void ANF_GameMode::UnregisterFallbackBuildHandler(const UObject* HandlerOwner)
{
	if (!HasAuthority() || !IsValid(BuildReadyComponent) || !IsValid(HandlerOwner)) return;


	FNF_ApplyFallbackBuild& Handler = BuildReadyComponent->ApplyFallbackBuild;

	// 자신이 소유한 바인딩만 해제한다.
	if (Handler.IsBoundToObject(HandlerOwner))
	{
		Handler.Unbind();
	}
}

bool ANF_GameMode::RegisterBossSpawnHandler(const FNF_RequestBossSpawn& Handler)
{
	if (!HasAuthority() ||bMatchAborting ||!IsValid(BossEncounterComponent) ||!Handler.IsBound() ||
		!IsValid(Handler.GetUObject()) ||!BossEncounterComponent->CanRegisterSpawnHandler())
	{
		return false;
	}

	BossEncounterComponent->RequestBossSpawn = Handler;
	return true;
}

void ANF_GameMode::UnregisterBossSpawnHandler(const UObject* HandlerOwner)
{
	if (!HasAuthority() ||!IsValid(BossEncounterComponent) ||!IsValid(HandlerOwner)) return;

	FNF_RequestBossSpawn& Handler =BossEncounterComponent->RequestBossSpawn;

	if (Handler.IsBoundToObject(HandlerOwner))
	{
		Handler.Unbind();
	}
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
		UE_LOG(LogTemp, Error, TEXT("Build 진입 실패: 매치 참가자 수집 실패"));
		return;
	}

	if (!MatchDepartureComponent->StartTracking(Participants))
	{
		UE_LOG(LogTemp, Error, TEXT("Build 진입 실패: 참가자 이탈 추적 시작 실패"));
		return;
	}

	if (!NFGameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Build, BuildDuration))
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
	if (bMatchAborting) return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Build) return;

	if (!IsValid(BuildReadyComponent))
	{
		AbortMatch(TEXT("빌드 준비 컴포넌트가 없습니다."));
		return;
	}

	BuildReadyComponent->BeginResolve();
}

void ANF_GameMode::EnterFarming()
{
	ANF_GameState* NFGameState = GetNFGameState();
	if (!HasAuthority()) return;
	if (bMatchAborting) return;
	if (!IsValid(NFGameState))return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Build) return;

	if (!NFGameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Farming, FarmingDuration))
	{
		AbortMatch(TEXT("파밍 페이즈 진입에 실패했습니다."));
		return;
	}

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ThisClass::HandleFarmingPhaseExpired,
		FarmingDuration,
		false);
}

void ANF_GameMode::EnterBoss()
{
	ANF_GameState* NFGameState = GetNFGameState();

	if (!HasAuthority() ||bMatchAborting ||!IsValid(NFGameState))return;

	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Farming)return;

	if (!NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Boss))
	{
		AbortMatch(TEXT("보스 페이즈 전환에 실패했습니다."));
	}
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
	
	if (IsValid(BossEncounterComponent))
	{
		BossEncounterComponent->CancelEncounter();
	}
	
	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->StopTracking();
	}

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Finished);

	BuildReadyComponent->CancelResolve();
	// 승리 팀 기록, 결과 UI용 데이터 복제, 매치 정리 등
}

void ANF_GameMode::AbortMatchDueToDepartures()
{
	AbortMatch(TEXT("참가자 이탈 한도에 도달하여 매치가 중단되었습니다."));
}

void ANF_GameMode::HandleBuildResolveSucceeded()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState) || bMatchAborting) return;

	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Build) return;

	EnterFarming();
}

void ANF_GameMode::HandleBuildResolveFailed(const FString& Reason)
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState) || bMatchAborting) return;


	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Build) return;

	AbortMatch(Reason);
}

void ANF_GameMode::AbortMatch(const FString& Reason)
{
	if (bMatchAborting) return;

	bMatchAborting = true;

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	if (IsValid(BuildReadyComponent))
	{
		BuildReadyComponent->CancelResolve();
	}
	
	if (IsValid(BossEncounterComponent))
	{
		BossEncounterComponent->CancelEncounter();
	}
	
	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->StopTracking();
	}

	if (ANF_GameState* NFGameState = GetNFGameState())
	{
		NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Finished);
	}

	UE_LOG(LogTemp, Error, TEXT("%s"), *Reason);

	if (RequestMatchAbort.IsBound())
	{
		RequestMatchAbort.Execute(Reason);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("매치 복귀 처리 실패: RequestMatchAbort 미바인딩"));
	}
}

void ANF_GameMode::HandleServerTeamIdsAssigned(const TArray<FNFServerTeamIdAssignment>& Assignments)
{
	if (!HasAuthority() || bTeamsReady || Assignments.Num() != InitialRequiredPlayers) return;

	NotifyTeamsReady();
}

UNF_BossEncounterComponent* ANF_GameMode::GetBossEncounterComponent() const
{
	return BossEncounterComponent.Get();
}

void ANF_GameMode::HandleFarmingPhaseExpired()
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!HasAuthority() ||bMatchAborting ||!IsValid(NFGameState))return;
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Farming)return;
	

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);

	if (!IsValid(BossEncounterComponent))
	{
		AbortMatch(TEXT("보스 출현 컴포넌트가 없습니다."));
		return;
	}

	BossEncounterComponent->RequestSpawn();
}

void ANF_GameMode::HandleBossSpawnSucceeded(ANF_BossBase* Boss)
{
	if (!HasAuthority() || bMatchAborting)
	{
		return;
	}

	if (!IsValid(BossEncounterComponent) ||
		!IsValid(Boss) ||
		BossEncounterComponent->GetActiveBoss() != Boss)
	{
		AbortMatch(TEXT("보스 출현 결과를 확인할 수 없습니다."));
		return;
	}

	EnterBoss();
}

void ANF_GameMode::HandleBossSpawnFailed(const FString& Reason)
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!HasAuthority() ||bMatchAborting ||!IsValid(NFGameState))return;

	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Farming)return;

	AbortMatch(Reason);
}


