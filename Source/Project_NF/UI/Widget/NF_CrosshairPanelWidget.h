#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_CrosshairPanelWidget.generated.h"

class USizeBox;
class UNF_CrosshairViewModel;

// 조준선/스코프 인스턴스의 수명과 발사 피드백만 관리한다.
// 표시 클래스 바인딩과 배치는 자식 WBP에서 설정한다.
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_CrosshairPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Crosshair")
	void SetCrosshairClass(TSubclassOf<UUserWidget> InClass);

	// 상태 FieldNotify는 구독하지 않고 일회성 발사 이벤트만 연결한다.
	void SetCrosshairViewModel(UNF_CrosshairViewModel* NewViewModel);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> CrosshairSlot;

private:
	void HandleShotFeedback();

	UPROPERTY(Transient)
	TObjectPtr<UNF_CrosshairViewModel> CrosshairViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveCrosshairWidget;

	FDelegateHandle ShotFeedbackHandle;
};
