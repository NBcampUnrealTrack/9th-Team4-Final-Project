// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_SkillsViewModel.generated.h"

struct FNF_BuildItemData;
class UNF_BuildItemDefinitionDataAsset;
class UTexture2D;

UCLASS()
class PROJECT_NF_API UNF_SkillsViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()
	
public:
	void Configure(
		const UNF_BuildItemDefinitionDataAsset* Item, const FNF_BuildItemData& ItemData,UTexture2D* InIcon,const FText& InDescription,float InCooldownSeconds);

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	FName SkillId;

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
	float CooldownSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	float CastTime = 0.0f;
	
	float ResolveSkillCastTime(const FNF_BuildItemData& ItemData) const;
};
