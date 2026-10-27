#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "NF_TeamPlayerStart.generated.h"

// @owner : 조민웅 | 2026-10-27
// 목적: 팀별 플레이어 리스폰 위치를 제공한다.
UCLASS()
class PROJECT_NF_API ANF_TeamPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	int32 GetTeamId() const
	{
		return TeamId;
	}

private:
	UPROPERTY(EditInstanceOnly, Category = "NF|Respawn", meta = (ClampMin = "1", ClampMax = "4"))
	int32 TeamId = 1;
};
