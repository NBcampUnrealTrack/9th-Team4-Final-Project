#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_TeamPanelWidget.generated.h"

class APlayerState;
class UVerticalBox;
class UNF_TeamMemberWidget;

UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_TeamPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 호출자가 본인 제외 및 같은 팀 여부를 확인한 뒤 전달한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	UNF_TeamMemberWidget* AddMember(APlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	UNF_TeamMemberWidget* FindMember(APlayerState* PlayerState) const;

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void RemoveMember(APlayerState* PlayerState);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void ClearMembers();

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VB_Members;

	// WBP의 Class Defaults에서 WBP_NF_TeamMember를 지정한다.
	UPROPERTY(EditDefaultsOnly, Category = "NF|UI|Team")
	TSubclassOf<UNF_TeamMemberWidget> MemberWidgetClass;

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<APlayerState>, TObjectPtr<UNF_TeamMemberWidget>> MemberWidgets;
};
