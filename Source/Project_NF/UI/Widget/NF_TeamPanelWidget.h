#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotificationId.h"
#include "NF_TeamPanelWidget.generated.h"

class UVerticalBox;
class UNF_TeamMemberWidget;
class UNF_TeamMemberViewModel;
class UNF_TeamPanelViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_TeamPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// HUD에서 지정한다. nullptr를 전달하면 구독과 행을 모두 해제한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void SetTeamPanelViewModel(UNF_TeamPanelViewModel* NewViewModel);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_Members;

	// WBP의 Class Defaults에서 WBP_NF_TeamMember를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "NF|UI|Team")
	TSubclassOf<UNF_TeamMemberWidget> MemberWidgetClass;

private:
	void UnbindViewModel();
	void HandleMembersChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);
	void SynchronizeMembers();
	void CreateMemberWidget(UNF_TeamMemberViewModel* MemberVM);
	bool AssignMemberViewModel(UNF_TeamMemberWidget* Widget, UNF_TeamMemberViewModel* MemberVM);
	void ClearMemberWidgets();

	UPROPERTY(Transient)
	TObjectPtr<UNF_TeamPanelViewModel> TeamPanelViewModel;

	UPROPERTY(Transient)
	TMap<TObjectPtr<UNF_TeamMemberViewModel>, TObjectPtr<UNF_TeamMemberWidget>> MemberWidgets;

	FDelegateHandle MembersChangedHandle;
	UE::FieldNotification::FFieldId MembersFieldId;
};
