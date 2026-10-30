#include "Core/NF_GameMode.h"

#include "Core/NF_GameState.h"
#include "Core/Match/NF_DedicatedMatchSubsystem.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Component/NF_BossEncounterComponent.h"
#include "Component/NF_BossResolutionComponent.h"
#include "Component/NF_BuildReadyComponent.h"
#include "Component/NF_MatchDepartureComponent.h"
#include "Player/State/NF_PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Player/Controller/NF_PlayerController.h"
#include "UI/Minimap/NF_MinimapRevealRegistry.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "Data/Struct/NF_BossDefeatResult.h"
#include "Component/NF_BossContributionRewardComponent.h"
#include "Component/NF_EscapeKeyComponent.h"
#include "AbilitySystemComponent.h"
#include "Component/NF_EscapeComponent.h"
#include "Component/NF_PlayerRespawnComponent.h"
#include "Core/Spawn/NF_TeamPlayerStart.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
#include "GamePlay/EscapeZone/NF_EscapeZone.h"
#include "GAS/Tags/NFTag_LifeState.h"

ANF_GameMode::ANF_GameMode()
{
	GameStateClass = ANF_GameState::StaticClass();
	PlayerStateClass = ANF_PlayerState::StaticClass();
	BuildReadyComponent = CreateDefaultSubobject<UNF_BuildReadyComponent>(TEXT("BuildReadyComponent"));
	MatchDepartureComponent = CreateDefaultSubobject<UNF_MatchDepartureComponent>(TEXT("MatchDepartureComponent"));
	BossEncounterComponent = CreateDefaultSubobject<UNF_BossEncounterComponent>(TEXT("BossEncounterComponent"));
	BossResolutionComponent =CreateDefaultSubobject<UNF_BossResolutionComponent>(TEXT("BossResolutionComponent"));
	BossContributionRewardComponent =CreateDefaultSubobject<UNF_BossContributionRewardComponent>(TEXT("BossContributionRewardComponent"));
	EscapeKeyComponent =CreateDefaultSubobject<UNF_EscapeKeyComponent>(TEXT("EscapeKeyComponent"));
	EscapeComponent = CreateDefaultSubobject<UNF_EscapeComponent>(TEXT("EscapeComponent"));
	PlayerRespawnComponent = CreateDefaultSubobject<UNF_PlayerRespawnComponent>(TEXT("PlayerRespawnComponent"));
}

bool ANF_GameMode::ShouldUseDedicatedMatchSubsystem() const
{
	return true;
}

void ANF_GameMode::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(PlayerRespawnComponent))
	{
		PlayerRespawnComponent->EvaluateRespawnPermission.BindUObject(
			this,
			&ThisClass::CanRespawnPlayer);
		OnMatchCleanup.AddUObject(
			PlayerRespawnComponent.Get(),
			&UNF_PlayerRespawnComponent::Shutdown);
	}

	if (HasAuthority() && ShouldUseDedicatedMatchSubsystem())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UNF_DedicatedMatchSubsystem* MatchSubsystem = GameInstance->GetSubsystem<UNF_DedicatedMatchSubsystem>())
			{
				RequestMatchAbort.BindUObject(MatchSubsystem, &UNF_DedicatedMatchSubsystem::HandleMatchAbort);
				RequestMatchCompleted.BindUObject(MatchSubsystem, &UNF_DedicatedMatchSubsystem::HandleMatchCompleted);

				MatchSubsystem->OnServerTeamIdsAssigned.AddUObject(this, &ThisClass::HandleServerTeamIdsAssigned);
			}
		}
	}

	if (IsValid(MatchDepartureComponent))
	{
		MatchDepartureComponent->OnDepartureLimitReached.AddUObject(this, &ThisClass::AbortMatchDueToDepartures);
	}

	if (IsValid(BossResolutionComponent))
	{
		BossResolutionComponent->OnBossDefeatResolved.AddUObject(this,&ThisClass::HandleBossDefeatResolved);
	}

	if (IsValid(BossContributionRewardComponent))
	{
		OnMatchCleanup.AddUObject(BossContributionRewardComponent.Get(), &UNF_BossContributionRewardComponent::CleanupTeamPenalty);
	}
	
	if (IsValid(EscapeKeyComponent))
	{
		EscapeKeyComponent->OnInitialKeyAssignmentFailed.AddUObject(this,&ThisClass::HandleInitialKeyAssignmentFailed);

		OnMatchCleanup.AddUObject(EscapeKeyComponent.Get(),&UNF_EscapeKeyComponent::CleanupKey);
		OnMatchPlayerLeaving.AddUObject(EscapeKeyComponent.Get(),&UNF_EscapeKeyComponent::HandlePlayerLeaving);

		EscapeKeyComponent->OnKeyOperationFailed.AddUObject(this,&ThisClass::HandleKeyOperationFailed);

		KeyRevealSourceId = FGuid::NewGuid();
		EscapeKeyComponent->OnKeyStateChanged.AddUObject(this, &ThisClass::HandleKeyStateChanged);
		HandleKeyStateChanged(EscapeKeyComponent->GetPublicState());
	}
	
	if (IsValid(EscapeComponent))
	{
		EscapeComponent->EvaluateEscapeRequest.BindUObject(this, &ThisClass::EvaluateEscapeRequest);
		EscapeComponent->OnEscapeSucceeded.AddUObject(this, &ThisClass::NotifyEscapeSucceeded);
		OnMatchCleanup.AddUObject(EscapeComponent.Get(), &UNF_EscapeComponent::CancelEscape);
		OnMatchPlayerLeaving.AddUObject(EscapeComponent.Get(), &UNF_EscapeComponent::HandlePlayerLeaving);
	}

	ANF_GameState* NFGameState = GetNFGameState();
	if (!NFGameState)return;

	BuildReadyComponent->OnBuildResolveSucceeded.AddUObject(this, &ThisClass::HandleBuildResolveSucceeded);
	BuildReadyComponent->OnBuildResolveFailed.AddUObject(this, &ThisClass::HandleBuildResolveFailed);

	BossEncounterComponent->OnBossSpawnSucceeded.AddUObject(this, &ThisClass::HandleBossSpawnSucceeded);
	BossEncounterComponent->OnBossSpawnFailed.AddUObject(this, &ThisClass::HandleBossSpawnFailed);
	
	StartWaiting();
}

