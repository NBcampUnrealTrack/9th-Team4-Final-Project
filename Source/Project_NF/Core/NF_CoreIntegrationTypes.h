#pragma once

#include "CoreMinimal.h"

class ANF_PlayerState;
class ANF_BossBase;

// @Owner: 황준현 | 2026-10-19
// Core의 기본 빌드 요청 계약. 적용과 Ready 갱신을 완료한 경우에만 true를 반환한다.
DECLARE_DELEGATE_RetVal_OneParam(bool, FNF_ApplyFallbackBuild, ANF_PlayerState*);

// @Owner: 조민웅 | 2026-10-19
// Core의 보스 출현 요청 계약.서버에서 생성·GAS·AI 초기화 완료 후 보스를 반환한다. 실패 시 부분 생성 객체를 정리하고 nullptr를 반환한다.
DECLARE_DELEGATE_RetVal(ANF_BossBase*,FNF_RequestBossSpawn);

