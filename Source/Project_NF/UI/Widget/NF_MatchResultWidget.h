#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Struct/NF_MatchResult.h"
#include "NF_MatchResultWidget.generated.h"

class UButton;
class UNF_MatchResultViewModel;
class UNF_ViewModelSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnMatchResultReturnRequested);

/** 결과 WBP의 부모. Manual MVVM 슬롯 ViewModel과 ReturnButton이 필요하다. */
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_MatchResultWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 생성 직후 또는 PushScreen 이후 호출 가능. 팀 정보 도착 시 다시 적용한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|MatchResult")
	void ApplyResult(const FNF_MatchResult& Result, int32 LocalTeamId);

	// 요청 거절 또는 비동기 실패 시 false로 복구한다.
	UFUNCTION(BlueprintCallable, Category = "NF|UI|MatchResult")
	void SetReturnPending(bool bPending);

	UFUNCTION(BlueprintPure, Category = "NF|UI|MatchResult")
	UNF_MatchResultViewModel* GetResultViewModel() const { return ResultViewModel.Get(); }

	// 외부에서 매칭 복귀 처리에 연결한다. 이 위젯은 이동/세션 정리를 수행하지 않는다.
	UPROPERTY(BlueprintAssignable, Category = "NF|UI|MatchResult")
	FNF_OnMatchResultReturnRequested OnReturnRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReturnButton;

private:
	void EnsureViewModel();
	void AssignViewModel(bool bAssign);

	UFUNCTION()
	void HandleReturnClicked();

	UPROPERTY(Transient)
	TObjectPtr<UNF_MatchResultViewModel> ResultViewModel;

	TWeakObjectPtr<UNF_ViewModelSubsystem> ViewModelSubsystem;
};
