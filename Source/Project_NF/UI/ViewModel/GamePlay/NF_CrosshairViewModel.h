#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_CrosshairViewModel.generated.h"

class ANF_WeaponBase;
class UNF_WeaponComponent;
class UUserWidget;

// 저장할 상태가 아닌 매 발의 UI 연출 요청
DECLARE_MULTICAST_DELEGATE(FNF_OnCrosshairShotFeedbackRequested);

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_CrosshairViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	TSubclassOf<UUserWidget> GetActiveCrosshairClass() const {return ActiveCrosshairClass;}

	// 크로스헤어 패널이 구독하고 현재 표시 중인 위젯에 연출을 전달한다.
	FNF_OnCrosshairShotFeedbackRequested OnShotFeedbackRequested;

protected:
	virtual void OnInitializeViewModel() override;
	virtual void OnViewModelContextChanged() override;
	virtual void OnDeinitializeViewModel() override;

private:
	void BindWeaponComponent();
	void UnbindWeaponComponent();
	TSubclassOf<UUserWidget> ResolveCrosshairClass() const;
	void RefreshCrosshair();

	void HandleEquippedWeaponChanged(ANF_WeaponBase* NewWeapon);
	void HandleZoomActiveChanged(bool bZoomActive);
	void HandleLocalShotFired(ANF_WeaponBase* FiredWeapon);

	UPROPERTY(BlueprintReadOnly, FieldNotify, Category = "NF|UI|Crosshair", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> ActiveCrosshairClass;

	TWeakObjectPtr<UNF_WeaponComponent> BoundWeaponComponent;

	FDelegateHandle EquippedWeaponChangedHandle;
	FDelegateHandle ZoomActiveChangedHandle;
	FDelegateHandle LocalShotFiredHandle;
};
