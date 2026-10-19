#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "GameFramework/Character.h"
#include "Player/Data/NF_PlayerActionType.h"
#include "Player/Data/NF_PlayerInteractTypeEnum.h"
#include "NF_Player.generated.h"

enum class ENF_BuildShopUIState : uint8;
enum class ENF_PlayerActionType : uint8;

class UNF_WeaponComponent;
class UNF_BuildComponent;
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
	
	void OnTryAction(APlayerController* SendController, ENF_PlayerActionType ActionType);
	void OnSkillBind(APlayerController* SendController, ENF_PlayerActionType ActionType);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF_Player|Mesh")
	TObjectPtr<USkeletalMeshComponent> VisualMesh;
	
	
protected:
	virtual void PostInitializeComponents() override;
	
	virtual void BeginPlay() override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void OnRep_PlayerState() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF_Player|Camera")
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF_Player|Camera")
	TObjectPtr<UCameraComponent> CameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF_Player|MotionMatching")
	TObjectPtr<UMotionMatchingComponent> MotionMatchingComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "NF_Player|Build")
	TObjectPtr<UNF_BuildComponent> NF_BuildComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "NF_Player|Weapon")
	TObjectPtr<UNF_WeaponComponent> NF_WeaponComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "GAS|GameplayAbility")
	TArray<TSubclassOf<class UGameplayAbility>> GameplayAbilityArray;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PlayerController|Interaction")
	float InteractionRange = 300.0f;
	
private:
	void BindDelegate();
	
#pragma region TryAction
	void TryStartAttack();
	
	void TryStopAttack();
	
	void TryZoom();
	
	void TryReload();
	
	void TrySkillActionWithInputID(int32 InputID);
	
	void TryInteraction(APlayerController* SendController);
	
	UFUNCTION(Server, Reliable)
	void ServerRPC_TryInteraction(APlayerController* SendController, AActor* TargetActor);
	
	FHitResult GetActorFromLineTrace();
	
	static constexpr int32 SkillButtonA_InputID = 0;
	
	static constexpr int32 SkillButtonB_InputID = 1;
#pragma endregion
	
	
#pragma region BuildShop
	void OnEnablePlayerMovementFromType(ENF_BuildShopUIState BuildShopType);
#pragma endregion 
	
	
#pragma region AbilitySystem
	UFUNCTION()
	void OnOutOfHealthCpp();
	
	UFUNCTION()
	void OnOutOfShieldCpp();
	
	void InitAbilitySystem();
	
	void GiveDefaultAbility();
	
	void AddGameplayAbility();
#pragma endregion
};
