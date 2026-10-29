#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_BuildShopScreen.generated.h"

class UNF_BuildShopViewModel;

// @Owner: 황준현 | 2026-10-29. Model을 탐색하지 않는 빌드 상점 View.
UCLASS()
class PROJECT_NF_API UNF_BuildShopScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	bool SetShopViewModel(UNF_BuildShopViewModel* InViewModel);

	UFUNCTION(BlueprintPure, Category = "NF|Build|UI")
	UNF_BuildShopViewModel* GetShopViewModel() const { return ShopViewModel; }

protected:
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|UI")
	FName ViewModelSlotName = TEXT("BuildShopVM");

	UPROPERTY(Transient, BlueprintReadOnly, Category = "NF|Build|UI")
	TObjectPtr<UNF_BuildShopViewModel> ShopViewModel;
};
