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
	void ShowAnnouncement(const FText& Message, float Duration);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

private:
	void BuildWidgetTree();

	UFUNCTION()
	void HandleLifetimeExpired();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MessageText;

	FTimerHandle LifetimeTimerHandle;
};
