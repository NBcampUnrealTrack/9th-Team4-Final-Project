#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_TeamPanelViewModel.generated.h"

class APlayerState;
class UNF_TeamMemberViewModel;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_TeamPanelViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	const TArray<TObjectPtr<UNF_TeamMemberViewModel>>& GetMembers() const{return Members;}

	// 초기화 시 및 입장/퇴장/팀 변경/이름 변경/ASC 준비 알림에서 호출한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void RefreshMembers();

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void ClearMembers();

	UFUNCTION(BlueprintPure, Category = "NF|UI|Team")
	UNF_TeamMemberViewModel* FindMember(APlayerState* PlayerState) const;

	UFUNCTION(BlueprintPure, FieldNotify, Category = "NF|UI|Team")
	int32 GetMemberCount() const;

protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

private:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Team", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UNF_TeamMemberViewModel>> Members;
};
