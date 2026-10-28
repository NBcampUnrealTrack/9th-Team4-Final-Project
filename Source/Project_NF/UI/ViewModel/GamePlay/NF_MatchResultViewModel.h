#pragma once

#include "CoreMinimal.h"
#include "Data/Struct/NF_MatchResult.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_MatchResultViewModel.generated.h"

/** 서버에서 확정한 결과를 로컬 팀 기준의 표시 데이터로 변환한다. */
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_MatchResultViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	void ApplyResult(const FNF_MatchResult& Result, int32 LocalTeamId);
	void SetReturnPending(bool bPending);

	UFUNCTION(BlueprintPure, Category = "NF|UI|MatchResult")
	FText GetResultText() const { return ResultText; }

	UFUNCTION(BlueprintPure, Category = "NF|UI|MatchResult")
	bool GetCanReturn() const { return bCanReturn; }

private:
	void RefreshCanReturn();

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|MatchResult", meta = (AllowPrivateAccess = "true"))
	FText ResultText;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|MatchResult", meta = (AllowPrivateAccess = "true"))
	bool bCanReturn = false;

	bool bHasResult = false;
	bool bReturnPending = false;
};
