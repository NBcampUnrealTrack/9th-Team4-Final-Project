#include "Core/Component/NF_BossResolutionComponent.h"

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

bool UNF_BossResolutionComponent::ResolveBossDefeat(const FNF_BossDefeatResult& DefeatResult,const ANF_BossBase* ActiveBoss)
{
	const AActor* OwnerActor = GetOwner();

	if (!IsValid(OwnerActor) || !OwnerActor->HasAuthority()){return false;}

	if (bBossDefeatResolved){return false;}

	if (!ValidateDefeatResult(DefeatResult, ActiveBoss)){return false;}

	ConfirmedResult = MakeConfirmedResult(DefeatResult);

	// 이벤트 내부에서 재진입하더라도 중복 처리를 차단한다.
	bBossDefeatResolved = true;

	OnBossDefeatResolved.Broadcast(ConfirmedResult);
	return true;
}

bool UNF_BossResolutionComponent::ValidateDefeatResult(const FNF_BossDefeatResult& DefeatResult,const ANF_BossBase* ActiveBoss) const
{
	const UWorld* World = GetWorld();

	if (!IsValid(World)){return false;}

	const ANF_GameState* NFGameState =World->GetGameState<ANF_GameState>();

	if (!IsValid(NFGameState)){return false;}

	if (NFGameState->GetMatchPhase() != ENF_MatchPhase::Boss){return false;}

	if (!IsValid(ActiveBoss)){return false;}

	if (DefeatResult.DefeatedBoss.Get() != ActiveBoss){return false;}

	if (ActiveBoss->GetWorld() != World){return false;}

	return ActiveBoss->IsDefeated();
}

FNF_BossDefeatResult UNF_BossResolutionComponent::MakeConfirmedResult(const FNF_BossDefeatResult& DefeatResult) const
{
	FNF_BossDefeatResult Result = DefeatResult;

	// 직전 ValidateDefeatResult()에서 World와 GameState를 검증했다.
	const ANF_GameState* NFGameState =GetWorld()->GetGameState<ANF_GameState>();

	ANF_PlayerState* LastHitPlayerState =Result.LastHitPlayerState.Get();

	const bool bValidRecipient =IsValid(LastHitPlayerState) &&LastHitPlayerState->GetWorld() == GetWorld() &&NFGameState->PlayerArray.Contains(LastHitPlayerState);

	if (bValidRecipient)
	{
		Result.LastHitPlayerID = LastHitPlayerState->GetPlayerId();
	}
	else
	{
		// 지급 대상이 없어도 보스 처치는 정상 확정한다.
		Result.LastHitPlayerState = nullptr;
		Result.LastHitPlayerID = INDEX_NONE;
	}

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
