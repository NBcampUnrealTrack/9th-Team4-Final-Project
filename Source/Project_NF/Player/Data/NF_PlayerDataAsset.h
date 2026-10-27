#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_PlayerDataAsset.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage; 
class UDataTable;
/**
 * 
 */
UCLASS()
class PROJECT_NF_API UNF_PlayerDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UNF_PlayerDataAsset();
	
	UPROPERTY(EditAnywhere, Category = "GAS|GA_Montage")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;
	
	UPROPERTY(EditAnywhere, Category = "GAS|GA_Montage")
	TObjectPtr<UAnimMontage> DeathMontage;
	
	UPROPERTY(EditAnywhere, Category = "GAS|MeleeAttack", DisplayName = "MeleeTraceRadius (근접공격 범위)")
	float MeleeTraceRadius = 20.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|MeleeAttack")
	FName MeleeSocket_Start = "MeleeSocket_Start";
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|MeleeAttack")
	FName MeleeSocket_End = "MeleeSocket_End";
	
	UPROPERTY(EditAnywhere, Category = "GAS|PayLoad_Damage", DisplayName = "MeleeAttackDamage (음수)")
	float MeleeAttackDamage = 10.0f;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Interact" , DisplayName = "InteractionRange (상호작용 범위)")
	float InteractionRange = 300.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player|Socket")
	FName MeleeAttackSocket = "MeleeAttackSocket";
	
	
	UPROPERTY(EditDefaultsOnly, Category = "PlayerState|GA_Class", DisplayName = "GameplayAbility (Play 전 적용 될 GA_Class)")
	TArray<TSubclassOf<UGameplayAbility>> InitGA_Array;
	
	UPROPERTY(EditAnywhere, Category = "PlayerState|GE_Class", DisplayName = "MeleeAttack_GEClass (Health 영향)")
	TSubclassOf<UGameplayEffect> MeleeAttack_Health_GEClass;
	
	UPROPERTY(EditAnywhere, Category = "PlayerState|GE_Class", DisplayName = "MeleeAttack_GEClass (Shield 영향)")
	TSubclassOf<UGameplayEffect> MeleeAttack_Shield_GEClass;
	
	UPROPERTY(EditAnywhere, Category = "PlayerState|GE_Class", DisplayName = "GainExp_GEClass (IncomingExp 영향)")
	TSubclassOf<UGameplayEffect> GainExp_GEClass;
	
	UPROPERTY(EditAnywhere, Category = "PlayerState|PlayerLV", DisplayName = "PlayerLevelDataTable (1번째 행 = 1레벨)")
	TObjectPtr<UDataTable> PlayerLevelDataTable;
};
