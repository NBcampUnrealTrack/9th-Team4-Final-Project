#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "GameFramework/Character.h"
#include "NF_Player.generated.h"

class UNF_GASFunctionLibraryComponent;
class ANF_PlayerState;
class UMotionMatchingComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PROJECT_NF_API ANF_Player : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ANF_Player(const FObjectInitializer& ObjectInitializer);
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override; 
	

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	
	virtual void BeginPlay() override;
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF_Player|MotionMatching")
	TObjectPtr<UMotionMatchingComponent> MotionMatchingComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF_Player|Mesh")
	TObjectPtr<USkeletalMeshComponent> VisualMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF_Player|Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF_Player|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "GAS|GameplayAbility")
	TArray<TSubclassOf<class UGameplayAbility>> GameplayAbilityArray;
	
	
private:
	void InitAbilitySystem();
};
