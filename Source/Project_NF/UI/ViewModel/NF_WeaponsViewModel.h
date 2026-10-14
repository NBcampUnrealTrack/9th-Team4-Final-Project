// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/NF_ViewModelBase.h"
#include "NF_WeaponsViewModel.generated.h"

class UNF_WeaponDataAsset;
class UNF_BuildItemDefinition;
class UTexture2D;

UCLASS()
class PROJECT_NF_API UNF_WeaponsViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()
	
public:
	void Configure(
		const UNF_BuildItemDefinition* Item,
		const UNF_WeaponDataAsset* WeaponData,
		UTexture2D* InIcon,
		const FText& InDescription);

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
	float Damage = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	float RoundsPerMinute = 0.0f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	int32 MagazineSize = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta=(AllowPrivateAccess="true"))
	FText FireModeText;
	
	FText GetFireModeText(ENF_FireMode FireMode) const;
};
