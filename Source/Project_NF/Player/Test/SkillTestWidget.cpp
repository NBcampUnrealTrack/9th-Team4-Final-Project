// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillTestWidget.h"

#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/Data/NF_PlayerActionType.h"


void USkillTestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnPlayerBindingSkill, OnPlayerBindingSkill);
}


void USkillTestWidget::NativeDestruct()
{
	if (IsValid(GetWorld()) == true)
	{
		auto* PlayerActionDelegateSubsystem = GetWorld()->GetSubsystem<UNF_PlayerActionDelegateSubsystem>();
		if (IsValid(PlayerActionDelegateSubsystem) == true)
		{
			PlayerActionDelegateSubsystem->OnPlayerBindingSkill.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}


void USkillTestWidget::OnPlayerBindingSkill(APlayerController* SendController, ENF_PlayerActionType PlayerActionType)
{
	if (SendController != GetOwningPlayer()) return;

	if (PlayerActionType == ENF_PlayerActionType::SkillA)
	{
		SetSkillSlotImage(SkillSlotImage_A, SkillTexture_A);
		
		return;
	}

	if (PlayerActionType == ENF_PlayerActionType::SkillB)
	{
		SetSkillSlotImage(SkillSlotImage_B, SkillTexture_B);
		return;
	}
}


void USkillTestWidget::SetSkillSlotImage(UImage* SkillSlotImage, UTexture2D* SkillTexture)
{
	if (IsValid(SkillSlotImage) == false) return;
	if (IsValid(SkillTexture) == false) return;

	SkillSlotImage->SetBrushFromTexture(SkillTexture);
	SkillSlotImage->SetOpacity(1.0);
	
}
