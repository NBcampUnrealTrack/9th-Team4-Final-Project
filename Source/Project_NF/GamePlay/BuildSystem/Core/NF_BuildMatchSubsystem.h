#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "NF_BuildMatchSubsystem.generated.h"

class ANF_GameMode;
class ANF_PlayerState;

/** @Owner: 황준현 | 2026-10-19. 서버 월드에서 Core 공개 API에 기본 빌드 콜백을 한 번 등록한다. */
UCLASS()
class PROJECT_NF_API UNF_BuildMatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

private:
	bool HandleApplyFallbackBuild(ANF_PlayerState* PlayerState);
	TWeakObjectPtr<ANF_GameMode> RegisteredGameMode;
};
