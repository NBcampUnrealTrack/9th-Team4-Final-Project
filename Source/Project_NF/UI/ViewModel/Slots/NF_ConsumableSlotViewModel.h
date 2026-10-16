
#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_ConsumableSlotViewModel.generated.h"

class UTexture2D;

// 소모품 시스템이 뷰모델에 전달할 UI용 데이터.
USTRUCT(BlueprintType)
struct FNF_ConsumableSlotViewData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FName ItemId = NAME_None;

	UPROPERTY(BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(BlueprintReadWrite)
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadWrite)
	FText ShortcutText;
	
	// 쿨타임, 사용 불가 상태 태그, 사용 조건 등을 소모품 담당 시스템에서 판단 후전달.
	UPROPERTY(BlueprintReadWrite)
	bool bUsageAllowed = false;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_ConsumableSlotViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	void UpdateSlot(const FNF_ConsumableSlotViewData& NewData);

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Consumable",meta = (AllowPrivateAccess = "true"))
	FName SelectedItemId;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Consumable",meta = (AllowPrivateAccess = "true"))
	FText SelectedItemName;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Consumable",meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> SelectedItemIcon;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Consumable",meta = (AllowPrivateAccess = "true"))
	int32 Quantity = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Consumable",meta = (AllowPrivateAccess = "true"))
	FText ShortcutText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "Consumable",meta = (AllowPrivateAccess = "true"))
	bool bCanUse = false;
};