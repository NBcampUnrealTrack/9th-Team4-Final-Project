#include "UI/ViewModel/Status/NF_TeamPanelViewModel.h"

#include "Core/NF_GameState.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Player/State/NF_PlayerState.h"
#include "UI/ViewModel/Status/NF_TeamMemberViewModel.h"

void UNF_TeamPanelViewModel::OnInitializeViewModel()
{
	Super::OnInitializeViewModel();
	BindTeamRoster();
	RefreshMembers();
	StartTeamRosterRetry();
}

void UNF_TeamPanelViewModel::OnViewModelContextChanged()
{
	Super::OnViewModelContextChanged();
	StopTeamRosterRetry();
	UnbindTeamRoster();

	BindTeamRoster();
	RefreshMembers();
	StartTeamRosterRetry();
}

void UNF_TeamPanelViewModel::OnDeinitializeViewModel()
{
	StopTeamRosterRetry();
	UnbindTeamRoster();
	ClearMembers();
	Super::OnDeinitializeViewModel();
}

void UNF_TeamPanelViewModel::BindTeamRoster()
{
	APlayerController* PlayerController = GetViewModelContext().PlayerController.Get();
	UWorld* World = nullptr;
	if (IsValid(PlayerController))
	{
		World = PlayerController->GetWorld();
	}

	ANF_GameState* NewGameState = nullptr;
	if (IsValid(World))
	{
		NewGameState = World->GetGameState<ANF_GameState>();
	}

	if (BoundGameState.Get() == NewGameState) return;

	UnbindTeamRoster();
	if (!IsValid(NewGameState)) return;

	BoundGameState = NewGameState;
	NewGameState->OnTeamRosterChanged.AddUObject(this, &ThisClass::HandleTeamRosterChanged);
}

void UNF_TeamPanelViewModel::UnbindTeamRoster()
{
	ANF_GameState* GameState = BoundGameState.Get();
	if (IsValid(GameState))
	{
		GameState->OnTeamRosterChanged.RemoveAll(this);
	}

	BoundGameState.Reset();
}

void UNF_TeamPanelViewModel::HandleTeamRosterChanged()
{
	RefreshMembers();
}

void UNF_TeamPanelViewModel::StartTeamRosterRetry()
{
	if (BoundGameState.IsValid()) return;

	APlayerController* PlayerController = GetViewModelContext().PlayerController.Get();
	if (!IsValid(PlayerController)) return;

	UWorld* World = PlayerController->GetWorld();
	if (!IsValid(World)) return;

	StopTeamRosterRetry();
	TeamRosterRetryWorld = World;
	World->GetTimerManager().SetTimer(
		TeamRosterRetryHandle,
		this,
		&ThisClass::RetryBindTeamRoster,
		0.2f,
		true);
}

void UNF_TeamPanelViewModel::StopTeamRosterRetry()
{
	UWorld* World = TeamRosterRetryWorld.Get();
	if (IsValid(World))
	{
		World->GetTimerManager().ClearTimer(TeamRosterRetryHandle);
	}

	TeamRosterRetryHandle.Invalidate();
	TeamRosterRetryWorld.Reset();
}

void UNF_TeamPanelViewModel::RetryBindTeamRoster()
{
	BindTeamRoster();
	if (!BoundGameState.IsValid()) return;

	StopTeamRosterRetry();
	// 구독 전에 도착했던 명단도 현재 상태로 복원한다.
	RefreshMembers();
}

void UNF_TeamPanelViewModel::RefreshMembers()
{
	ANF_PlayerState* LocalPlayerState = Cast<ANF_PlayerState>(GetViewModelContext().PlayerState.Get());
	if (!IsValid(LocalPlayerState))
	{
		ClearMembers();
		return;
	}
	if (LocalPlayerState->NF_TeamID == INDEX_NONE)
	{
		ClearMembers();
		return;
	}

	UWorld* World = LocalPlayerState->GetWorld();
	if (!IsValid(World))
	{
		ClearMembers();
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!IsValid(GameState))
	{
		ClearMembers();
		return;
	}

	TArray<TObjectPtr<UNF_TeamMemberViewModel>> NewMembers;
	for (const TObjectPtr<APlayerState>& PlayerState : GameState->PlayerArray)
	{
		ANF_PlayerState* TeamPlayerState = Cast<ANF_PlayerState>(PlayerState.Get());
		if (!IsValid(TeamPlayerState)) continue;
		
		if (TeamPlayerState == LocalPlayerState) continue;
		
		if (TeamPlayerState->NF_TeamID != LocalPlayerState->NF_TeamID) continue;

		UNF_TeamMemberViewModel* MemberVM = FindMember(TeamPlayerState);
		if (!IsValid(MemberVM))
		{
			MemberVM = NewObject<UNF_TeamMemberViewModel>(this);
		}

		MemberVM->SetMember(TeamPlayerState);
		NewMembers.Add(MemberVM);
	}

	// 목록에서 제외된 팀원의 GAS 델리게이트를 해제한다.
	for (const TObjectPtr<UNF_TeamMemberViewModel>& MemberVM : Members)
	{
		if (!IsValid(MemberVM.Get())) continue;
		
		if (!NewMembers.Contains(MemberVM))
		{
			MemberVM->DeinitializeViewModel();
		}
	}

	if (Members != NewMembers)
	{
		Members = MoveTemp(NewMembers);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Members);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMemberCount);
	}
}

void UNF_TeamPanelViewModel::ClearMembers()
{
	if (Members.IsEmpty()) return;
	
	for (const TObjectPtr<UNF_TeamMemberViewModel>& MemberVM : Members)
	{
		if (IsValid(MemberVM.Get()))
		{
			MemberVM->DeinitializeViewModel();
		}
	}

	Members.Reset();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Members);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetMemberCount);
}

UNF_TeamMemberViewModel* UNF_TeamPanelViewModel::FindMember(APlayerState* PlayerState) const
{
	if (!IsValid(PlayerState)) {return nullptr;}
	
	for (const TObjectPtr<UNF_TeamMemberViewModel>& MemberVM : Members)
	{
		if (!IsValid(MemberVM.Get())) continue;
		
		if (MemberVM->GetMemberPlayerState() == PlayerState)
		{
			return MemberVM.Get();
		}
	}
	
	return nullptr;
}

int32 UNF_TeamPanelViewModel::GetMemberCount() const
{
	return Members.Num();
}
