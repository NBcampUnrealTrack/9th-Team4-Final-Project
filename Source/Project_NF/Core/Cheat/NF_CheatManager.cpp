#include "Core/Cheat/NF_CheatManager.h"

#include "Core/Cheat/NF_TestPlayerController.h"

void UNF_CheatManager::NF_TestEscape()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (ANF_TestPlayerController* Controller = Cast<ANF_TestPlayerController>(GetOuterAPlayerController()))
	{
		Controller->ServerRequestEscapeTest();
	}
#endif
}

void UNF_CheatManager::NF_PrintMatchState()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (ANF_TestPlayerController* Controller = Cast<ANF_TestPlayerController>(GetOuterAPlayerController()))
	{
		Controller->ServerRequestMatchDebugState();
	}
#endif
}