void ANF_GameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (IsValid(NewPlayer))
	{
		ANF_PlayerState* PlayerState = NewPlayer->GetPlayerState<ANF_PlayerState>();

		BuildReadyComponent->RegisterPlayer(PlayerState);

		if (IsValid(PlayerRespawnComponent))
		{
			PlayerRespawnComponent->RegisterPlayer(PlayerState);
		}
	}

	if (ANF_PlayerController* NFPlayerController = Cast<ANF_PlayerController>(NewPlayer))
	{
		SyncKeyMinimapForPlayer(NFPlayerController);
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
		if (HasAuthority())
		{
			if (IsValid(PlayerRespawnComponent))
			{
				PlayerRespawnComponent->HandlePlayerLeaving(LeavingPlayerState);
			}

			OnMatchPlayerLeaving.Broadcast(LeavingPlayerState,Exiting->GetPawn());
		}
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

void ANF_GameMode::RestartPlayer(AController* NewPlayer)
{
	if (!HasAuthority() || bMatchFinalized || bMatchCleanupNotified || !IsValid(NewPlayer))return;

	ANF_PlayerState* PlayerState = NewPlayer->GetPlayerState<ANF_PlayerState>();
	if (!IsValid(PlayerState))return;

	// 팀 미배정 상태에서는 기본 PlayerStart로 Pawn을 생성하지 않는다.
	// 팀 배정 완료 콜백에서 최초 스폰을 다시 요청한다.
	if (PlayerState->NF_TeamID < 1 || PlayerState->NF_TeamID > 4)return;
	if (!IsValid(PlayerRespawnComponent))return;

	ANF_TeamPlayerStart* TeamStart = PlayerRespawnComponent->FindTeamPlayerStart(PlayerState);
	if (!IsValid(TeamStart))
	{
		AbortMatch(FString::Printf(
			TEXT("팀 스폰 지점이 부족합니다. 팀원 수만큼 NF_TeamPlayerStart를 배치하세요. TeamId=%d"),
			PlayerState->NF_TeamID), ENF_MatchEndReason::RuntimeFailure);
		return;
	}

	RestartPlayerAtPlayerStart(NewPlayer, TeamStart);
	if (!IsValid(NewPlayer->GetPawn()))
	{
		AbortMatch(FString::Printf(TEXT("팀 PlayerStart에서 Pawn 생성 실패. PlayerId=%d, Start=%s"),
			PlayerState->GetPlayerId(), *GetNameSafe(TeamStart)), ENF_MatchEndReason::RuntimeFailure);
		return;
	}

	if (IsValid(PlayerRespawnComponent))
	{
		PlayerRespawnComponent->HandlePlayerRestarted(NewPlayer);
	}
}

void ANF_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(PlayerRespawnComponent))
	{
		PlayerRespawnComponent->Shutdown();
		OnMatchCleanup.RemoveAll(PlayerRespawnComponent.Get());
	}

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	GetWorldTimerManager().ClearTimer(ResultDisplayTimerHandle);
	NotifyMatchCleanup();

	if (IsValid(BossContributionRewardComponent))
	{
		BossContributionRewardComponent->CleanupTeamPenalty();
		OnMatchCleanup.RemoveAll(BossContributionRewardComponent.Get());
	}

	if (IsValid(EscapeComponent))
	{
		EscapeComponent->CancelEscape();
		EscapeComponent->EvaluateEscapeRequest.Unbind();
		EscapeComponent->OnEscapeSucceeded.RemoveAll(this);
		OnMatchCleanup.RemoveAll(EscapeComponent.Get());
		OnMatchPlayerLeaving.RemoveAll(EscapeComponent.Get());
	}
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UNF_DedicatedMatchSubsystem* MatchSubsystem = GameInstance->GetSubsystem<UNF_DedicatedMatchSubsystem>())
		{
			MatchSubsystem->OnServerTeamIdsAssigned.RemoveAll(this);
				if (RequestMatchCompleted.IsBoundToObject(MatchSubsystem)) RequestMatchCompleted.Unbind();
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
	
	if (IsValid(EscapeKeyComponent))
	{
		EscapeKeyComponent->OnInitialKeyAssignmentFailed.RemoveAll(this);
		OnMatchCleanup.RemoveAll(EscapeKeyComponent.Get());
		
		EscapeKeyComponent->OnKeyOperationFailed.RemoveAll(this);
		OnMatchPlayerLeaving.RemoveAll(EscapeKeyComponent.Get());
		EscapeKeyComponent->OnKeyStateChanged.RemoveAll(this);
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

ENF_PlayerPawnState ANF_GameMode::GetPlayerPawnState(
	const ANF_PlayerState* PlayerState) const
{
	if (!IsValid(PlayerRespawnComponent))
	{
		return ENF_PlayerPawnState::Unknown;
	}

	return PlayerRespawnComponent->GetPlayerPawnState(PlayerState);
}

bool ANF_GameMode::CanRespawnPlayer(
	const ANF_PlayerState* PlayerState) const
{
	if (!HasAuthority() ||
		bMatchAborting ||
		bMatchFinalized ||
		bMatchCleanupNotified ||
		!IsValid(PlayerState))
	{
		return false;
	}

	if (PlayerState->NF_TeamID < 1 || PlayerState->NF_TeamID > 4)
	{
		return false;
	}

	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState))return false;

	switch (NFGameState->GetMatchPhase())
	{
	case ENF_MatchPhase::Build:
	case ENF_MatchPhase::Farming:
	case ENF_MatchPhase::Boss:
		return true;

	case ENF_MatchPhase::Waiting:
	case ENF_MatchPhase::Escape:
	case ENF_MatchPhase::Finished:
	default:
		return false;
	}
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
	if (!HasAuthority() || bMatchFinalized)return;

	bTeamsReady = true;

	if (IsValid(PlayerRespawnComponent))
	{
		PlayerRespawnComponent->NotifyTeamsReady();
	}

	TryStartBuild();
}

