#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "NF_PlayerState.generated.h"

class UNF_PlayerStatusAttribute;
class UNF_AbilitySystemComponent;

UCLASS()
class PROJECT_NF_API ANF_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	ANF_PlayerState();
	
	//Core
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Core|ID")
	int32 NF_TeamID = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, Category = "Core|ID")
	int32 NF_PlayerID;
	

	//GAS
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	bool SetNFTeamID(int32 NewTeamID);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS|ASC")
	TObjectPtr<UNF_AbilitySystemComponent> PlayerASC;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS|Attribute")
	TObjectPtr<UNF_PlayerStatusAttribute> PlayerStatusAttribute; 

	
protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	
};
