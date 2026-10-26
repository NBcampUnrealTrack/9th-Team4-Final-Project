#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_PlayerStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_PlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NF|UI|PlayerStatus")
	void SetHealth(float Current, float Maximum);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|PlayerStatus")
	void SetShield(float Current, float Maximum);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|PlayerStatus")
	void SetLevel(int32 NewLevel);

	// Current: 현재 레벨에서 획득한 경험치
	// Required: 다음 레벨까지 필요한 경험치
	UFUNCTION(BlueprintCallable, Category = "NF|UI|PlayerStatus")
	void SetExperience(float Current, float Required);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Health;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Shield;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Experience;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Health;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Shield;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Level;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Experience;
};