bool ANF_GameMode::NotifyBossDefeated(const FNF_BossDefeatResult& DefeatResult)
{
	if (!HasAuthority() || bMatchAborting){return false;}

	if  (!IsValid(BossResolutionComponent) ||!IsValid(BossEncounterComponent)){return false;}
	
	return BossResolutionComponent->ResolveBossDefeat(DefeatResult,BossEncounterComponent->GetActiveBoss());
}

void ANF_GameMode::NotifyEscapeSucceeded(int32 WinningTeamId)
{
	if (!HasAuthority() || bMatchFinalized)return;

	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState) ||NFGameState->GetMatchPhase() != ENF_MatchPhase::Escape)return;

	if (!IsValid(EscapeKeyComponent))return;

	const ANF_PlayerState* Holder =EscapeKeyComponent->GetKeyHolder();

	// 승리 팀은 현재 열쇠 보유자의 팀과 일치해야 한다.
	if (!IsValid(Holder) ||
		!NFGameState->PlayerArray.Contains(Holder) ||
		WinningTeamId <= 0 ||
		Holder->NF_TeamID != WinningTeamId)
	{
		return;
	}

	FNF_MatchResult Result;
	Result.Outcome = ENF_MatchOutcome::Victory;
	Result.EndReason = ENF_MatchEndReason::EscapeSucceeded;
	Result.WinningTeamId = WinningTeamId;

	FinishMatch(Result);
}

void ANF_GameMode::TryStartBuild()
{
	if (!HasAuthority() ||!bWaitingStarted ||bMatchFinalized ||bMatchAborting)return;
	
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState) ||NFGameState->GetMatchPhase() != ENF_MatchPhase::Waiting)return;

	// 타이머 콜백보다 접속·팀 배정 이벤트가 먼저 처리돼도 마감 시각 이후에는 시작시키지 않는다.
	if (NFGameState->GetRemainingPhaseTime() <= 0.0)
	{
		HandleWaitingPhaseExpired();
		return;
	}

	if (GetNumPlayers() != InitialRequiredPlayers || !bTeamsReady)return;

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
	
	if (bMatchFinalized)return;

	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
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

