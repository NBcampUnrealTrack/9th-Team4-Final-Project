#include "UI/ViewModel/Status/NF_TeamPanelViewModel.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Player/State/NF_PlayerState.h"
#include "UI/ViewModel/Status/NF_TeamMemberViewModel.h"

void UNF_TeamPanelViewModel::OnInitializeViewModel()
{
	Super::OnInitializeViewModel();
	RefreshMembers();
}

void UNF_TeamPanelViewModel::OnViewModelContextChanged()
{
	Super::OnViewModelContextChanged();
	RefreshMembers();
}

void UNF_TeamPanelViewModel::OnDeinitializeViewModel()
{
	ClearMembers();
	Super::OnDeinitializeViewModel();
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
