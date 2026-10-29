#include "Core/NF_TestGameMode.h"

#include "AbilitySystemComponent.h"
#include "Core/Component/NF_EscapeKeyComponent.h"
#include "Core/Match/NF_DedicatedMatchSubsystem.h"
#include "Core/NF_GameState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/State/NF_PlayerState.h"
#include "TimerManager.h"

ANF_TestGameMode::ANF_TestGameMode()
{
	InitialRequiredPlayers = 4;
}

bool ANF_TestGameMode::ShouldUseDedicatedMatchSubsystem() const
{
	return false;
}

bool ANF_TestGameMode::RequestEscapeTest(ANF_PlayerState* Requester, FString& OutMessage)
{
	return TryStartEscapeTest(Requester, OutMessage);
}

FString ANF_TestGameMode::BuildMatchDebugState() const
{
#if UE_BUILD_SHIPPING || UE_BUILD_TEST
	return TEXT("이 빌드에서는 테스트 치트를 사용할 수 없습니다.");
#else
	if (!HasAuthority() || GetWorld()->WorldType != EWorldType::PIE)
	{
		return TEXT("PIE 테스트 게임모드의 서버에서만 상태를 조회할 수 있습니다.");
	}

	const ANF_GameState* NFGameState = GetGameState<ANF_GameState>();
	if (!IsValid(NFGameState))return TEXT("NF_GameState가 없습니다.");

	const FNF_KeyPublicState KeyState = NFGameState->GetKeyPublicState();
	int32 HolderId = INDEX_NONE;
	if (IsValid(KeyState.Holder))HolderId = KeyState.Holder->GetPlayerId();
	FString Result = FString::Printf(TEXT("Phase=%s, TeamsReady=%d, Players=%d/%d, Remaining=%.1f\nKeyState=%s, HolderPlayerId=%d"),
		*UEnum::GetValueAsString(NFGameState->GetMatchPhase()), AreTeamsReady(),
		NumPlayers, InitialRequiredPlayers, NFGameState->GetRemainingPhaseTime(),
		*UEnum::GetValueAsString(KeyState.State), HolderId);

	if (const UNF_EscapeKeyComponent* KeyComponent = FindComponentByClass<UNF_EscapeKeyComponent>())
	{
		Result += FString::Printf(TEXT(", KeyInit=%s"), *UEnum::GetValueAsString(KeyComponent->GetInitState()));
	}
	for (APlayerState* BaseState : NFGameState->PlayerArray)
	{
		const ANF_PlayerState* State = Cast<ANF_PlayerState>(BaseState);
		if (!IsValid(State))continue;
		const APawn* Pawn = State->GetPawn();
		const UAbilitySystemComponent* ASC = State->GetAbilitySystemComponent();
		const bool bGASReady = IsValid(Pawn) && IsValid(ASC) && ASC->GetAvatarActor() == Pawn;
		Result += FString::Printf(TEXT("\nPlayerId=%d, TeamId=%d, Pawn=%s, GASReady=%d"),
			State->GetPlayerId(), State->NF_TeamID, *GetNameSafe(Pawn), bGASReady);
	}
	return Result;
#endif
}

void ANF_TestGameMode::BeginPlay()
{
	// 부모의 Waiting 초기화 중 정리가 발생해도 재시도 타이머를 시작하지 않는다.
	OnMatchCleanup.AddUObject(this, &ThisClass::StopTeamAssignment);

	Super::BeginPlay();

	if (!HasAuthority() || bTeamAssignmentStopped)
	{
		return;
	}

	// 현재 프로젝트는 팀 ID 1~4를 사용한다. 테스트에서도 팀당 최대 2명이다.
	if (InitialRequiredPlayers < 1 || InitialRequiredPlayers > 8)
	{
		UE_LOG(LogTemp, Error, TEXT("[TestGameMode] 테스트 인원은 1~8이어야 합니다. Current=%d"), InitialRequiredPlayers);
		StopTeamAssignment();
		AbortMatch(TEXT("테스트 게임모드의 인원 설정이 잘못되었습니다."));
		return;
	}

	GetWorldTimerManager().SetTimer(
		TeamAssignmentTimerHandle,
		this,
		&ThisClass::TryAssignTeams,
		0.2f,
		true);

	TryAssignTeams();
}

