#pragma once

#include "CoreMinimal.h"

class ANF_BossBase;

/** Synchronous Core request for the boss system to prepare and return the match boss. */
DECLARE_DELEGATE_RetVal(ANF_BossBase*, FNF_RequestBossSpawn);
