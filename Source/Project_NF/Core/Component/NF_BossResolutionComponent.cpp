#include "Core/Component/NF_BossResolutionComponent.h"

#include "Core/NF_GameMode.h"
#include "Core/NF_GameState.h"
#include "Data/Enum/NF_MatchPhase.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "Player/State/NF_PlayerState.h"

UNF_BossResolutionComponent::UNF_BossResolutionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// GameMode 전용 서버 컴포넌트
	SetIsReplicatedByDefault(false);
}

bool UNF_BossResolutionComponent::ResolveBossDefeat(const FNF_BossDefeatResult& DefeatResult)
{
	const ANF_GameMode* NFGameMode =Cast<ANF_GameMode>(GetOwner());

	if (!IsValid(NFGameMode)) {return false;}
	
	if (!NFGameMode->HasAuthority()) {return false;}

	if (bBossDefeatResolved)
	{
		UE_LOG(LogTemp,Warning,TEXT("[BossResolution] 보스 처치 결과가 ""이미 확정되었습니다."));
		return false;
	}

	if (!ValidateDefeatResult(DefeatResult)){return false;}
	
	bBossDefeatResolved = true;
	ConfirmedResult = MakeConfirmedResult(DefeatResult);

	OnBossDefeatResolved.Broadcast(ConfirmedResult);

	return true;
}

bool UNF_BossResolutionComponent::ValidateDefeatResult(const FNF_BossDefeatResult& DefeatResult) const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World)) {return false;}

	const ANF_GameState* NFGameState =World->GetGameState<ANF_GameState>();
	if (!IsValid(NFGameState)) {return false;}

	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Boss) {return false;}

	if (!IsValid(DefeatResult.DefeatedBoss)) {return false;}
	
	return true;
}

FNF_BossDefeatResult UNF_BossResolutionComponent::MakeConfirmedResult(const FNF_BossDefeatResult& DefeatResult) const
{
	FNF_BossDefeatResult Result = DefeatResult;

	if (!IsValid(Result.LastHitPlayerState))
	{
		Result.LastHitPlayerID = INDEX_NONE;
		return Result;
	}

	// 보스 시스템이 전달한 ID 값과 PlayerState를 기준으로 확정
	Result.LastHitPlayerID =Result.LastHitPlayerState->GetPlayerId();
	
	return Result;
}