#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_MatchStatusWidget.generated.h"

// 게임 페이즈 및 남은 시간을 표시하는 WBP의 부모
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_MatchStatusWidget : public UUserWidget
{
	GENERATED_BODY()
};
