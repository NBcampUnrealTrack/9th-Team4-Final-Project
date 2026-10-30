#include "Core/Cheat/NF_TestPlayerController.h"

#include "Core/Cheat/NF_CheatManager.h"
#include "Core/NF_TestGameMode.h"
#include "Engine/World.h"
#include "Player/State/NF_PlayerState.h"

ANF_TestPlayerController::ANF_TestPlayerController()
{
	CheatClass = UNF_CheatManager::StaticClass();
}

void ANF_TestPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (GetWorld()->WorldType == EWorldType::PIE && IsLocalController())
	{
		EnableCheats();
	}
#endif
}

void ANF_TestPlayerController::ServerRequestEscapeTest_Implementation()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	ANF_TestGameMode* GameMode = GetWorld()->GetAuthGameMode<ANF_TestGameMode>();
	if (GetWorld()->WorldType != EWorldType::PIE || !IsValid(GameMode))
	{
		ClientReportCheatResult(TEXT("NF 치트는 PIE의 NF_TestGameMode에서만 사용할 수 있습니다."));
		return;
	}

	FString Message;
	GameMode->RequestEscapeTest(GetPlayerState<ANF_PlayerState>(), Message);
	ClientReportCheatResult(Message);
#endif
}

void ANF_TestPlayerController::ServerRequestMatchDebugState_Implementation()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const ANF_TestGameMode* GameMode = GetWorld()->GetAuthGameMode<ANF_TestGameMode>();
	if (GetWorld()->WorldType != EWorldType::PIE || !IsValid(GameMode))
	{
		ClientReportCheatResult(TEXT("NF 치트는 PIE의 NF_TestGameMode에서만 사용할 수 있습니다."));
		return;
	}

	ClientReportCheatResult(GameMode->BuildMatchDebugState());
#endif
}

void ANF_TestPlayerController::ClientReportCheatResult_Implementation(const FString& Message)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	TArray<FString> Lines;
	Message.ParseIntoArrayLines(Lines);
	for (const FString& Line : Lines)
	{
		ClientMessage(Line);
		UE_LOG(LogTemp, Display, TEXT("[NF Cheat] %s"), *Line);
	}
#endif
}
