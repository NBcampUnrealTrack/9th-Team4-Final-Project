#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_PlayerStatusWidget.generated.h"

class UProgressBar;

// 플레이어 상태 WBP의 부모. 표시값은 MVVM 바인딩으로 연결
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_PlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// WBP의 View Binding에서 호출할 목적지 함수
	UFUNCTION(BlueprintCallable, Category = "NF|UI|PlayerStatus")
	void SetHealthPercent(float NewPercent);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Health;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_HealthTrail;

	// 실제 애니메이션은 WBP에서 구현
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|UI|PlayerStatus")
	void BP_PlayDamageFeedback();

	UFUNCTION(BlueprintImplementableEvent, Category = "NF|UI|PlayerStatus")
	void BP_PlayHealFeedback();

	UPROPERTY(EditDefaultsOnly, Category = "NF|UI|PlayerStatus", meta = (ClampMin = "0.0"))
	float TrailDelay = 0.25f;

	// 초당 감소할 체력 비율
	UPROPERTY(EditDefaultsOnly, Category = "NF|UI|PlayerStatus", meta = (ClampMin = "0.01"))
	float TrailSpeed = 0.6f;

private:
	bool bHasInitialHealth = false;
	float TargetPercent = 0.0f;
	float TrailPercent = 0.0f;
	float TrailDelayRemaining = 0.0f;
};
