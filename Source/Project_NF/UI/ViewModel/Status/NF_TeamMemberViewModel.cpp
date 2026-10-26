#include "UI/ViewModel/Status/NF_TeamMemberViewModel.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

void UNF_TeamMemberViewModel::SetMember(APlayerState* NewPlayerState)
{
	const bool bMemberChanged = MemberPlayerState.Get() != NewPlayerState;
	MemberPlayerState = NewPlayerState;

	FNF_ViewModelContext MemberContext;
	if (IsValid(NewPlayerState))
	{
		MemberContext.PlayerState = NewPlayerState;
		MemberContext.Pawn = NewPlayerState->GetPawn();
		MemberContext.AbilitySystemComponent =UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPlayerState);
	}

	UpdateViewModelContext(MemberContext);

	RefreshFromAbilitySystem();

	if (bMemberChanged)
	{
		SetMemberLevel(1);
	}
}

APlayerState* UNF_TeamMemberViewModel::GetMemberPlayerState() const
{
	return MemberPlayerState.Get();
}

void UNF_TeamMemberViewModel::SetMemberLevel(int32 NewLevel)
{
	UpdateProgression(FMath::Max(1, NewLevel), 0.0f, 0.0f);
}

void UNF_TeamMemberViewModel::OnDeinitializeViewModel()
{
	// 부모가 해당 팀원 ASC에 등록된 델리게이트를 해제한다.
	Super::OnDeinitializeViewModel();
	MemberPlayerState.Reset();
}
