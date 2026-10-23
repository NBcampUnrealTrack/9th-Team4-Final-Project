#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_SkillSlotWidget.generated.h"

// 액티브/패시브 스킬 슬롯 WBP의 공통 부모
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_SkillSlotWidget : public UUserWidget
{
	GENERATED_BODY()
};
