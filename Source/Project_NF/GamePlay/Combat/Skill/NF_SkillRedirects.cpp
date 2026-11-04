#include "GamePlay/Combat/Skill/NF_SkillRedirects.h"
#include "UObject/CoreRedirects.h"

void NFRegisterSkillOwnershipRedirects()
{
	static const bool bSkillRedirectsRegistered = []()
	{
		TArray<FCoreRedirect> SkillOwnershipRedirects;
		const FString OldSkillOwner = TEXT("/Script/Project_NF.NFGA_SkillBase.");
		const FString ActiveSkillOwner = TEXT("/Script/Project_NF.NFGA_ActiveSkillBase.");
		const FString PassiveSkillOwner = TEXT("/Script/Project_NF.NFGA_PassiveSkillBase.");
		// 설정 필드는 DA로 이전되어 Self 변수 노드를 자동 redirect하지 않는다.
		for (const TCHAR* MovedSkillFunction : { TEXT("BeginLocalPreview"), TEXT("EndLocalPreview"), TEXT("GetSkillPreviewData"),
			TEXT("BuildConfirmedTargetData"), TEXT("ValidateConfirmedTargetData"), TEXT("LocalInputCanceled") })
		{
			SkillOwnershipRedirects.Emplace(ECoreRedirectFlags::Type_Function, OldSkillOwner + MovedSkillFunction, ActiveSkillOwner + MovedSkillFunction);
		}
		for (const TCHAR* MovedSkillFunction : { TEXT("TrackPassiveEffect"), TEXT("NotifyPassiveTriggered"), TEXT("StartPassiveActiveCue"),
			TEXT("StopPassiveActiveCue"), TEXT("PassiveEquipped"), TEXT("PassiveUnequipped"), TEXT("PassiveGameplayEvent") })
		{
			SkillOwnershipRedirects.Emplace(ECoreRedirectFlags::Type_Function, OldSkillOwner + MovedSkillFunction, PassiveSkillOwner + MovedSkillFunction);
		}
		return FCoreRedirects::AddRedirectList(SkillOwnershipRedirects, TEXT("NFSkillOwnershipMigration"));
	}();
	(void)bSkillRedirectsRegistered;
}
