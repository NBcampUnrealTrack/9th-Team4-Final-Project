#include "NF_InteractionViewModel.h"

#include "EnhancedActionKeyMapping.h"
#include "InputMappingContext.h"
#include "Player/Data/NF_PlayerInputDataAsset.h"

void UNF_InteractionViewModel::ConfigureInput(
	const UNF_PlayerInputDataAsset* InputDataAsset)
{
	FText NewInteractionKeyText;

	if (IsValid(InputDataAsset)&& IsValid(InputDataAsset->NF_PlayerIMC)&& IsValid(InputDataAsset->InteractButton))
	{
		const TArray<FEnhancedActionKeyMapping>& Mappings =InputDataAsset->NF_PlayerIMC->GetMappings();

		for (const FEnhancedActionKeyMapping& Mapping : Mappings)
		{
			if (Mapping.Action != InputDataAsset->InteractButton)continue;

			if (!Mapping.Key.IsValid())continue;

			NewInteractionKeyText =Mapping.Key.GetDisplayName();

			break;
		}
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(InteractionKeyText,NewInteractionKeyText))
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetInteractionPromptText);
	}
}

void UNF_InteractionViewModel::ApplyInteractionState(const FNF_InteractionViewData& NewState)
{
	bool bPromptChanged = false;

	if (UE_MVVM_SET_PROPERTY_VALUE(bShowInteractionPrompt,NewState.bHasTarget))
	{
		bPromptChanged = true;
	}

	UE_MVVM_SET_PROPERTY_VALUE(TargetType,NewState.TargetType);

	if (UE_MVVM_SET_PROPERTY_VALUE(TargetName,NewState.TargetName))
	{
		bPromptChanged = true;
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(InteractionActionText,NewState.ActionText))
	{
		bPromptChanged = true;
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(bCanInteract,NewState.bCanInteract))
	{
		bPromptChanged = true;
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(UnavailableReason,NewState.UnavailableReason))
	{
		bPromptChanged = true;
	}

	bool bNewShowProgress = false;

	if (NewState.bHasTarget&& NewState.bIsInteracting)
	{
		bNewShowProgress = true;
	}

	UE_MVVM_SET_PROPERTY_VALUE(bShowInteractionProgress,bNewShowProgress);

	const float NewProgress =FMath::Clamp(NewState.Progress, 0.0f, 1.0f);

	UE_MVVM_SET_PROPERTY_VALUE(InteractionProgress,NewProgress);

	if (bPromptChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetInteractionPromptText);
	}
}

void UNF_InteractionViewModel::ClearInteractionState()
{
	bool bPromptChanged = false;

	if (UE_MVVM_SET_PROPERTY_VALUE(bShowInteractionPrompt,false))
	{
		bPromptChanged = true;
	}

	UE_MVVM_SET_PROPERTY_VALUE(TargetType,ENF_InteractionTargetType::None);

	if (UE_MVVM_SET_PROPERTY_VALUE(TargetName,FText::GetEmpty()))
	{
		bPromptChanged = true;
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(InteractionActionText,FText::GetEmpty()))
	{
		bPromptChanged = true;
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(bCanInteract,false))
	{
		bPromptChanged = true;
	}

	if (UE_MVVM_SET_PROPERTY_VALUE(UnavailableReason,FText::GetEmpty()))
	{
		bPromptChanged = true;
	}

	UE_MVVM_SET_PROPERTY_VALUE(bShowInteractionProgress,false);
	UE_MVVM_SET_PROPERTY_VALUE(InteractionProgress,0.0f);

	if (bPromptChanged)
	{
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetInteractionPromptText);
	}
}

FText UNF_InteractionViewModel::GetInteractionPromptText() const
{
	if (!bShowInteractionPrompt) {return FText::GetEmpty();}

	FText DisplayActionText = InteractionActionText;

	if (!bCanInteract&& !UnavailableReason.IsEmpty())
	{
		DisplayActionText = UnavailableReason;
	}

	if (InteractionKeyText.IsEmpty())
	{
		if (TargetName.IsEmpty()) {return DisplayActionText;}

		return FText::Format(FText::FromString(TEXT("{0} - {1}")),TargetName,DisplayActionText);
	}

	if (TargetName.IsEmpty())
	{
		return FText::Format(FText::FromString(TEXT("[{0}] {1}")),InteractionKeyText,DisplayActionText);
	}

	return FText::Format(FText::FromString(TEXT("[{0}] {1} - {2}")),InteractionKeyText,TargetName,DisplayActionText);
}