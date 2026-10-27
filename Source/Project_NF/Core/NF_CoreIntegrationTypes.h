#pragma once

#include "CoreMinimal.h"
	
class APawn;
class ANF_PlayerState;
class ANF_BossBase;
struct FNF_BossDefeatResult;
struct FNF_MatchResult;

// @owner : 조민웅 | 2026-10-27
// 목적: Core 리스폰 시스템이 플레이어의 현재 리스폰 허용 여부를 GameMode에 요청한다.
DECLARE_DELEGATE_RetVal_OneParam(bool,FNF_EvaluateRespawnPermission,const ANF_PlayerState*);

// @Owner: 조민웅 | 2026-10-19
// 매치메이킹 연동 코드가 서버에서 바인딩. 남은 클라이언트의 복귀 및 세션 정리를 요청
DECLARE_DELEGATE_OneParam(FNF_RequestMatchAbort, const FString&);

// @Owner: 황준현 | 2026-10-19
// Core의 기본 빌드 요청 계약. 적용과 Ready 갱신을 완료한 경우에만 true를 반환한다.
DECLARE_DELEGATE_RetVal_OneParam(bool, FNF_ApplyFallbackBuild, ANF_PlayerState*);

// @Owner: 조민웅 | 2026-10-19
// Core의 보스 출현 요청 계약.서버에서 생성·GAS·AI 초기화 완료 후 보스를 반환한다. 실패 시 부분 생성 객체를 정리하고 nullptr를 반환한다.
DECLARE_DELEGATE_RetVal(ANF_BossBase*,FNF_RequestBossSpawn);

// @Owner: 조민웅 | 2026-10-20
// Core가 보스 처치 결과를 검증하고 최종 확정했을 때 발생한다.
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnBossDefeatResolved,const FNF_BossDefeatResult&)


// @owner : 조민웅 | 2026-10-21
// 목적: 매치 참가자 이탈을 관련 시스템에 통보한다.
DECLARE_MULTICAST_DELEGATE_TwoParams(FNF_OnMatchPlayerLeaving,ANF_PlayerState*,APawn*);

// @owner : 조민웅 | 2026-10-21
// 목적: 매치 종료·중단·월드 종료에 따른 정리를 통보한다.
DECLARE_MULTICAST_DELEGATE(FNF_OnMatchCleanup);

// @owner : 조민웅 | 2026-10-21
// 목적: 결과 표시 후 정상 승리 매치의 복귀·세션 정리를 요청한다.
DECLARE_DELEGATE_OneParam(FNF_RequestMatchCompleted,const FNF_MatchResult&);
