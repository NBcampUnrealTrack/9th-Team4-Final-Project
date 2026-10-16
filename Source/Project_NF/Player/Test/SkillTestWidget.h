// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SkillTestWidget.generated.h"

class UImage;
class UTexture2D;
enum class ENF_PlayerActionType : uint8;

/**
 * 스킬 슬롯 A/B 빈칸 2개. 상점이 열린 상태에서 스킬 버튼을 누르면 해당 칸에 이미지를 넣는다.
 */
UCLASS()
class PROJECT_NF_API USkillTestWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void OnPlayerBindingSkill(APlayerController* SendController, ENF_PlayerActionType PlayerActionType);

	UFUNCTION(BlueprintCallable, Category = "SkillSlot")
	void SetSkillSlotImage(UImage* SkillSlotImage, UTexture2D* SkillTexture);

protected:
	// WBP에서 같은 이름의 Image 위젯을 만들어야 한다
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillSlotImage_A;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SkillSlotImage_B;

	// WBP 디테일 패널에서 넣을 이미지를 고른다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillSlot")
	TObjectPtr<UTexture2D> SkillTexture_A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillSlot")
	TObjectPtr<UTexture2D> SkillTexture_B;
};