bool ANF_GameMode::TryStartEscapeTest(ANF_PlayerState* Requester, FString& OutMessage)
{
	OutMessage.Empty();
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	OutMessage = TEXT("이 빌드에서는 테스트 치트를 사용할 수 없습니다.");
	return false;
#else
	if (!HasAuthority() || GetWorld()->WorldType != EWorldType::PIE || ShouldUseDedicatedMatchSubsystem())
	{
		OutMessage = TEXT("PIE 테스트 게임모드의 서버에서만 실행할 수 있습니다.");
		return false;
	}

	ANF_GameState* NFGameState = GetNFGameState();
	if (!IsValid(NFGameState) || bMatchFinalized || bMatchAborting || bMatchCleanupNotified)
	{
		OutMessage = TEXT("종료되었거나 준비되지 않은 매치에서는 실행할 수 없습니다.");
		return false;
	}

	if (!bTeamsReady)
	{
		OutMessage = TEXT("팀 배정, 최초 스폰 및 GAS 연결이 완료된 뒤 실행하세요.");
		return false;
	}

	const ENF_MatchPhase Phase = NFGameState->GetMatchPhase();
	if (Phase != ENF_MatchPhase::Build && Phase != ENF_MatchPhase::Farming)
	{
		OutMessage = TEXT("NF_TestEscape는 Build 또는 Farming에서 한 번 실행할 수 있습니다. 현재 상태는 NF_PrintMatchState로 확인하세요.");
		return false;
	}

	if (!IsValid(Requester) || Requester->GetWorld() != GetWorld() ||
		!NFGameState->PlayerArray.Contains(Requester) || Requester->NF_TeamID < 1 || Requester->NF_TeamID > 4)
	{
		OutMessage = TEXT("명령을 실행한 플레이어의 팀 배정 정보를 확인할 수 없습니다.");
		return false;
	}

	int32 TeamMemberCount = 0;
	for (APlayerState* BaseState : NFGameState->PlayerArray)
	{
		const ANF_PlayerState* Member = Cast<ANF_PlayerState>(BaseState);
		if (IsValid(Member) && Member->NF_TeamID == Requester->NF_TeamID)++TeamMemberCount;
	}
	if (TeamMemberCount != 2)
	{
		OutMessage = TEXT("실제 탈출 판정에는 같은 팀 2명이 필요합니다. PIE 인원과 InitialRequiredPlayers를 2명 이상으로 맞추세요.");
		return false;
	}

	APawn* Pawn = Requester->GetPawn();
	UAbilitySystemComponent* ASC = Requester->GetAbilitySystemComponent();
	if (!IsValid(Pawn) || !IsValid(ASC) || ASC->GetAvatarActor() != Pawn ||
		ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead) ||
		ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Downed))
	{
		OutMessage = TEXT("명령을 실행한 플레이어가 생존 상태이고 GAS Avatar 연결이 완료되어야 합니다.");
		return false;
	}

	if (!IsValid(EscapeKeyComponent) || !IsValid(PlayerRespawnComponent) || !IsValid(BuildReadyComponent))
	{
		OutMessage = TEXT("탈출 테스트에 필요한 Core 컴포넌트가 없습니다.");
		return false;
	}
	if (EscapeKeyComponent->GetInitState() != ENF_KeyInitState::NotStarted)
	{
		OutMessage = TEXT("이미 열쇠 초기 지급이 시작되었습니다. 새 PIE 세션에서 실행하세요.");
		return false;
	}
	if (!EscapeKeyComponent->ValidateInitialAssignmentSettings(OutMessage))return false;
	if (IsValid(BossEncounterComponent) && IsValid(BossEncounterComponent->GetActiveBoss()))
	{
		OutMessage = TEXT("보스가 활성화된 상태입니다. 보스 출현 전에 실행하세요.");
		return false;
	}

	// 모든 사전 검증 이후에만 기존 페이즈 진행을 중단한다.
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	BuildReadyComponent->CancelResolve();
	PlayerRespawnComponent->LockRespawnsAfterBossDefeat();
	EnterEscape();
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Escape || bMatchFinalized)
	{
		OutMessage = TEXT("Escape 전환에 실패했습니다.");
		return false;
	}

	// 보스 처치 이벤트와 보상은 만들지 않고, 기존 최초 열쇠 지급 절차만 재사용한다.
	FNF_BossDefeatResult TestResult;
	TestResult.LastHitPlayerState = Requester;
	TestResult.LastHitPlayerID = Requester->GetPlayerId();
	EscapeKeyComponent->BeginInitialAssignment(TestResult);
	if (bMatchFinalized || EscapeKeyComponent->GetInitState() == ENF_KeyInitState::Failed)
	{
		OutMessage = TEXT("열쇠 지급에 실패했습니다. Output Log의 EscapeKey 로그를 확인하세요.");
		return false;
	}
	if (!EscapeKeyComponent->IsKeyHolder(Requester))
	{
		OutMessage = TEXT("Escape로 전환했습니다. 열쇠 지급은 재시도 중입니다. NF_PrintMatchState로 확인하세요.");
		return true;
	}

	OutMessage = FString::Printf(TEXT("Escape 테스트 준비 완료. 열쇠 보유자 PlayerId=%d, TeamId=%d. 팀원과 탈출구로 이동하세요."),
		Requester->GetPlayerId(), Requester->NF_TeamID);
	return true;
#endif
}

void ANF_GameMode::EnterEscape()
{
	ANF_GameState* NFGameState = GetNFGameState();

	if (!NFGameState) return;

	NFGameState->AuthoritySetUntimedMatchPhase(ENF_MatchPhase::Escape);

	//부활 시스템에 부활 불가 상태 전달
}

bool ANF_GameMode::FinishMatch(const FNF_MatchResult& Result)
{
	if (!HasAuthority() || bMatchFinalized){return false;}

	ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState)){return false;}

	// 결과 변경 이벤트 내부의 재진입을 먼저 차단.
	bMatchFinalized = true;

	if (!NFGameState->AuthorityFinishMatch(Result, FMath::Max(0.1f, ResultDisplayDuration)))
	{
		bMatchFinalized = false;
		return false;
	}

	bMatchAborting = Result.Outcome == ENF_MatchOutcome::Aborted;

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

	// 결과는 GameState에 남기고 게임플레이 자원만 정리.
	NotifyMatchCleanup();

	GetWorldTimerManager().SetTimer(
		ResultDisplayTimerHandle,
		this,
		&ThisClass::DispatchMatchConclusion,
		FMath::Max(0.1f, ResultDisplayDuration),
		false);

	return true;
}

UNF_BossResolutionComponent* ANF_GameMode::GetBossResolutionComponent() const
{
	return BossResolutionComponent;
}

UNF_BossContributionRewardComponent* ANF_GameMode::GetBossContributionRewardComponent() const
{
	return BossContributionRewardComponent;
}

UNF_EscapeKeyComponent* ANF_GameMode::GetEscapeKeyComponent() const
{
	return EscapeKeyComponent;
}