void ANF_TestGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!HasAuthority() || bTeamAssignmentStopped || AreTeamsReady() || !IsValid(NewPlayer))
	{
		return;
	}

	ANF_PlayerState* PlayerState = NewPlayer->GetPlayerState<ANF_PlayerState>();
	if (IsValid(PlayerState))
	{
		LoginOrder.AddUnique(TWeakObjectPtr<ANF_PlayerState>(PlayerState));
	}

	// 접속 인원과 PlayerState가 모이면 Pawn 생성 전에 팀을 배정한다.
}

void ANF_TestGameMode::Logout(AController* Exiting)
{
	if (IsValid(Exiting))
	{
		LoginOrder.Remove(TWeakObjectPtr<ANF_PlayerState>(
			Exiting->GetPlayerState<ANF_PlayerState>()));
	}

	Super::Logout(Exiting);
}

void ANF_TestGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTeamAssignment();
	OnMatchCleanup.RemoveAll(this);
	LoginOrder.Reset();

	Super::EndPlay(EndPlayReason);
}

void ANF_TestGameMode::StopTeamAssignment()
{
	bTeamAssignmentStopped = true;
	GetWorldTimerManager().ClearTimer(TeamAssignmentTimerHandle);
}

void ANF_TestGameMode::TryAssignTeams()
{
	if (!HasAuthority() || bTeamAssignmentStopped || AreTeamsReady())
	{
		StopTeamAssignment();
		return;
	}

	const ANF_GameState* NFGameState = GetGameState<ANF_GameState>();
	if (!IsValid(NFGameState))
	{
		return;
	}

	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Waiting)
	{
		StopTeamAssignment();
		return;
	}

	// 마감 이후에는 배정하지 않는다. 타임아웃 처리는 부모의 Waiting 타이머가 담당한다.
	if (NFGameState->GetRemainingPhaseTime() <= 0.0)
	{
		return;
	}

	LoginOrder.RemoveAll([](const TWeakObjectPtr<ANF_PlayerState>& Entry)
	{
		return !Entry.IsValid();
	});

	if (GetNumPlayers() != InitialRequiredPlayers || LoginOrder.Num() != InitialRequiredPlayers)
	{
		return;
	}

	// Pawn과 GAS Avatar는 팀 배정 이후 부모 GameMode에서 생성 및 확인한다.
	for (const TWeakObjectPtr<ANF_PlayerState>& Entry : LoginOrder)
	{
		ANF_PlayerState* PlayerState = Entry.Get();
		if (!IsValid(PlayerState) || !IsValid(PlayerState->GetPlayerController()))
		{
			return;
		}
	}

	constexpr int32 PlayersPerTeam = 2;
	TArray<FNFServerTeamIdAssignment> Assignments;
	Assignments.Reserve(LoginOrder.Num());

	for (int32 Index = 0; Index < LoginOrder.Num(); ++Index)
	{
		ANF_PlayerState* PlayerState = LoginOrder[Index].Get();
		const int32 TeamId = (Index / PlayersPerTeam) + 1;

		if (!IsValid(PlayerState))
		{
			return;
		}

		if (PlayerState->NF_TeamID != TeamId)
		{
			if (!PlayerState->SetNFTeamID(TeamId))
			{
				StopTeamAssignment();
				AbortMatch(TEXT("테스트 팀 ID 적용에 실패했습니다."));
				return;
			}
		}

		FNFServerTeamIdAssignment& Assignment = Assignments.AddDefaulted_GetRef();
		Assignment.PlayerState = PlayerState;
		Assignment.TeamId = TeamId;
	}

	// 팀 배정은 한 번만 제출한다. 이후 스폰 및 GAS 준비 재확인은 부모가 담당한다.
	HandleServerTeamIdsAssigned(Assignments);
	UE_LOG(LogTemp, Log, TEXT("[TestGameMode] 접속 순서 기준 팀 배정 제출. Players=%d, PlayersPerTeam=%d"), Assignments.Num(), PlayersPerTeam);
	StopTeamAssignment();
}
