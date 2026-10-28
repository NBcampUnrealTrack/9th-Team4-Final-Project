#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_OptionsWidget.generated.h"

class UNF_OptionsViewModel;

/** Parent of the designer-authored WBP. Does not construct a widget tree. */
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_OptionsWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="NF|Options") void Close();
	UFUNCTION(BlueprintPure, Category="NF|Options") UNF_OptionsViewModel* GetOptionsViewModel() const { return OptionsViewModel; }
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	/** Called after the manual MVVM source is assigned. Populate combo options here if needed. */
	UFUNCTION(BlueprintImplementableEvent, Category="NF|Options") void OnOptionsViewModelReady();
	UPROPERTY(EditDefaultsOnly, Category="NF|Options") FName ViewModelName = TEXT("OptionsViewModel");
private:
	UPROPERTY(Transient) TObjectPtr<UNF_OptionsViewModel> OptionsViewModel;
};
