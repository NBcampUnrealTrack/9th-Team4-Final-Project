#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "UI/Data/NF_ViewModelContext.h"
#include "NF_ViewModelBase.generated.h"


// 초기화, 델리게이트 해제 등 공통 생명주기 관련 함수를 담은 부모
UCLASS(Abstract, BlueprintType)
class PROJECT_NF_API UNF_ViewModelBase : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void InitializeViewModel(const FNF_ViewModelContext& NewContext);
	void UpdateViewModelContext(const FNF_ViewModelContext& NewContext);
	void DeinitializeViewModel();

	bool IsViewModelInitialized() const
	{
		return bIsInitialized;
	}

protected:
	virtual void OnInitializeViewModel() {}
	virtual void OnViewModelContextChanged() {}
	virtual void OnDeinitializeViewModel() {}

	const FNF_ViewModelContext& GetViewModelContext() const
	{
		return ViewModelContext;
	}

private:
	FNF_ViewModelContext ViewModelContext;
	bool bIsInitialized = false;
};