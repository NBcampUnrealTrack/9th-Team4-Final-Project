#pragma once

#include "CoreMinimal.h"

class ANF_PlayerState;

// @Owner: 황준현 | 2026-10-19
// Core의 기본 빌드 요청 계약. 적용과 Ready 갱신을 완료한 경우에만 true를 반환한다.
DECLARE_DELEGATE_RetVal_OneParam(bool, FNF_ApplyFallbackBuild, ANF_PlayerState*);
