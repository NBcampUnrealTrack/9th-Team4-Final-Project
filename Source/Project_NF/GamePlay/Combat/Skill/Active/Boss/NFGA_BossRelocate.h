#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossHomingVolley.h"
#include "NFGA_BossRelocate.generated.h"

/** Compatibility class for saved StateTrees/BPs. Executes HomingVolley; no boss movement. */
UCLASS(Blueprintable, meta = (DisplayName = "Boss Homing Volley (Legacy Relocate)"))
class PROJECT_NF_API UNFGA_BossRelocate : public UNFGA_BossHomingVolley
{
	GENERATED_BODY()
};
