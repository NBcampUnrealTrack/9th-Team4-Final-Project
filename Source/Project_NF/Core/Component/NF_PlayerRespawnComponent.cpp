#include "Core/Component/NF_PlayerRespawnComponent.h"

#include "Core/NF_GameMode.h"
#include "Core/NF_GameState.h"
#include "Core/Spawn/NF_TeamPlayerStart.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
#include "Player/State/NF_PlayerState.h"
#include "TimerManager.h"

UNF_PlayerRespawnComponent::UNF_PlayerRespawnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void UNF_PlayerRespawnComponent::RegisterPlayer(
	ANF_PlayerState* PlayerState)
{
	if (!HasServerAuthority() || bStopped || !IsValid(PlayerState))return;

	ENF_PlayerPawnState InitialPawnState =
		ENF_PlayerPawnState::InitialSpawnPending;

	APawn* ExistingPawn = PlayerState->GetPawn();

	if (IsValid(ExistingPawn))
	{
		InitialPawnState = ENF_PlayerPawnState::Active;

		UNF_PlayerLifeComponent* LifeComponent =
			ExistingPawn->FindComponentByClass<UNF_PlayerLifeComponent>();

		if (IsValid(LifeComponent))
		{
			BindPlayerLifeComponent(LifeComponent);
		}
	}

	SetPlayerPawnState(PlayerState, InitialPawnState);
}

void UNF_PlayerRespawnComponent::HandlePlayerRestarted(
	AController* Controller)
{
	if (!HasServerAuthority() || bStopped || !IsValid(Controller))return;

	ANF_PlayerState* PlayerState =
		Controller->GetPlayerState<ANF_PlayerState>();
	APawn* NewPawn = Controller->GetPawn();

	if (!IsValid(PlayerState) || !IsValid(NewPawn))return;

	SetPlayerPawnState(PlayerState, ENF_PlayerPawnState::Active);

	UNF_PlayerLifeComponent* LifeComponent =
		NewPawn->FindComponentByClass<UNF_PlayerLifeComponent>();

	if (!IsValid(LifeComponent))return;

	BindPlayerLifeComponent(LifeComponent);

	if (bTeamsReady)
	{
		LifeComponent->ConfirmTeamAssignment();
		RetryTeamPendingHealthZeroChecks(PlayerState->NF_TeamID);
	}
}

void UNF_PlayerRespawnComponent::HandlePlayerLeaving(
	ANF_PlayerState* PlayerState)
{
	if (!HasServerAuthority() || !IsValid(PlayerState))return;

	CancelPlayerRespawn(PlayerState, false);
	UnbindPlayerLifeComponent(PlayerState);
	RemovePlayerPawnState(PlayerState);
	AssignedPlayerStarts.Remove(TWeakObjectPtr<ANF_PlayerState>(PlayerState));
}

void UNF_PlayerRespawnComponent::BindAssignedLifeComponents(
	const TArray<UNF_PlayerLifeComponent*>& LifeComponents)
{
	if (!HasServerAuthority() || bStopped)return;

	for (UNF_PlayerLifeComponent* LifeComponent : LifeComponents)
	{
		BindPlayerLifeComponent(LifeComponent);
	}
}

void UNF_PlayerRespawnComponent::NotifyTeamsReady()
{
	if (!HasServerAuthority() || bStopped)return;

	bTeamsReady = true;
}

void UNF_PlayerRespawnComponent::LockRespawnsAfterBossDefeat()
{
	if (!HasServerAuthority() || bStopped)return;

	bRespawnLockedByBossDefeat = true;
	CancelAllPendingRespawns(true);
}

void UNF_PlayerRespawnComponent::Shutdown()
{
	if (bStopped)return;

	bStopped = true;
	CancelAllPendingRespawns(false);

	for (TPair<
		TWeakObjectPtr<ANF_PlayerState>,
		TWeakObjectPtr<UNF_PlayerLifeComponent>>& Pair : BoundLifeComponents)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->OnLifeStateChanged.RemoveAll(this);
		}
	}

	BoundLifeComponents.Empty();
	PlayerPawnStates.Empty();
	AssignedPlayerStarts.Empty();
	EvaluateRespawnPermission.Unbind();
}