void ANF_GameMode::AbortMatchDueToDepartures()
{
	AbortMatch(TEXT("참가자 이탈 한도에 도달하여 매치가 중단되었습니다."),ENF_MatchEndReason::DepartureLimit);
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

void ANF_GameMode::AbortMatch(const FString& Reason, ENF_MatchEndReason EndReason)
{
	FNF_MatchResult Result;
	Result.Outcome = ENF_MatchOutcome::Aborted;
	Result.EndReason = EndReason;
	Result.Message = Reason;

	if (FinishMatch(Result))
	{
		UE_LOG(LogTemp, Error, TEXT("%s"), *Reason);
	}
}

void ANF_GameMode::HandleServerTeamIdsAssigned(const TArray<FNFServerTeamIdAssignment>& Assignments)
{
	if (!HasAuthority() || bTeamsReady || bInitialTeamSpawnsStarted || bMatchFinalized)return;

	if (Assignments.Num() != InitialRequiredPlayers)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[GameMode] 팀 배정 인원 불일치. Expected=%d, Actual=%d"),
			InitialRequiredPlayers,
			Assignments.Num());

		return;
	}

	if (!IsValid(PlayerRespawnComponent))
	{
		AbortMatch(TEXT("최초 팀 스폰에 필요한 RespawnComponent가 없습니다."), ENF_MatchEndReason::RuntimeFailure);
		return;
	}

	// Pawn을 만들기 전에 전원의 배정과 서로 다른 팀 스폰 지점을 검증한다.
	TSet<ANF_PlayerState*> AssignedPlayers;
	for (const FNFServerTeamIdAssignment& Assignment : Assignments)
	{
		ANF_PlayerState* PlayerState = Assignment.PlayerState.Get();
		if (!IsValid(PlayerState) || AssignedPlayers.Contains(PlayerState) ||
			PlayerState->GetWorld() != GetWorld() || !IsValid(PlayerState->GetPlayerController()) ||
			Assignment.TeamId != PlayerState->NF_TeamID || Assignment.TeamId < 1 || Assignment.TeamId > 4)
		{
			AbortMatch(TEXT("최초 스폰을 위한 팀 배정 정보가 유효하지 않습니다."), ENF_MatchEndReason::RuntimeFailure);
			return;
		}

		AssignedPlayers.Add(PlayerState);
		if (!IsValid(PlayerRespawnComponent->FindTeamPlayerStart(PlayerState)))
		{
			AbortMatch(FString::Printf(
				TEXT("최초 팀 스폰 지점이 부족합니다. 팀원 수만큼 NF_TeamPlayerStart를 배치하세요. TeamId=%d"),
				Assignment.TeamId), ENF_MatchEndReason::RuntimeFailure);
			return;
		}
	}

	bInitialTeamSpawnsStarted = true;
	for (const FNFServerTeamIdAssignment& Assignment : Assignments)
	{
		PendingInitialSpawnPlayers.Add(Assignment.PlayerState);
	}

	for (const TWeakObjectPtr<ANF_PlayerState>& Entry : PendingInitialSpawnPlayers)
	{
		ANF_PlayerState* PlayerState = Entry.Get();
		if (!IsValid(PlayerState->GetPawn()))
		{
			RestartPlayer(PlayerState->GetPlayerController());
		}

		if (bMatchFinalized)return;
	}

	// 배정은 한 번만 수행하고, GAS 준비만 별도로 재확인한다.
	GetWorldTimerManager().SetTimer(InitialSpawnReadyTimerHandle, this,
		&ThisClass::TryCompleteInitialTeamSpawns, 0.2f, true);
	TryCompleteInitialTeamSpawns();
}

void ANF_GameMode::TryCompleteInitialTeamSpawns()
{
	if (!HasAuthority() || bMatchFinalized || bMatchCleanupNotified || bTeamsReady)
	{
		GetWorldTimerManager().ClearTimer(InitialSpawnReadyTimerHandle);
		return;
	}

	const ANF_GameState* NFGameState = GetNFGameState();
	if (!IsValid(NFGameState) || NFGameState->GetMatchPhase() != ENF_MatchPhase::Waiting)return;
	if (NFGameState->GetRemainingPhaseTime() <= 0.0)return;

	TArray<UNF_PlayerLifeComponent*> LifeComponents;
	LifeComponents.Reserve(PendingInitialSpawnPlayers.Num());
	for (const TWeakObjectPtr<ANF_PlayerState>& Entry : PendingInitialSpawnPlayers)
	{
		ANF_PlayerState* PlayerState = Entry.Get();
		if (!IsValid(PlayerState) || !IsValid(PlayerState->GetPlayerController()))
		{
			AbortMatch(TEXT("최초 팀 스폰 준비 중 플레이어가 이탈했습니다."), ENF_MatchEndReason::RuntimeFailure);
			return;
		}

		APawn* PlayerPawn = PlayerState->GetPawn();
		if (!IsValid(PlayerPawn) || PlayerPawn->GetPlayerState<ANF_PlayerState>() != PlayerState)return;

		UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
		if (!IsValid(ASC) || ASC->GetAvatarActor() != PlayerPawn)return;

		UNF_PlayerLifeComponent* LifeComponent =
			PlayerPawn->FindComponentByClass<UNF_PlayerLifeComponent>();

		if (!IsValid(LifeComponent))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT(
					"[GameMode] 팀 배정 완료 처리 실패: LifeComponent가 없습니다. "
					"PlayerId=%d, Pawn=%s"),
				PlayerState->GetPlayerId(),
				*GetNameSafe(PlayerPawn));

			return;
		}

		if (LifeComponents.Contains(LifeComponent))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT(
					"[GameMode] 팀 배정 완료 처리 실패: "
					"동일한 LifeComponent가 중복되었습니다. Pawn=%s"),
				*GetNameSafe(PlayerPawn));

			return;
		}

		LifeComponents.Add(LifeComponent);
	}

	// 팀 배정이 완료된 현재 Pawn의 서버 생존 상태 변경을 구독한다.
	if (IsValid(PlayerRespawnComponent))
	{
		PlayerRespawnComponent->BindAssignedLifeComponents(LifeComponents);
	}

	// 1단계: 모든 LifeComponent의 팀 배정 확인 상태를 먼저 완료한다.
	for (UNF_PlayerLifeComponent* LifeComponent : LifeComponents)
	{
		if (!IsValid(LifeComponent))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[GameMode] 팀 배정 확인 도중 LifeComponent가 유효하지 않게 됐습니다."));

			return;
		}

		if (!LifeComponent->ConfirmTeamAssignment())
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT(
					"[GameMode] LifeComponent 팀 배정 확인에 실패했습니다. "
					"Owner=%s"),
				*GetNameSafe(LifeComponent->GetOwner()));

			return;
		}
	}

	// 2단계: 전원의 확인이 끝난 후 보류된 Health 0 판정을 다시 실행한다.
	for (UNF_PlayerLifeComponent* LifeComponent : LifeComponents)
	{
		if (!IsValid(LifeComponent))continue;

		// 보류 중인 판정이 없는 경우도 false를 반환하므로 반환값은 실패로 처리하지 않는다.
		LifeComponent->RetryPendingHealthZeroCheck();
	}

	GetWorldTimerManager().ClearTimer(InitialSpawnReadyTimerHandle);
	PendingInitialSpawnPlayers.Empty();
	NotifyTeamsReady();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[GameMode] 전체 플레이어의 팀 배정과 LifeComponent 확인을 완료했습니다. Count=%d"),
		LifeComponents.Num());
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
		AbortMatch(TEXT("보스 출현 컴포넌트가 없습니다."), ENF_MatchEndReason::RuntimeFailure);
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
		AbortMatch(TEXT("보스 출현 결과를 확인할 수 없습니다."), ENF_MatchEndReason::RuntimeFailure);
		return;
	}

	// Boss 페이즈 변경을 통해 ViewModel에서 출현 알림을 생성한다.
	EnterBoss();
}

