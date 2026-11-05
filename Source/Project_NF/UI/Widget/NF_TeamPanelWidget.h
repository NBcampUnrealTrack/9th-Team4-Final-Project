#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_TeamPanelWidget.generated.h"

class UVerticalBox;
class UNF_TeamMemberWidget;
class UNF_TeamMemberViewModel;

UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_TeamPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// WBP의 ViewModel.Members를 바인딩한다. 빈 배열은 모든 행을 제거한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void SetMembers(const TArray<UNF_TeamMemberViewModel*>& InMembers);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_Members;

	// WBP의 Class Defaults에서 WBP_NF_TeamMember를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "NF|UI|Team")
	TSubclassOf<UNF_TeamMemberWidget> MemberWidgetClass;

private:
	void CreateMemberWidget(UNF_TeamMemberViewModel* MemberVM);
	bool AssignMemberViewModel(UNF_TeamMemberWidget* Widget, UNF_TeamMemberViewModel* MemberVM);
	void ClearMemberWidgets();

	UPROPERTY(Transient)
	TMap<TObjectPtr<UNF_TeamMemberViewModel>, TObjectPtr<UNF_TeamMemberWidget>> MemberWidgets;

};
