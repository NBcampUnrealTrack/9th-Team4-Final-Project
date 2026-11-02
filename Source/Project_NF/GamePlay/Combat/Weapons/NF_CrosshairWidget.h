#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_CrosshairWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_CrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void SetMaxRadius(float InValue);

	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void UpdateRadius(float InValue);

	UFUNCTION(BlueprintCallable, Category="Crosshair")
	void NotifyShotFired();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Crosshair")
	TObjectPtr<UWidget> CrosshairTop;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Crosshair")
	TObjectPtr<UWidget> CrosshairBottom;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Crosshair")
	TObjectPtr<UWidget> CrosshairLeft;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget), Category="Crosshair")
	TObjectPtr<UWidget> CrosshairRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta=(ClampMin="0.0", UIMin="0.0"))
	float CenterGap = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta=(ClampMin="0.0", UIMin="0.0"))
	float MaxRadius = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta=(ClampMin="0.0", ClampMax="1.0", UIMin="0.0", UIMax="1.0"))
	float Radius = 0.f;

	/** Normalized spread added for each successful shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SpreadPerShot = 0.35f;

	/** Normalized spread recovered per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crosshair", meta=(ClampMin="0.01"))
	float SpreadRecoverySpeed = 1.5f;

private:
	void UpdateCrosshairLayout();
	void UpdateCrosshairVisibility();
};
