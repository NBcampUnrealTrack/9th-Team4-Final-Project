#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "NF_PlayerState.generated.h"

class UNF_PlayerLevelAttribute;
class UNF_KeyHolderAttributeSet;
class UGameplayAbility;
class UNF_PlayerDataAsset;
class UNF_PlayerStatusAttribute;
class UNF_AbilitySystemComponent;

// generated.h include 아래, 클래스 선언 위
DECLARE_MULTICAST_DELEGATE(FNF_OnTeamIDChanged);

UCLASS()
class PROJECT_NF_API ANF_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ANF_PlayerState();
	
	void InitAbilitySystem(APawn* AvatarPawn);
	void GiveDefaultAbility();

	bool bIsGiveAbility = false; 
	
	UPROPERTY(EditAnywhere, Category = "Player|Data")
	TObjectPtr<UNF_PlayerDataAsset> NF_PlayerDataAsset;  
	
	UPROPERTY()
	TArray<TSubclassOf<UGameplayAbility>> NF_PlayerNewAbility_Array;
	
	
	//Core
	UPROPERTY(ReplicatedUsing = OnRep_NFTeamID, BlueprintReadOnly, Category = "Core|ID")
	int32 NF_TeamID = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, Category = "Core|ID")
	int32 NF_PlayerID;
	
	int32 GetNFTeamID() const {return NF_TeamID;}
	
	
	//GAS
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	bool SetNFTeamID(int32 NewTeamID);
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|ASC")
	TObjectPtr<UNF_AbilitySystemComponent> PlayerASC;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Attribute")
	TObjectPtr<UNF_PlayerStatusAttribute> PlayerStatusAttribute; 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Attribute")
	TObjectPtr<UNF_PlayerLevelAttribute> PlayerLevelAttribute; 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Player|Attribute")
	TObjectPtr<UNF_KeyHolderAttributeSet> KeyHolderAttributeSet; 

	FNF_OnTeamIDChanged OnTeamIDChanged;
	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
private:
	void ConfigDataAsset();
	
	UFUNCTION()
	void OnRep_NFTeamID();
	
};
