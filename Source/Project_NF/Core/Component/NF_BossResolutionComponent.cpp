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

	if (IsValid(Result.LastHitPlayerState))
	{
		// 보스 시스템이 전달한 ID 값과 PlayerState를 기준으로 확정
		Result.LastHitPlayerID = Result.LastHitPlayerState->GetPlayerId();
	}
	else
	{
		Result.LastHitPlayerID = INDEX_NONE;
	}
	//보스 기여도 계산
	NormalizeContributions(Result);

	return Result;
}

void UNF_BossResolutionComponent::NormalizeContributions(FNF_BossDefeatResult& DefeatResult) const
{
	TMap<ANF_PlayerState*, float> AggregatedDamage;

	for (const FNF_BossDamageContribution& Entry: DefeatResult.Contributions)
	{
		if (!IsValid(Entry.PlayerState)) continue;
		if (!FMath::IsFinite(Entry.EffectiveDamage)) continue;
		if (Entry.EffectiveDamage <= 0.0f) continue;
		
		AggregatedDamage.FindOrAdd(Entry.PlayerState) += Entry.EffectiveDamage;
	}

	DefeatResult.Contributions.Reset();
	DefeatResult.Contributions.Reserve(AggregatedDamage.Num());

	for (const TPair<ANF_PlayerState*, float>& Pair: AggregatedDamage)
	{
		FNF_BossDamageContribution ConfirmedEntry;
		ConfirmedEntry.PlayerState = Pair.Key;
		ConfirmedEntry.EffectiveDamage = Pair.Value;

		DefeatResult.Contributions.Add(ConfirmedEntry
		);
	}

	// 로그와 테스트 결과를 일정하게 만들기 위한 정렬
	DefeatResult.Contributions.Sort([](const FNF_BossDamageContribution& Left,const FNF_BossDamageContribution& Right)
		{
			if (!IsValid(Left.PlayerState)) {return false;}
			if (!IsValid(Right.PlayerState)) {return true;}

			return Left.PlayerState->GetPlayerId() < Right.PlayerState->GetPlayerId();
		}
	);
}
