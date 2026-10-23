#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_AnnouncementWidget.generated.h"

class UTextBlock;

/** Short-lived, non-interactive announcement shown to one local player. */
UCLASS()
class PROJECT_NF_API UNF_AnnouncementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMessage(const FText& Message);

protected:
	virtual void NativeOnInitialized() override;

private:
	void BuildWidgetTree();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;
};