void ANF_GameMode::HandleBossSpawnFailed(const FString& Reason)
{
	const ANF_GameState* NFGameState = GetNFGameState();

	if (!HasAuthority() ||bMatchAborting ||!IsValid(NFGameState))return;

	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Farming)return;

	AbortMatch(Reason, ENF_MatchEndReason::RuntimeFailure);
}

void ANF_GameMode::HandleBossDefeatResolved(const FNF_BossDefeatResult& DefeatResult)
{

	if (!HasAuthority() || bMatchAborting || bMatchFinalized)return;

	const ANF_GameState* NFGameState =GetNFGameState();
	
	if (!IsValid(NFGameState))return;
	if (NFGameState->GetMatchPhase()!= ENF_MatchPhase::Boss)return;

	UE_LOG(LogTemp,Log,TEXT("[GameMode] 보스 처치 확정. ""PlayerID=%d"),DefeatResult.LastHitPlayerID);
	if (ANF_GameState* AnnouncementGameState = GetNFGameState())
	{
		AnnouncementGameState->MulticastShowAnnouncement(
			NSLOCTEXT("NFGameFlow", "BossDefeated", "보스를 처치했습니다! 이제 부활과 소생이 불가능합니다."),
			3.0f);
	}

	// 보스 처치가 확정된 시점부터 예약과 실제 리스폰을 모두 금지한다.
	if (IsValid(PlayerRespawnComponent))
	{
		PlayerRespawnComponent->LockRespawnsAfterBossDefeat();
	}
	
	// 페이즈 변경 콜백이 실행되기 전에 처치 순간의 참가 팀과 피해량을 확정한다.
	if (IsValid(BossContributionRewardComponent))
	{
		if (!BossContributionRewardComponent->PrepareTeamPenalty(DefeatResult))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] 팀 페널티 스냅샷 생성에 실패했습니다."));
		}
	}

	// 먼저 Escape로 전환하여 부활 및 소생 제한을 매치 정책에 반영
	EnterEscape();

	NFGameState = GetNFGameState();

	if (!IsValid(NFGameState))return;

	if (NFGameState->GetMatchPhase()!= ENF_MatchPhase::Escape)
	{
		UE_LOG(LogTemp,Error,TEXT("[GameMode] Escape 페이즈 전환에 ""실패하여 기여도 보상을 적용하지 않습니다."));
		return;
	}

	// 기여도 회복 보상
	if (IsValid(BossContributionRewardComponent))
	{
		if (!BossContributionRewardComponent->ApplyRewards(
			DefeatResult))
		{
			UE_LOG(LogTemp,Error,TEXT("[GameMode] 보스 기여도 보상 적용에 실패했습니다."));
		}
	}
	else
	{
		UE_LOG(LogTemp,Error,TEXT("[GameMode] 보스 기여도 보상 컴포넌트가 없습니다."));
	}

	if (bMatchFinalized)return;

	// 회복 보상의 성공 여부와 독립적으로 팀 페널티를 적용한다.
	if (IsValid(BossContributionRewardComponent))
	{
		if (!BossContributionRewardComponent->ApplyTeamPenalty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] 팀 페널티 적용이 완료되지 않았습니다."));
		}
	}

	if (bMatchFinalized)return;

	if (!IsValid(EscapeKeyComponent))return;

	EscapeKeyComponent->BeginInitialAssignment(DefeatResult);
}

bool ANF_GameMode::RequestEscape(ANF_EscapeZone* Zone, ANF_PlayerState* Requester)
{
	if (!HasAuthority() || bMatchFinalized || bMatchAborting || bMatchCleanupNotified || !IsValid(EscapeComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] RequestRejected Authority=%d Finalized=%d Aborting=%d Cleanup=%d Component=%s"),
			HasAuthority(), bMatchFinalized, bMatchAborting, bMatchCleanupNotified, *GetNameSafe(EscapeComponent.Get()));
		return false;
	}
	return EscapeComponent->TryStartEscape(Zone, Requester);
}

void ANF_GameMode::NotifyEscapeRangeExited(ANF_EscapeZone* Zone, ANF_PlayerState* PlayerState)
{
	if (!HasAuthority() || !IsValid(EscapeComponent)) return;
	EscapeComponent->HandleRangeExited(Zone, PlayerState);
}

