#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotificationId.h"
#include "NF_MonsterStatusWidget.generated.h"

class ANF_Monster;
class UProgressBar;
class UTextBlock;
class UNF_MonsterStatusViewModel;

/** A world-space monster label. Each instance observes exactly one monster. */
UCLASS(Blueprintable)
class PROJECT_NF_API UNF_MonsterStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ObserveMonster(ANF_Monster* Monster);
	void RefreshMonsterName();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Optional Blueprint widgets. A native name/bar layout is created when absent. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TXT_MonsterName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> PB_Health;

private:
	void BindObservedMonster();
	void EnsureNativeLayout();
	void RefreshDisplay();
	void HandleViewModelChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);
	void UnbindViewModel();

	UPROPERTY(Transient)
	TObjectPtr<UNF_MonsterStatusViewModel> ViewModel;

	TWeakObjectPtr<ANF_Monster> ObservedMonster;

	UE::FieldNotification::FFieldId HealthFieldId;
	UE::FieldNotification::FFieldId NameFieldId;
	FDelegateHandle HealthChangedHandle;
	FDelegateHandle NameChangedHandle;
};
