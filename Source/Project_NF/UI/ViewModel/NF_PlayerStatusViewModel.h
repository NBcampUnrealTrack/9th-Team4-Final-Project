// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/NF_GASViewModelBase.h"
#include "NF_PlayerStatusViewModel.generated.h"

struct FOnAttributeChangeData;

UCLASS()
class PROJECT_NF_API UNF_PlayerStatusViewModel : public UNF_GASViewModelBase
{
	GENERATED_BODY()
	
	public:
    // 각 데이터 시스템이 완성되면 변경 알림을 받아 호출
    void UpdateProgression(int32 NewLevel,float NewExperience,float NewRequiredExperience);

    void UpdateCost(int32 NewUsedCost, int32 NewMaxCost);

    UFUNCTION(BlueprintPure, FieldNotify)
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure, FieldNotify)
    float GetShieldPercent() const;

    UFUNCTION(BlueprintPure, FieldNotify)
    float GetExperiencePercent() const;

protected:
    virtual void OnInitializeViewModel() override;
    virtual void OnViewModelContextChanged() override;
    virtual void BindAbilitySystemDelegates() override;
    virtual void RefreshFromAbilitySystem() override;

private:
    void HandleVitalAttributeChanged(const FOnAttributeChangeData& Data);

    void RefreshPlayerName();

    void UpdateVitals(float NewHealth,float NewMaxHealth,float NewShield,float NewMaxShield);

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    FText PlayerName;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    float Health = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    float MaxHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    float Shield = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    float MaxShield = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    int32 Level = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    float Experience = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    float RequiredExperience = 0.0f;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    int32 UsedCost = 0;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Category="Player|UI",meta=(AllowPrivateAccess="true"))
    int32 MaxCost = 0;
};
