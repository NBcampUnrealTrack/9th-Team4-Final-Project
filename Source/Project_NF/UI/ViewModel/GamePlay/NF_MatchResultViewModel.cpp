#include "UI/ViewModel/GamePlay/NF_MatchResultViewModel.h"

void UNF_MatchResultViewModel::ApplyResult(const FNF_MatchResult& Result, const int32 LocalTeamId)
{
	FText NewText;
	bHasResult = false;

	if (Result.Outcome == ENF_MatchOutcome::Aborted)
	{
		NewText = NSLOCTEXT("NFMatchResult", "Aborted", "매치 취소");
		bHasResult = true;
	}
	else if (Result.Outcome == ENF_MatchOutcome::Victory && Result.WinningTeamId > 0)
	{
		bHasResult = true;
		if (LocalTeamId <= 0)
		{
			NewText = NSLOCTEXT("NFMatchResult", "WaitingTeam", "팀 결과 확인 중");
		}
		else if (LocalTeamId == Result.WinningTeamId)
		{
			NewText = FText::Format(NSLOCTEXT("NFMatchResult", "Victory", "{0}팀 승리"), FText::AsNumber(LocalTeamId));
		}
		else
		{
			NewText = FText::Format(NSLOCTEXT("NFMatchResult", "Defeat", "{0}팀 패배"), FText::AsNumber(LocalTeamId));
		}
	}

	// 팀 정보가 늦게 도착해 재적용되어도 진행 중인 복귀 요청은 유지한다.
	UE_MVVM_SET_PROPERTY_VALUE(ResultText, NewText);
	RefreshCanReturn();
}

void UNF_MatchResultViewModel::SetReturnPending(const bool bPending)
{
	bReturnPending = bPending;
	RefreshCanReturn();
}

void UNF_MatchResultViewModel::RefreshCanReturn()
{
	UE_MVVM_SET_PROPERTY_VALUE(bCanReturn, bHasResult && !bReturnPending);
}
