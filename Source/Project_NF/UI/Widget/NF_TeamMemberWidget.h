#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_TeamMemberWidget.generated.h"

class UProgressBar;
class UTextBlock;

UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_TeamMemberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void SetMemberName(FText NewName);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void SetMemberLevel(int32 NewLevel);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void SetHealth(float Current, float Maximum);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Team")
	void SetShield(float Current, float Maximum);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_PlayerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Level;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Health;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Shield;
};
