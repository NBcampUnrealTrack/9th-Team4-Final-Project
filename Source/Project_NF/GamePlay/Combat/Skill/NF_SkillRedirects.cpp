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
		for (const TCHAR* MovedSkillField : { TEXT("CastMontage"), TEXT("CastMontagePlayRate"), TEXT("CastMontageStartSection"),
			TEXT("bAffectedByEMP"), TEXT("bCancelOnEMP"), TEXT("ExecutionActorClass"), TEXT("DefaultPreviewData"),
			TEXT("InputCancelTags"), TEXT("bRequiresConfirmedTargetData"), TEXT("bActivateOnInputRelease"),
			TEXT("PreviewActorClass"), TEXT("SkillExecutionMode"), TEXT("CastTime"), TEXT("ExecutionDuration") })
		{
			SkillOwnershipRedirects.Emplace(ECoreRedirectFlags::Type_Property, OldSkillOwner + MovedSkillField, ActiveSkillOwner + MovedSkillField);
		}
		for (const TCHAR* MovedSkillField : { TEXT("ActivationCueTag"), TEXT("ActiveCueTag"), TEXT("PassiveEffects"),
			TEXT("PassiveEventTags"), TEXT("bActivatePassiveOnGrant") })
		{
			SkillOwnershipRedirects.Emplace(ECoreRedirectFlags::Type_Property, OldSkillOwner + MovedSkillField, PassiveSkillOwner + MovedSkillField);
		}
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
