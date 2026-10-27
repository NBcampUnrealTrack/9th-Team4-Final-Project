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
	
	/*UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character")
	TObjectPtr<USkeletalMesh> VisualMesh;*/
	
	UPROPERTY(EditAnywhere, Category = "GA_Montage")
	TObjectPtr<UAnimMontage> MeleeAttackMontage;
	
	UPROPERTY(EditAnywhere, Category = "GA_Montage")
	TObjectPtr<UAnimMontage> DeathMontage;
	
	UPROPERTY(EditAnywhere, Category = "PayLoad_Damage", DisplayName = "MeleeAttackDamage (음수)")
	float MeleeAttackDamage = 0.0f;
	
	UPROPERTY(EditAnywhere, Category = "GA_ETC", DisplayName = "MeleeTraceRadius (근접공격 범위)")
	float MeleeTraceRadius = 20.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GA_Class", DisplayName = "GameplayAbility (Play 전 적용 될 GA_Class)")
	TArray<TSubclassOf<UGameplayAbility>> InitGA_Array;
	
	UPROPERTY(EditAnywhere, Category = "GE_Class", DisplayName = "MeleeAttack_GEClass (Health 영향)")
	TSubclassOf<UGameplayEffect> MeleeAttack_Health_GEClass;
	
	UPROPERTY(EditAnywhere, Category = "GE_Class", DisplayName = "MeleeAttack_GEClass (Shield 영향)")
	TSubclassOf<UGameplayEffect> MeleeAttack_Shield_GEClass;
	
	UPROPERTY(EditAnywhere, Category = "GE_Class", DisplayName = "GainExp_GEClass (IncomingExp 영향)")
	TSubclassOf<UGameplayEffect> GainExp_GEClass;
	
	UPROPERTY(EditAnywhere, Category = "PlayerLV", DisplayName = "PlayerLevelDataTable (1번째 행 = 1레벨)")
	TObjectPtr<UDataTable> PlayerLevelDataTable;
};