FNF_EscapeValidationResult ANF_GameMode::EvaluateEscapeRequest(ANF_EscapeZone* Zone, ANF_PlayerState* Requester)
{
	FNF_EscapeValidationResult Result;

	if (!HasAuthority() || bMatchFinalized || bMatchAborting || bMatchCleanupNotified)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: AuthorityOrMatchEnding"));
		return Result;
	}
	if (!IsValid(Zone) || Zone->GetWorld() != GetWorld() || !IsValid(Requester) || Requester->GetWorld() != GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: InvalidZoneOrRequesterWorld"));
		return Result;
	}

	const ANF_GameState* NFGameState = GetNFGameState();
	if (!IsValid(NFGameState))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: MissingGameState"));
		return Result;
	}
	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Escape)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: NotEscapePhase Phase=%d"), static_cast<int32>(NFGameState->GetMatchPhase()));
		return Result;
	}
	if (!IsValid(EscapeKeyComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: MissingKeyComponent"));
		return Result;
	}

	ANF_PlayerState* Holder = EscapeKeyComponent->GetKeyHolder();
	if (!IsValid(Holder) || Holder->GetWorld() != GetWorld() || !NFGameState->PlayerArray.Contains(Holder))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: InvalidKeyHolder Holder=%s"), *GetNameSafe(Holder));
		return Result;
	}

	// 탈출 상호작용은 같은 팀원이 대신할 수 없으며 열쇠 보유자가 직접 요청해야 한다.
	if (Requester != Holder || Holder->NF_TeamID <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: RequesterNotHolderOrInvalidTeam Requester=%s Holder=%s TeamId=%d"),
			*GetNameSafe(Requester), *GetNameSafe(Holder), Holder->NF_TeamID);
		return Result;
	}

	TArray<ANF_PlayerState*> TeamMembers;
	for (APlayerState* BasePlayerState : NFGameState->PlayerArray)
	{
		ANF_PlayerState* Member = Cast<ANF_PlayerState>(BasePlayerState);
		if (!IsValid(Member) || Member->NF_TeamID != Holder->NF_TeamID) continue;

		TeamMembers.Add(Member);
	}

	// 현재 매치 규칙은 2인 1팀이다.
	if (TeamMembers.Num() != 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: TeamMemberCount Count=%d TeamId=%d"), TeamMembers.Num(), Holder->NF_TeamID);
		return Result;
	}

	ANF_PlayerState* Teammate = nullptr;
	for (ANF_PlayerState* Member : TeamMembers)
	{
		if (Member != Holder)
		{
			Teammate = Member;
			break;
		}
	}

	if (!IsValid(Teammate))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: MissingTeammate"));
		return Result;
	}

	Result.Holder = Holder;
	Result.Teammate = Teammate;

	// 열쇠 보유자는 탈출 진행 내내 생존, 행동 가능, 범위 안 상태여야 한다.
	UAbilitySystemComponent* HolderASC = Holder->GetAbilitySystemComponent();
	if (!IsValid(HolderASC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: MissingHolderASC"));
		return FNF_EscapeValidationResult();
	}
	if (HolderASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: HolderDead"));
		return FNF_EscapeValidationResult();
	}
	if (HolderASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Downed))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: HolderDowned"));
		return FNF_EscapeValidationResult();
	}

	APawn* HolderPawn = Holder->GetPawn();
	if (!IsValid(HolderPawn) || HolderASC->GetAvatarActor() != HolderPawn || !Zone->ContainsPawn(HolderPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: HolderPawnAvatarOrRange Pawn=%s Avatar=%s InRange=%d"),
			*GetNameSafe(HolderPawn), *GetNameSafe(HolderASC->GetAvatarActor()), Zone->ContainsPawn(HolderPawn));
		return FNF_EscapeValidationResult();
	}

	UAbilitySystemComponent* TeammateASC = Teammate->GetAbilitySystemComponent();
	if (!IsValid(TeammateASC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: MissingTeammateASC"));
		return FNF_EscapeValidationResult();
	}

	// 팀원이 사망했다면 Pawn과 범위 상태에 관계없이 20초 단독 탈출 규칙을 적용한다.
	if (TeammateASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead))
	{
		Result.State = ENF_EscapeValidationState::SoloReady;
		return Result;
	}

	// 다운은 사망으로 보지 않으며, 회복하거나 사망할 때까지 진행을 멈춘다.
	if (TeammateASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Downed))
	{
		Result.State = ENF_EscapeValidationState::WaitingForTeammate;
		return Result;
	}

	APawn* TeammatePawn = Teammate->GetPawn();
	if (!IsValid(TeammatePawn) || TeammateASC->GetAvatarActor() != TeammatePawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Escape] ValidationRejected: TeammatePawnOrAvatar Pawn=%s Avatar=%s"),
			*GetNameSafe(TeammatePawn), *GetNameSafe(TeammateASC->GetAvatarActor()));
		return FNF_EscapeValidationResult();
	}

	if (!Zone->ContainsPawn(TeammatePawn))
	{
		Result.State = ENF_EscapeValidationState::WaitingForTeammate;
		return Result;
	}

	Result.State = ENF_EscapeValidationState::TeamReady;
	return Result;
}

bool ANF_GameMode::CanPlayerUseEscape(const ANF_PlayerState* PlayerState) const
{
	if (!HasAuthority()) {return false;}

	const ANF_GameState* NFGameState =GetNFGameState();
	if (!IsValid(NFGameState)) {return false;}

	if (NFGameState->GetMatchPhase() !=ENF_MatchPhase::Escape) {return false;}

	if (!IsValid(EscapeKeyComponent)) {return false;}

	return EscapeKeyComponent->IsKeyHolder(PlayerState);
}

