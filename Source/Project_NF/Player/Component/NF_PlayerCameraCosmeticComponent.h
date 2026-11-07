#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_PlayerCameraCosmeticComponent.generated.h"

class UCurveFloat;
class USpringArmComponent;
class UMotionMatchingComponent;
class UNF_PlayerCosmeticDataAsset;

UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_PlayerCameraCosmeticComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_PlayerCameraCosmeticComponent();
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF_Cosmetic")
	TObjectPtr<UNF_PlayerCosmeticDataAsset> NF_PlayerCosmeticDataAsset;

private:
	void ConfigData();
	
	void UpdateSprintCameraArmOffset(float DeltaTime);
	
	void UpdateCrouchCameraArmOffset(float DeltaTime);
	
	void UpdateCameraArmLength(float DeltaTime);
	
	UPROPERTY()
	TObjectPtr<USpringArmComponent> SpringArmComponent;
	
	UPROPERTY()
	TObjectPtr<UMotionMatchingComponent> MotionMatchingComponent;
	
	float DefaultCameraArmLength = 200.0f;
	
	float CameraCollisionArmLength = 200.0f;
	
	UPROPERTY()
	TObjectPtr<UCurveFloat> SprintCameraArmCurve;
	
	UPROPERTY()
	TObjectPtr<UCurveFloat> CrouchCameraArmCurve;
	
	bool bIsSprintCameraEnabled = true;
	
	bool bIsCrouchCameraEnabled = true;
	
	float CameraCollisionNearSpeed = 30.0f;
	
	float CameraCollisionFarSpeed = 5.0f;
	
	float SprintCameraArmOffset = 0.0f;
	
	float CrouchCameraElapsedTime = 0.0f;
	
	float CrouchCameraArmOffset = 0.0f;
	
	FVector DefaultTargetOffset = FVector::ZeroVector;
	
	float CrouchCapsuleHeightGap = 0.0f;
};
