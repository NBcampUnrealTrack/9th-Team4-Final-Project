#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypes.h"
#include "Components/Button.h"
#include "Framework/Commands/InputChord.h"
#include "NF_BuildShopWidget.generated.h"

class UInputKeySelector;
class UScrollBox;
class UTextBlock;
class UNF_BuildComponent;
class UNF_BuildInputComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNF_OnBuildItemClicked, FName, ItemId, bool, bRemove);

/** A parameterized button avoids changing a shared selected item in per-button callbacks. */
UCLASS()
class PROJECT_NF_API UNF_BuildItemButton : public UButton
{
	GENERATED_BODY()

public:
	void Configure(FName InItemId, bool bInRemove);
	FNF_OnBuildItemClicked OnItemClicked;

private:
	UFUNCTION()
	void HandleClicked();
	FName ItemId;
	bool bRemove = false;
};

/** Native prototype UI; project UI can use the component's public draft/session APIs instead. */
UCLASS()
class PROJECT_NF_API UNF_BuildShopWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
	UFUNCTION()
	void Refresh();
	UFUNCTION()
	void HandleItemClicked(FName ItemId, bool bRemove);
	UFUNCTION()
	void HandleSlotOne();
	UFUNCTION()
	void HandleSlotTwo();
	UFUNCTION()
	void HandleApply();
	UFUNCTION()
	void HandleClose();
	UFUNCTION()
	void HandleSlotOneKey(FInputChord Chord);
	UFUNCTION()
	void HandleSlotTwoKey(FInputChord Chord);
	UFUNCTION()
	void HandleRequestCompleted(const FNF_BuildValidation& Validation);

	void ChangeKey(int32 SlotIndex, const FInputChord& Chord);
	void AddItemButton(UScrollBox* Container, FName ItemId, const FText& Label, bool bRemove);

	UPROPERTY(Transient)
	TObjectPtr<UNF_BuildComponent> Build;
	UPROPERTY(Transient)
	TObjectPtr<UNF_BuildInputComponent> BuildInput;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResultText;
	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> CatalogList;
	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> DraftList;
	UPROPERTY(Transient)
	TObjectPtr<UButton> SlotOneButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> SlotTwoButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ApplyButton;
	UPROPERTY(Transient)
	TObjectPtr<UInputKeySelector> SlotOneKeySelector;
	UPROPERTY(Transient)
	TObjectPtr<UInputKeySelector> SlotTwoKeySelector;
	bool bRefreshingKeys = false;
};
