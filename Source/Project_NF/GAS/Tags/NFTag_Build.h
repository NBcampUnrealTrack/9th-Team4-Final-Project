#pragma once

#include "NativeGameplayTags.h"

namespace NFTag_Build
{
	// @Owner: 준현 | 2026-10-11 | Match/survival code can block build changes.
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Build_Locked);

	// @Owner: 민웅 | 2026-10-16 
	PROJECT_NF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Build_Ready);
} // namespace NFTag_Build
