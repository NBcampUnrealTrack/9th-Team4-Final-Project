#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_PlayerCosmeticDataAsset.generated.h"

class UCurveFloat;
enum class ENF_CustomVisualMeshType : uint8;
class UNF_CustomVisualMeshDataAsset;

UCLASS()
class PROJECT_NF_API UNF_PlayerCosmeticDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category = "Cosmetic|Camera", DisplayName = "bIsSprintCameraEnabled (질주 카메라 연출 사용)")
	bool bIsSprintCameraEnabled = true;
	
	UPROPERTY(EditAnywhere, Category = "Cosmetic|Camera", DisplayName = "SprintCameraFarCurve (질주 시 카메라 멀어짐 커브)")
	TObjectPtr<UCurveFloat> SprintCameraFarCurve;
	
	UPROPERTY(EditAnywhere, Category = "Cosmetic|Camera", DisplayName = "bIsCrouchCameraEnabled (앉기 카메라 연출 사용)")
	bool bIsCrouchCameraEnabled = true;
	
	UPROPERTY(EditAnywhere, Category = "Cosmetic|Camera", DisplayName = "CrouchCameraCloserCurve (앉을 때 카메라 가까워짐 커브, X = 초, Y = cm)")
	TObjectPtr<UCurveFloat> CrouchCameraCloserCurve;
	
	UPROPERTY(EditAnywhere, Category = "Cosmetic|Camera", DisplayName = "CameraCollisionNearSpeed (벽에 막혀 가까워질 때 빠르기)")
	float CameraCollisionNearSpeed = 30.0f;
	
	UPROPERTY(EditAnywhere, Category = "Cosmetic|Camera", DisplayName = "CameraCollisionFarSpeed (벽에서 벗어나 멀어질 때 빠르기)")
	float CameraCollisionFarSpeed = 5.0f;
	
	UPROPERTY(EditAnywhere, Category = "Cosmetic|VisualMesh")
	TMap<ENF_CustomVisualMeshType, TObjectPtr<UNF_CustomVisualMeshDataAsset>> CustomVisualMeshMap;
};