ENF_PlayerPawnState UNF_PlayerRespawnComponent::GetPlayerPawnState(
	const ANF_PlayerState* PlayerState) const
{
	if (!HasServerAuthority() || !IsValid(PlayerState))
	{
		return ENF_PlayerPawnState::Unknown;
	}

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(
		const_cast<ANF_PlayerState*>(PlayerState));

	const ENF_PlayerPawnState* FoundState =
		PlayerPawnStates.Find(PlayerStateKey);

	if (FoundState == nullptr)
	{
		return ENF_PlayerPawnState::Unknown;
	}

	if (*FoundState == ENF_PlayerPawnState::Active &&
		!IsValid(PlayerState->GetPawn()))
	{
		return ENF_PlayerPawnState::RespawnPending;
	}

	return *FoundState;
}

void UNF_PlayerRespawnComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	Shutdown();
	Super::EndPlay(EndPlayReason);
}

bool UNF_PlayerRespawnComponent::HasServerAuthority() const
{
	const AActor* Owner = GetOwner();
	return IsValid(Owner) && Owner->HasAuthority();
}

ANF_GameMode* UNF_PlayerRespawnComponent::GetNFGameMode() const
{
	return Cast<ANF_GameMode>(GetOwner());
}

bool UNF_PlayerRespawnComponent::SetPlayerPawnState(
	ANF_PlayerState* PlayerState,
	const ENF_PlayerPawnState NewState)
{
	if (!HasServerAuthority() || !IsValid(PlayerState))return false;
	if (PlayerState->GetWorld() != GetWorld())return false;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(PlayerState);
	PlayerPawnStates.FindOrAdd(PlayerStateKey) = NewState;
	return true;
}

void UNF_PlayerRespawnComponent::RemovePlayerPawnState(
	const ANF_PlayerState* PlayerState)
{
	if (!HasServerAuthority() || !IsValid(PlayerState))return;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(
		const_cast<ANF_PlayerState*>(PlayerState));

	PlayerPawnStates.Remove(PlayerStateKey);
}

void UNF_PlayerRespawnComponent::BindPlayerLifeComponent(
	UNF_PlayerLifeComponent* LifeComponent)
{
	if (!HasServerAuthority() || bStopped || !IsValid(LifeComponent))return;

	APawn* OwnerPawn = Cast<APawn>(LifeComponent->GetOwner());

	if (!IsValid(OwnerPawn))return;

	ANF_PlayerState* PlayerState =
		OwnerPawn->GetPlayerState<ANF_PlayerState>();

	if (!IsValid(PlayerState))return;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(PlayerState);

	if (TWeakObjectPtr<UNF_PlayerLifeComponent>* ExistingComponent =
		BoundLifeComponents.Find(PlayerStateKey))
	{
		if (ExistingComponent->IsValid() &&
			ExistingComponent->Get() != LifeComponent)
		{
			ExistingComponent->Get()
				->OnLifeStateChanged.RemoveAll(this);
		}
	}

	LifeComponent->OnLifeStateChanged.RemoveAll(this);
	LifeComponent->OnLifeStateChanged.AddDynamic(
		this,
		&ThisClass::HandlePlayerLifeStateChanged);

	BoundLifeComponents.FindOrAdd(PlayerStateKey) = LifeComponent;
}

void UNF_PlayerRespawnComponent::UnbindPlayerLifeComponent(
	ANF_PlayerState* PlayerState)
{
	if (!IsValid(PlayerState))return;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(PlayerState);
	TWeakObjectPtr<UNF_PlayerLifeComponent>* FoundComponent =
		BoundLifeComponents.Find(PlayerStateKey);

	if (FoundComponent != nullptr && FoundComponent->IsValid())
	{
		FoundComponent->Get()->OnLifeStateChanged.RemoveAll(this);
	}

	BoundLifeComponents.Remove(PlayerStateKey);
}

void UNF_PlayerRespawnComponent::HandlePlayerLifeStateChanged(
	UNF_PlayerLifeComponent* SourceLifeComponent,
	const ENFPlayerLifeState NewState)
{
	if (!HasServerAuthority() ||
		bStopped ||
		!IsValid(SourceLifeComponent) ||
		NewState != ENFPlayerLifeState::Dead)
	{
		return;
	}

	APawn* DeadPawn = Cast<APawn>(SourceLifeComponent->GetOwner());

	if (!IsValid(DeadPawn))return;

	ANF_PlayerState* DeadPlayerState =
		DeadPawn->GetPlayerState<ANF_PlayerState>();

	if (!IsValid(DeadPlayerState))return;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(DeadPlayerState);
	const TWeakObjectPtr<UNF_PlayerLifeComponent>* BoundComponent =
		BoundLifeComponents.Find(PlayerStateKey);

	if (BoundComponent == nullptr ||
		!BoundComponent->IsValid() ||
		BoundComponent->Get() != SourceLifeComponent)
	{
		return;
	}

	SchedulePlayerRespawn(DeadPlayerState);
}

