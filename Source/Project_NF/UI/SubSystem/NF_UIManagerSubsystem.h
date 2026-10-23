#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "NF_UIManagerSubsystem.generated.h"

class APlayerController;
class UNF_AnnouncementWidget;

UENUM(BlueprintType)
enum class ENF_UIInputMode : uint8
{
	GameOnly,
	GameAndUI,
	UIOnly
};

USTRUCT()
struct FNFManagedWidgetEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Widget;

	UPROPERTY(Transient)
	ENF_UIInputMode InputMode = ENF_UIInputMode::GameAndUI;

	UPROPERTY(Transient)
	bool bShowMouseCursor = true;
};

USTRUCT()
struct FNFManagedAnnouncementEntry
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UNF_AnnouncementWidget> Widget;

	FTimerHandle LifetimeTimerHandle;
};

/** Owns viewport/modal UI and input mode for one local player. */
UCLASS()
class PROJECT_NF_API UNF_UIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	UUserWidget* PushWidget(TSubclassOf<UUserWidget> WidgetClass,
		ENF_UIInputMode InputMode = ENF_UIInputMode::GameAndUI,
		bool bShowMouseCursor = true, int32 ZOrder = 0);

	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	bool RemoveWidget(UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	bool PopTopWidget();

	UFUNCTION(BlueprintCallable, Category = "NF|UI")
	void PopAllWidgets();

	UFUNCTION(BlueprintPure, Category = "NF|UI")
	UUserWidget* GetTopWidget() const;

	/** Shows a brief non-modal message for this local player. */
	void ShowAnnouncement(const FText& Message, float Duration = 3.0f);

	void RefreshInputMode();

private:
	APlayerController* GetOwningPlayerController() const;
	void ApplyInputModeForTopWidget();
	void ClearAnnouncements();

	UFUNCTION()
	void HandleAnnouncementExpired(UNF_AnnouncementWidget* Widget);

	UPROPERTY(Transient)
	TArray<FNFManagedWidgetEntry> WidgetStack;

	UPROPERTY(Transient)
	TArray<FNFManagedAnnouncementEntry> ActiveAnnouncements;

	bool bHasSavedMouseCursor = false;
	bool bSavedMouseCursor = false;
};
