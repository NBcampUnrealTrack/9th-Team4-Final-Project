#pragma once

#include "CoreMinimal.h"
#include "Player/Controller/NF_PlayerController.h"
#include "NF_TestPlayerController.generated.h"

// @owner : 조민웅 | 2026-10-29
// 목적: 기존 플레이어 컨트롤러를 수정하지 않고 PIE 치트의 소유 연결과 서버 RPC를 제공한다.
UCLASS()
class PROJECT_NF_API ANF_TestPlayerController : public ANF_PlayerController
{
	GENERATED_BODY()

public:
	ANF_TestPlayerController();

	UFUNCTION(Server, Reliable)
	void ServerRequestEscapeTest();

	UFUNCTION(Server, Reliable)
	void ServerRequestMatchDebugState();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Client, Reliable)
	void ClientReportCheatResult(const FString& Message);
};
