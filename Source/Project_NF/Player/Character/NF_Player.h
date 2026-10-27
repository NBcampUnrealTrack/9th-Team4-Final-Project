#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Data/Interface/NF_MeleeAttackInterface.h"
#include "GameFramework/Character.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "Player/Data/NF_PlayerActionType.h"
#include "NF_Player.generated.h"

class UNF_ConsumableComponent;
class UNF_PlayerDataAsset;
class UNF_PlayerLifeComponent;
enum class ENF_WeaponType : uint8;
enum class ENF_BuildShopUIState : uint8;
enum class ENF_PlayerActionType : uint8;

class UNF_ArmorComponent;
class UNF_WeaponComponent;
class UNF_BuildComponent;
class UNF_GASFunctionLibraryComponent;
class ANF_PlayerState;
class UMotionMatchingComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class PROJECT_NF_API ANF_Player : public ACharacter, public IAbilitySystemInterface, public INF_MeleeAttackInterface
{
	GENERATED_BODY()

public:
	ANF_Player(const FObjectInitializer& ObjectInitializer);
	
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override; 
	
	virtual UMeshComponent* NF_GetMeleeAttackMesh() override {return MeleeAttackMeshComponent;};
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void OnTryAction(APlayerController* SendController, ENF_PlayerActionType ActionType);
	void OnSkillBind(APlayerController* SendController, ENF_PlayerActionType ActionType);
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF_Player|Mesh")
	TObjectPtr<USkeletalMeshComponent> VisualMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF_Player|Mesh")
	TObjectPtr<UStaticMeshComponent> MeleeAttackMeshComponent;
	
	UPROPERTY(EditAnywhere, Replicated)
	ENF_WeaponType NF_WeaponType = ENF_WeaponType::Pistol;
	
	
	
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
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "NF_Player|Equipment")
	TObjectPtr<UNF_WeaponComponent> NF_WeaponComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "NF_Player|Equipment")
	TObjectPtr<UNF_ArmorComponent> NF_ArmorComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "NF_Player|Life")
	TObjectPtr<UNF_PlayerLifeComponent> NF_PlayerLifeComponent;
	
	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "NF_Player|Life")
	TObjectPtr<UNF_ConsumableComponent> NF_ConsumableComponent;
	
	
	
private:
	void ConfigBindDelegate();
	void ConfigData();
	
	void ConfigStatusAttribute();
	void ConfigLevelAttribute();
	
	void GainExpToKiller();
	
	FName MeleeAttackSocket = "MeleeAttackSocket";
	float InteractionRange = 0.0f;
	
#pragma region TryAction
private:
	void TryMeleeAttack();
	void TryStartFire();
	void TryStopFire();
	void TryZoom(ENF_PlayerActionType ActionType);
	void TryReload();
	void TrySkillActionWithInputID(int32 InputID);
	void TryInteraction(APlayerController* SendController);
	void TryUseItem();
	
	UFUNCTION(Server, Reliable)
	void ServerRPC_TryInteraction(APlayerController* SendController, AActor* TargetActor);
	
	FHitResult GetActorFromLineTrace();
	
	static constexpr int32 SkillButtonA_InputID = 0;
	
	static constexpr int32 SkillButtonB_InputID = 1;
#pragma endregion
	
	
#pragma region BuildShop
private:
	void OnEnablePlayerMovementFromType(ENF_BuildShopUIState BuildShopType);
#pragma endregion 
	
	
#pragma region AbilitySystem
	UFUNCTION()
	void OnOutOfHealthCpp();
	
	UFUNCTION()
	void OnOutOfShieldCpp();
	
	UFUNCTION()
	void OnLevelUpCpp(int32 NewLevel);
	
	void InitAbilitySystem();
#pragma endregion
};
