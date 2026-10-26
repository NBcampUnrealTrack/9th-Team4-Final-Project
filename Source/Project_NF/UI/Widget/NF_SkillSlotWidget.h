#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_SkillSlotWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;

// 액티브/패시브 스킬 슬롯 WBP의 공통 부모
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_SkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "NF|UI|Skill")
	void SetSkill(FText SkillName, UTexture2D* Icon, FText Shortcut, bool bPassive);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Skill")
	void ClearSkill();

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Skill")
	void SetCooldown(float RemainingSeconds, float DurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "NF|UI|Skill")
	void SetCanUse(bool bCanUse);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IMG_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_SkillName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Shortcut;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TXT_Cooldown;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> PB_Cooldown;

private:
	bool bIsPassive = false;
	bool bIsEquipped = false;
};
