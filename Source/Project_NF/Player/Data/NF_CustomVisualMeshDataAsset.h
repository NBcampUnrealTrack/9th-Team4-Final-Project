#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_CustomVisualMeshDataAsset.generated.h"

UCLASS()
class PROJECT_NF_API UNF_CustomVisualMeshDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USkeletalMesh> VisualSkeletalMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAnimInstance> VisualAnimClass;
	
	UPROPERTY(EditDefaultsOnly)
	FVector VisualMeshScale = FVector(1.0f, 1.0f, 1.0f);
};