bool UNF_PlayerRespawnComponent::CanRespawnPlayer(
	const ANF_PlayerState* PlayerState) const
{
	if (!HasServerAuthority() ||
		bStopped ||
		bRespawnLockedByBossDefeat ||
		!IsValid(PlayerState) ||
		!EvaluateRespawnPermission.IsBound())
	{
		return false;
	}

	return EvaluateRespawnPermission.Execute(PlayerState);
}

void UNF_PlayerRespawnComponent::SchedulePlayerRespawn(
	ANF_PlayerState* PlayerState)
{
	if (!HasServerAuthority() || bStopped || !IsValid(PlayerState))return;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(PlayerState);

	if (RespawnTimerHandles.Contains(PlayerStateKey))return;

	if (!CanRespawnPlayer(PlayerState))
	{
		SetPlayerPawnState(PlayerState, ENF_PlayerPawnState::Eliminated);
		return;
	}

	SetPlayerPawnState(PlayerState, ENF_PlayerPawnState::RespawnPending);

	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(
		this,
		&ThisClass::ExecutePlayerRespawn,
		PlayerStateKey);

	FTimerHandle& TimerHandle =
		RespawnTimerHandles.FindOrAdd(PlayerStateKey);

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		RespawnDelegate,
		RespawnDelaySeconds,
		false);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[PlayerRespawn] 리스폰 예약. PlayerId=%d, Delay=%.1f"),
		PlayerState->GetPlayerId(),
		RespawnDelaySeconds);
}

void UNF_PlayerRespawnComponent::ExecutePlayerRespawn(
	TWeakObjectPtr<ANF_PlayerState> PlayerStateKey)
{
	RespawnTimerHandles.Remove(PlayerStateKey);

	ANF_PlayerState* PlayerState = PlayerStateKey.Get();

	if (!HasServerAuthority() || bStopped || !IsValid(PlayerState))return;

	if (GetPlayerPawnState(PlayerState) != ENF_PlayerPawnState::RespawnPending)
	{
		return;
	}

	if (!CanRespawnPlayer(PlayerState))
	{
		SetPlayerPawnState(PlayerState, ENF_PlayerPawnState::Eliminated);
		return;
	}

	APlayerController* PlayerController = PlayerState->GetPlayerController();
	ANF_GameMode* GameMode = GetNFGameMode();

	if (!IsValid(PlayerController) || !IsValid(GameMode))return;

	ANF_TeamPlayerStart* TeamPlayerStart =
		FindTeamPlayerStart(PlayerState);

	if (!IsValid(TeamPlayerStart))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT(
				"[PlayerRespawn] 팀 리스폰 지점이 없습니다. "
				"PlayerId=%d, TeamId=%d"),
			PlayerState->GetPlayerId(),
			PlayerState->NF_TeamID);
		return;
	}

	APawn* OldPawn = PlayerController->GetPawn();
	UnbindPlayerLifeComponent(PlayerState);

	if (IsValid(OldPawn))
	{
		PlayerController->UnPossess();
		OldPawn->Destroy();
	}

	GameMode->RestartPlayerAtPlayerStart(PlayerController, TeamPlayerStart);

	APawn* NewPawn = PlayerController->GetPawn();

	if (!IsValid(NewPawn))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[PlayerRespawn] Pawn 생성 실패. PlayerId=%d"),
			PlayerState->GetPlayerId());
		return;
	}

	UNF_PlayerLifeComponent* NewLifeComponent =
		NewPawn->FindComponentByClass<UNF_PlayerLifeComponent>();

	if (!IsValid(NewLifeComponent))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[PlayerRespawn] 새 Pawn에 LifeComponent가 없습니다. Pawn=%s"),
			*GetNameSafe(NewPawn));
		return;
	}

	SetPlayerPawnState(PlayerState, ENF_PlayerPawnState::Active);
	BindPlayerLifeComponent(NewLifeComponent);

	if (bTeamsReady)
	{
		NewLifeComponent->ConfirmTeamAssignment();
		RetryTeamPendingHealthZeroChecks(PlayerState->NF_TeamID);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT(
			"[PlayerRespawn] 리스폰 완료. "
			"PlayerId=%d, TeamId=%d, Pawn=%s"),
		PlayerState->GetPlayerId(),
		PlayerState->NF_TeamID,
		*GetNameSafe(NewPawn));
}

