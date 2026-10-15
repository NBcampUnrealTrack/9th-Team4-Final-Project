// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_ArmorsViewModel.generated.h"


class UNF_BuildItemDefinition;
class UTexture2D;

UCLASS()
class PROJECT_NF_API UNF_ArmorsViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()
	
public:
	void Configure(
		const UNF_BuildItemDefinition* Item,
		UTexture2D* InIcon,
		const FText& InDescription,
		const FText& InEquipSlotText,
		float InDefenceBonus);

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	FName ItemId;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	int32 Cost = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	int32 Tier = 1;
	
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	FText Description;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	FText EquipSlotText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	float DefenceBonus = 0.0f;
};
