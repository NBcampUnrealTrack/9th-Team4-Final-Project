#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "NF_ArmorComponentConfigDataAsset.generated.h"

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_ArmorComponentConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Armor")
	TArray<TSubclassOf<class ANF_ArmorBase>> DefaultArmorClasses;

	// 메시 없이 논리적 장착 허용 여부 (외형 장착 여부)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sockets")
	bool bAttachArmorToSockets = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sockets", meta=(EditCondition="bAttachArmorToSockets"))
	FName HeadSocketName = TEXT("head");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sockets", meta=(EditCondition="bAttachArmorToSockets"))
	FName UpperSocketName = TEXT("spine_03");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sockets", meta=(EditCondition="bAttachArmorToSockets"))
	FName BottomSocketName = TEXT("pelvis");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit Regions")
	TMap<FName, ENF_ArmorSlot> BoneToSlot;

	// 피격을 담당할 메시 -> VisualMesh
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Hit Regions")
	FName HitMeshComponentName;
};
