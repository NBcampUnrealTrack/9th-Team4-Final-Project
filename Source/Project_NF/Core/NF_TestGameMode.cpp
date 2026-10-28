#include "Core/NF_TestGameMode.h"

#include "AbilitySystemComponent.h"
#include "Core/Match/NF_DedicatedMatchSubsystem.h"
#include "Core/NF_GameState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
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

	// PostLogin 직후에는 Pawn이 없을 수 있으므로 타이머에서 준비 여부를 확인한다.
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

	// 한 명이라도 초기 스폰이나 GAS 연결이 끝나지 않았으면 다음 호출에서 재확인한다.
	for (const TWeakObjectPtr<ANF_PlayerState>& Entry : LoginOrder)
	{
		ANF_PlayerState* PlayerState = Entry.Get();
		if (!IsValid(PlayerState))
		{
			return;
		}

		APawn* PlayerPawn = PlayerState->GetPawn();
		if (!IsValid(PlayerPawn) || PlayerPawn->GetPlayerState<ANF_PlayerState>() != PlayerState)
		{
			return;
		}

		UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
		if (!IsValid(ASC) || ASC->GetAvatarActor() != PlayerPawn)
		{
			return;
		}

		if (!IsValid(PlayerPawn->FindComponentByClass<UNF_PlayerLifeComponent>()))
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

	// 기존 LifeComponent 확인 및 NotifyTeamsReady -> Build 진입 경로를 재사용한다.
	HandleServerTeamIdsAssigned(Assignments);

	if (AreTeamsReady())
	{
		UE_LOG(LogTemp, Log, TEXT("[TestGameMode] 접속 순서 기준 팀 배정 완료. Players=%d, PlayersPerTeam=%d"), Assignments.Num(), PlayersPerTeam);
		StopTeamAssignment();
	}
}
