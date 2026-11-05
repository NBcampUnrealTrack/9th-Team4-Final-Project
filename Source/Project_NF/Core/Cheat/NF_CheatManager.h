#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "NF_CheatManager.generated.h"

// @owner : 조민웅 | 2026-10-29
// 목적: PIE 콘솔 명령을 테스트 컨트롤러의 서버 요청으로 전달한다.

/*
EnableCheats
NF_PrintMatchState
NF_TestEscape
 */
 
UCLASS()
class PROJECT_NF_API UNF_CheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// 명령을 실행한 플레이어에게 열쇠를 지급하고 Escape로 전환한다.
	UFUNCTION(Exec)
	void NF_TestEscape();

	// 서버의 페이즈, 열쇠, 플레이어별 팀 및 GAS 준비 상태를 출력한다.
	UFUNCTION(Exec)
	void NF_PrintMatchState();
};