void ANF_GameMode::NotifyMatchCleanup()
{
	if (!HasAuthority() || bMatchCleanupNotified){return;}

	bMatchCleanupNotified = true;
	GetWorldTimerManager().ClearTimer(InitialSpawnReadyTimerHandle);
	PendingInitialSpawnPlayers.Empty();

	OnMatchCleanup.Broadcast();
}

void ANF_GameMode::HandleInitialKeyAssignmentFailed(const FString& Reason)
{
	if (!HasAuthority() || bMatchFinalized)return;

	AbortMatch(Reason, ENF_MatchEndReason::RuntimeFailure);
}

void ANF_GameMode::HandleKeyOperationFailed(const FString& Reason)
{
	if (!HasAuthority() || bMatchFinalized)return;

	AbortMatch(Reason, ENF_MatchEndReason::RuntimeFailure);
}

void ANF_GameMode::HandleKeyStateChanged(const FNF_KeyPublicState& NewState)
{
	if (!HasAuthority()) { return; }

	if (IsValid(EscapeComponent))
	{
		EscapeComponent->CancelEscape();
	}

	if (ANF_GameState* NFGameState = GetNFGameState())
	{
		if (!NFGameState->AuthoritySetKeyPublicState(NewState))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameMode] 유효하지 않은 열쇠 공개 상태를 거부했습니다."));
		}
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) { return; }

	// 공개 상태 이벤트 중 재진입할 수 있으므로 미니맵은 현재 상태를 다시 읽는다.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ANF_PlayerController* PlayerController = Cast<ANF_PlayerController>(It->Get()))
		{
			SyncKeyMinimapForPlayer(PlayerController);
		}
	}
}

void ANF_GameMode::SyncKeyMinimapForPlayer(ANF_PlayerController* PlayerController)
{
	if (!HasAuthority() || !IsValid(PlayerController) || !IsValid(EscapeKeyComponent) || !KeyRevealSourceId.IsValid()) { return; }

	UNF_MinimapRevealRegistry* Registry = PlayerController->GetMinimapRevealRegistry();
	if (!IsValid(Registry)) { return; }

	// 이 매치의 열쇠 공개만 제거하며 천리안 등 다른 공개 출처는 유지한다.
	Registry->RemoveRevealSource(KeyRevealSourceId);

	const FNF_KeyPublicState CurrentState = EscapeKeyComponent->GetPublicState();
	if (CurrentState.State != ENF_KeyState::Held || !IsValid(CurrentState.Holder)) { return; }

	APawn* HolderPawn = CurrentState.Holder->GetPawn();
	if (!IsValid(HolderPawn)) { return; }

	Registry->RegisterReveal(KeyRevealSourceId, HolderPawn, ENF_MinimapRevealKind::EscapeKey, 0.0f);
}

void ANF_GameMode::StartWaiting()
{
	if (!HasAuthority() || bWaitingStarted || bMatchFinalized)return;

	ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState))
	{
		UE_LOG(LogTemp, Error, TEXT("Waiting 진입 실패: GameState 없음"));
		return;
	}

	const float Duration = FMath::Max(1.0f, WaitingDuration);

	if (!NFGameState->AuthoritySetTimedMatchPhase(ENF_MatchPhase::Waiting,Duration))return;

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ThisClass::HandleWaitingPhaseExpired,
		Duration,
		false);

	bWaitingStarted = true;

	// BeginPlay 이전에 접속·팀 배정 통보가 완료된 경우도 반영.
	TryStartBuild();
}

void ANF_GameMode::HandleWaitingPhaseExpired()
{
	if (!HasAuthority() || bMatchFinalized)return;

	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState) ||NFGameState->GetMatchPhase() != ENF_MatchPhase::Waiting)return;

	if (GetNumPlayers() != InitialRequiredPlayers)
	{
		AbortMatch(TEXT("제한시간 안에 필요한 참가자가 접속하지 않았습니다."),ENF_MatchEndReason::WaitingPlayersTimeout);
		return;
	}

	if (!bTeamsReady)
	{
		AbortMatch(TEXT("제한시간 안에 팀 배정이 완료되지 않았습니다."),ENF_MatchEndReason::WaitingTeamsTimeout);
		return;
	}

	// 인원과 팀은 준비됐지만 참가자 수집 등의 문제로 Build에 진입하지 못한 경우.
	AbortMatch(TEXT("제한시간 안에 매치 시작 초기화가 완료되지 않았습니다."),ENF_MatchEndReason::WaitingInitializationTimeout);
}

void ANF_GameMode::DispatchMatchConclusion()
{
	if (!HasAuthority() ||!bMatchFinalized ||bConclusionDispatched)return;

	const ANF_GameState* NFGameState = GetNFGameState();

	if (!IsValid(NFGameState))return;

	// 외부 콜백이 World를 변경할 수 있으므로 값 복사.
	const FNF_MatchResult Result =NFGameState->GetMatchState().Result;

	if (Result.Outcome == ENF_MatchOutcome::Aborted)
	{
		if (!RequestMatchAbort.IsBound())return;

		bConclusionDispatched = true;
		RequestMatchAbort.Execute(Result);
	}
	else if (Result.Outcome == ENF_MatchOutcome::Victory)
	{
		if (!RequestMatchCompleted.IsBound())return;
		
		bConclusionDispatched = true;
		RequestMatchCompleted.Execute(Result);
	}
}
