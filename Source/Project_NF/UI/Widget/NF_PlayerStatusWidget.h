#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_PlayerStatusWidget.generated.h"

// 플레이어 상태 WBP의 부모. 표시값은 MVVM 바인딩으로 연결
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_PlayerStatusWidget : public UUserWidget
{
	GENERATED_BODY()
};
