#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "NF_PlayerLevelAttribute.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLevelUp, int32, NewLevel);

UCLASS()
class PROJECT_NF_API UNF_PlayerLevelAttribute : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UNF_PlayerLevelAttribute();
	
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, MaxLevel);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, CurrentLevel);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, CurrentExp);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, TargetExp);
	ATTRIBUTE_ACCESSORS_BASIC(ThisClass, IncomingExp);
	
	UPROPERTY(BlueprintAssignable)
	mutable FOnLevelUp OnLevelUp;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_MaxLevel, Category = "Player|Attribute")
	FGameplayAttributeData MaxLevel = 10.0f;	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentLevel, Category = "Player|Attribute")
	FGameplayAttributeData CurrentLevel = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentExp, Category = "Player|Attribute")
	FGameplayAttributeData CurrentExp = 0.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_TargetExp, Category = "Player|Attribute")
	FGameplayAttributeData TargetExp = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_IncomingExp, Category = "Player|Attribute")
	FGameplayAttributeData IncomingExp = 0.0f;
	
	UFUNCTION()
	void OnRep_MaxLevel(const FGameplayAttributeData& OldMaxLevel);
	
	UFUNCTION()
	void OnRep_CurrentLevel(const FGameplayAttributeData& OldCurrentLevel);
	
	UFUNCTION()
	void OnRep_CurrentExp(const FGameplayAttributeData& OldCurrentExp);
	
	UFUNCTION()
	void OnRep_TargetExp(const FGameplayAttributeData& OldTargetExp);
	
	UFUNCTION()
	void OnRep_IncomingExp(const FGameplayAttributeData& OldIncomingExp);
	
private:
	//추가: 현재 레벨의 목표 경험치를 데이터 에셋 LevelInfos에서 읽음
	float GetTargetExpFromLevel() const;
};