void UNF_PlayerRespawnComponent::CancelPlayerRespawn(
	ANF_PlayerState* PlayerState,
	const bool bMarkEliminated)
{
	if (!IsValid(PlayerState))return;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(PlayerState);

	if (FTimerHandle* TimerHandle = RespawnTimerHandles.Find(PlayerStateKey))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*TimerHandle);
		}
	}

	RespawnTimerHandles.Remove(PlayerStateKey);

	if (bMarkEliminated)
	{
		SetPlayerPawnState(PlayerState, ENF_PlayerPawnState::Eliminated);
	}
}

void UNF_PlayerRespawnComponent::CancelAllPendingRespawns(
	const bool bMarkEliminated)
{
	UWorld* World = GetWorld();

	for (TPair<TWeakObjectPtr<ANF_PlayerState>, FTimerHandle>& Pair :
		RespawnTimerHandles)
	{
		if (IsValid(World))
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}

		ANF_PlayerState* PlayerState = Pair.Key.Get();

		if (bMarkEliminated && IsValid(PlayerState))
		{
			SetPlayerPawnState(PlayerState, ENF_PlayerPawnState::Eliminated);
		}
	}

	RespawnTimerHandles.Empty();
}

ANF_TeamPlayerStart* UNF_PlayerRespawnComponent::FindTeamPlayerStart(
	ANF_PlayerState* PlayerState)
{
	if (!HasServerAuthority() || bStopped || !IsValid(PlayerState) ||
		PlayerState->GetWorld() != GetWorld())return nullptr;

	const int32 TeamId = PlayerState->NF_TeamID;
	if (TeamId < 1 || TeamId > 4)return nullptr;

	const TWeakObjectPtr<ANF_PlayerState> PlayerStateKey(PlayerState);
	if (const TWeakObjectPtr<ANF_TeamPlayerStart>* AssignedStart = AssignedPlayerStarts.Find(PlayerStateKey))
	{
		if (AssignedStart->IsValid() && AssignedStart->Get()->GetTeamId() == TeamId)
		{
			return AssignedStart->Get();
		}
	}

	AssignedPlayerStarts.Remove(PlayerStateKey);
	TArray<ANF_TeamPlayerStart*> Candidates;
	for (TActorIterator<ANF_TeamPlayerStart> Iterator(GetWorld());
		Iterator;
		++Iterator)
	{
		ANF_TeamPlayerStart* TeamPlayerStart = *Iterator;

		if (IsValid(TeamPlayerStart) &&
			TeamPlayerStart->GetTeamId() == TeamId)
		{
			Candidates.Add(TeamPlayerStart);
		}
	}

	// 액터 순회 순서와 무관하게 같은 순서로 지점을 예약한다.
	Candidates.Sort([](const ANF_TeamPlayerStart& Left, const ANF_TeamPlayerStart& Right)
	{
		return Left.GetPathName() < Right.GetPathName();
	});

	for (ANF_TeamPlayerStart* Candidate : Candidates)
	{
		bool bReserved = false;
		for (const auto& Assignment : AssignedPlayerStarts)
		{
			if (Assignment.Key.IsValid() && Assignment.Value.Get() == Candidate)
			{
				bReserved = true;
				break;
			}
		}

		if (!bReserved)
		{
			AssignedPlayerStarts.Add(PlayerStateKey, Candidate);
			return Candidate;
		}
	}

	return nullptr;
}

void UNF_PlayerRespawnComponent::RetryTeamPendingHealthZeroChecks(
	const int32 TeamId)
{
	const ANF_GameMode* GameMode = GetNFGameMode();
	const ANF_GameState* NFGameState = nullptr;

	if (IsValid(GameMode))
	{
		NFGameState = GameMode->GetGameState<ANF_GameState>();
	}

	if (!IsValid(NFGameState))return;

	for (APlayerState* BasePlayerState : NFGameState->PlayerArray)
	{
		ANF_PlayerState* TeamPlayerState =
			Cast<ANF_PlayerState>(BasePlayerState);

		if (!IsValid(TeamPlayerState) ||
			TeamPlayerState->NF_TeamID != TeamId)
		{
			continue;
		}

		APawn* TeamPawn = TeamPlayerState->GetPawn();

		if (!IsValid(TeamPawn))continue;

		UNF_PlayerLifeComponent* LifeComponent =
			TeamPawn->FindComponentByClass<UNF_PlayerLifeComponent>();

		if (IsValid(LifeComponent))
		{
			LifeComponent->RetryPendingHealthZeroCheck();
		}
	}
}
