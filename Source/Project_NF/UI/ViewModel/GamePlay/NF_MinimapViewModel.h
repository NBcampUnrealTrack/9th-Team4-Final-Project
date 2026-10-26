#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "UI/Data/NF_MinimapSnapshot.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_MinimapViewModel.generated.h"

class UWorld;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_MinimapViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

private:
	void StartRefresh();
	void StopRefresh();
	void RefreshSnapshot();

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Minimap",meta = (AllowPrivateAccess = "true"))
	FNF_MinimapSnapshot Snapshot;

	TWeakObjectPtr<UWorld> RefreshWorld;
	FTimerHandle RefreshTimerHandle;
};