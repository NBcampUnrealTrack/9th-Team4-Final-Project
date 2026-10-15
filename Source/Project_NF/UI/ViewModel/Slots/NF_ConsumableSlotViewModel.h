
#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_ConsumableSlotViewModel.generated.h"

class UTexture2D;

// 소모품 시스템이 뷰모델에 전달할 UI용 데이터. 별도 뷰모델이 아님.
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

	// TODO: 소모품 담당 시스템에서 사용 가능 상태를 전달.
	// 쿨타임, 사용 불가 상태 태그, 사용 조건 등을 그쪽에서 판단.
	UPROPERTY(BlueprintReadWrite)
	bool bUsageAllowed = false;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_ConsumableSlotViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	// TODO: 소모품 시스템의 선택/수량/사용 가능 상태 변경 알림에서 호출.
	// TODO: 단축키 문구는 실제 입력 매핑이 확정되면 함께 전달.
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