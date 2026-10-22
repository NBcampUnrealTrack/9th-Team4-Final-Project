#include "Core/Component/NF_BossContributionRewardComponent.h"

#include "AbilitySystemComponent.h"
#include "Core/NF_GameMode.h"
#include "Core/NF_GameState.h"
#include "Curves/CurveFloat.h"
#include "Data/Struct/NF_BossDefeatResult.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GAS/Tags/NFTag_LifeState.h"
#include "GameplayEffect.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"
#include "Player/State/NF_PlayerState.h"

UNF_BossContributionRewardComponent::UNF_BossContributionRewardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	SetIsReplicatedByDefault(false);
}

bool UNF_BossContributionRewardComponent::ApplyRewards(const FNF_BossDefeatResult& DefeatResult)
{
	if (!HasServerAuthority()){return false;}
	
	//기여도 보상 이미 적용 시
	if (bRewardApplied){return false;}
	if (!IsValid(DefeatResult.DefeatedBoss)){return false;}
	if (!ContributionRecoveryEffectClass){return false;}
	if (!RecoveryMagnitudeTag.IsValid()){return false;}
	if (!IsValid(ContributionRecoveryCurve)){return false;}

	TMap<ANF_PlayerState*, float> DamageByPlayer;
	float TotalDamage = 0.0f;

	if (!BuildDamageByPlayer(DefeatResult,DamageByPlayer,TotalDamage))
	{
		UE_LOG(LogTemp,Warning,TEXT("[BossReward] 유효한 보스 피해 ""기여도가 없어 보상을 생략합니다."));

		//재처리 방지 위해 완료처리
		bRewardApplied = true;
		return true;
	}

	const UWorld* World = GetWorld();
	if (!IsValid(World)){return false;}

	const ANF_GameState* NFGameState =World->GetGameState<ANF_GameState>();
	if (!IsValid(NFGameState)){return false;}
	
	bRewardApplied = true;

	int32 AppliedPlayerCount = 0;

	for (APlayerState* BasePlayerState: NFGameState->PlayerArray)
	{
		ANF_PlayerState* NFPlayerState =Cast<ANF_PlayerState>(BasePlayerState);

		if (!IsValid(NFPlayerState)) continue;
		
		const float PlayerDamage =DamageByPlayer.FindRef(NFPlayerState);

		// 피해 기여도 0인 플레이어는 회복량 0
		if (PlayerDamage <= 0.0f) continue;

		const float ContributionRate =FMath::Clamp(PlayerDamage / TotalDamage,0.0f,1.0f);

		const float RecoveryRate =FMath::Clamp(ContributionRecoveryCurve->GetFloatValue(ContributionRate),0.0f,1.0f);

		UNF_PlayerStatusAttribute* StatusAttribute =NFPlayerState->PlayerStatusAttribute;

		if (!IsValid(StatusAttribute))continue;

		const float RecoveryAmount =StatusAttribute->GetMaxHealth()* RecoveryRate;

		if (ApplyRecoveryEffect(NFPlayerState,RecoveryAmount)) {++AppliedPlayerCount;}
	}

	UE_LOG(LogTemp,Log,TEXT("[BossReward] 기여도 회복 보상 적용 완료. ""적용 인원=%d"),AppliedPlayerCount);

	return true;
}

bool UNF_BossContributionRewardComponent::PrepareTeamPenalty(const FNF_BossDefeatResult& DefeatResult)
{
	if (!HasServerAuthority() || bTeamPenaltyCleaned) return false;
	if (bTeamPenaltyPrepared) return true;
	if (!IsValid(DefeatResult.DefeatedBoss) || DefeatResult.DefeatedBoss->GetWorld() != GetWorld()) return false;

	const ANF_GameState* GameState = GetWorld()->GetGameState<ANF_GameState>();
	if (!IsValid(GameState) || GameState->GetMatchPhase() != ENF_MatchPhase::Boss) return false;

	TArray<FTeamPenaltyMember> Members;
	TMap<ANF_PlayerState*, int32> TeamByPlayer;
	TMap<int32, double> DamageByTeam;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		if (!IsValid(Controller)) continue;

		ANF_PlayerState* PlayerState = Controller->GetPlayerState<ANF_PlayerState>();
		if (!IsValid(PlayerState) || PlayerState->NF_TeamID <= 0) continue;
		if (!GameState->PlayerArray.Contains(PlayerState) || TeamByPlayer.Contains(PlayerState)) continue;

		FTeamPenaltyMember Member;
		Member.PlayerState = PlayerState;
		Member.TeamId = PlayerState->NF_TeamID;
		Members.Add(Member);
		TeamByPlayer.Add(PlayerState, Member.TeamId);

		// 피해를 가하지 않은 팀도 비교 대상에 포함한다.
		DamageByTeam.FindOrAdd(Member.TeamId);
	}

	if (DamageByTeam.IsEmpty()) return false;

	for (const FNF_BossDamageContribution& Entry : DefeatResult.Contributions)
	{
		const int32* TeamId = TeamByPlayer.Find(Entry.PlayerState.Get());
		if (TeamId == nullptr) continue;
		if (!FMath::IsFinite(Entry.EffectiveDamage) || Entry.EffectiveDamage <= 0.0f) continue;

		double& Damage = DamageByTeam.FindChecked(*TeamId);
		Damage += static_cast<double>(Entry.EffectiveDamage);
		if (!FMath::IsFinite(Damage)) return false;
	}

	double MinimumDamage = TNumericLimits<double>::Max();
	bool bAnyDamage = false;

	for (const TPair<int32, double>& Pair : DamageByTeam)
	{
		MinimumDamage = FMath::Min(MinimumDamage, Pair.Value);
		bAnyDamage |= Pair.Value > 0.0;
	}

	PenaltyMembers = MoveTemp(Members);
	LowestContributionTeams.Reset();

	if (bAnyDamage)
	{
		for (const TPair<int32, double>& Pair : DamageByTeam)
		{
			// 피해량 차이 0.01 이하는 동률로 처리하며 해당 팀 모두 선정한다.
			if (FMath::Abs(Pair.Value - MinimumDamage) <= 0.01)
			{
				LowestContributionTeams.Add(Pair.Key);
			}
		}
	}

	bTeamPenaltyPrepared = true;
	UE_LOG(LogTemp, Log, TEXT("[BossPenalty] 팀 스냅샷 확정. 비교 팀=%d, 최저 팀=%d"), DamageByTeam.Num(), LowestContributionTeams.Num());
	return true;
}

bool UNF_BossContributionRewardComponent::ApplyTeamPenalty()
{
	if (!HasServerAuthority() || !bTeamPenaltyPrepared || bTeamPenaltyCleaned || bApplyingTeamPenalty) return false;

	const ANF_GameState* GameState = GetWorld()->GetGameState<ANF_GameState>();
	if (!IsValid(GameState) || GameState->GetMatchPhase() != ENF_MatchPhase::Escape) return false;
	if (LowestContributionTeams.IsEmpty()) return true;

	if (!LowestTeamPenaltyEffectClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossPenalty] LowestTeamPenaltyEffectClass가 설정되지 않았습니다."));
		return false;
	}

	const UGameplayEffect* EffectCDO = LowestTeamPenaltyEffectClass->GetDefaultObject<UGameplayEffect>();
	if (!IsValid(EffectCDO) || EffectCDO->DurationPolicy != EGameplayEffectDurationType::Infinite)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossPenalty] 최대 실드량 감소 GE는 Infinite여야 합니다."));
		return false;
	}

	TGuardValue<bool> ApplyingGuard(bApplyingTeamPenalty, true);
	bool bAllResolved = true;

	for (const FTeamPenaltyMember& Member : PenaltyMembers)
	{
		if (bTeamPenaltyCleaned || !IsValid(GameState) || GameState->GetMatchPhase() != ENF_MatchPhase::Escape) return false;
		if (!LowestContributionTeams.Contains(Member.TeamId) || ResolvedPenaltyPlayers.Contains(Member.PlayerState)) continue;

		ANF_PlayerState* PlayerState = Member.PlayerState.Get();
		if (!IsValid(PlayerState) || !GameState->PlayerArray.Contains(PlayerState))
		{
			ResolvedPenaltyPlayers.Add(Member.PlayerState);
			continue;
		}

		APlayerController* Controller = PlayerState->GetPlayerController();
		if (!IsValid(Controller) || Controller->GetPlayerState<ANF_PlayerState>() != PlayerState)
		{
			ResolvedPenaltyPlayers.Add(Member.PlayerState);
			continue;
		}

		UAbilitySystemComponent* ASC = PlayerState->GetAbilitySystemComponent();
		if (!IsValid(ASC))
		{
			bAllResolved = false;
			continue;
		}

		// 사망자는 적용 제외로 확정한다. 기절자는 이 정책에서 제외하지 않는다.
		if (ASC->HasMatchingGameplayTag(NFTag_LifeState::State_Life_Dead))
		{
			ResolvedPenaltyPlayers.Add(Member.PlayerState);
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(LowestTeamPenaltyEffectClass, 1.0f, Context);

		if (!Spec.IsValid())
		{
			bAllResolved = false;
			continue;
		}

		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

		// GE 적용 콜백에서 매치 정리가 재진입하면 방금 적용된 효과도 제거한다.
		if (bTeamPenaltyCleaned)
		{
			if (IsValid(ASC) && Handle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(Handle);
			}
			return false;
		}

		if (!IsValid(ASC) || !Handle.IsValid() || ASC->GetActiveGameplayEffect(Handle) == nullptr)
		{
			bAllResolved = false;
			continue;
		}

		FAppliedTeamPenalty Applied;
		Applied.ASC = ASC;
		Applied.Handle = Handle;
		AppliedTeamPenalties.Add(Applied);
		ResolvedPenaltyPlayers.Add(Member.PlayerState);
	}

	return bAllResolved;
}

void UNF_BossContributionRewardComponent::CleanupTeamPenalty()
{
	if (!HasServerAuthority() || bTeamPenaltyCleaned) return;

	bTeamPenaltyCleaned = true;
	TArray<FAppliedTeamPenalty> EffectsToRemove = MoveTemp(AppliedTeamPenalties);

	for (const FAppliedTeamPenalty& Applied : EffectsToRemove)
	{
		UAbilitySystemComponent* ASC = Applied.ASC.Get();
		if (IsValid(ASC) && Applied.Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Applied.Handle);
		}
	}
}

void UNF_BossContributionRewardComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CleanupTeamPenalty();
	Super::EndPlay(EndPlayReason);
}

bool UNF_BossContributionRewardComponent::HasServerAuthority() const
{
	const ANF_GameMode* NFGameMode =Cast<ANF_GameMode>(GetOwner());

	return IsValid(NFGameMode)&& NFGameMode->HasAuthority();
}

bool UNF_BossContributionRewardComponent::BuildDamageByPlayer(
	const FNF_BossDefeatResult& DefeatResult,
	TMap<ANF_PlayerState*, float>& OutDamageByPlayer,
	float& OutTotalDamage) const
{
	OutDamageByPlayer.Reset();
	OutTotalDamage = 0.0f;

	for (const FNF_BossDamageContribution& Entry
		: DefeatResult.Contributions)
	{
		if (!IsValid(Entry.PlayerState))continue;
		
		if (!FMath::IsFinite(Entry.EffectiveDamage))continue;

		if (Entry.EffectiveDamage <= 0.0f)continue;

		OutDamageByPlayer.FindOrAdd(Entry.PlayerState) += Entry.EffectiveDamage;

		OutTotalDamage += Entry.EffectiveDamage;
	}

	return OutTotalDamage > KINDA_SMALL_NUMBER;
}

bool UNF_BossContributionRewardComponent::ApplyRecoveryEffect(ANF_PlayerState* PlayerState,const float RecoveryAmount)
{
	if (!IsValid(PlayerState)){return false;}
	if (RecoveryAmount <= 0.0f){return false;}

	UNF_PlayerStatusAttribute* StatusAttribute =PlayerState->PlayerStatusAttribute;
	if (!IsValid(StatusAttribute)){return false;}
	
	//사망한 플레이어는 보상 버프 제외
	//지우님 State.Player.Dead 상태 태그를 제공 시 로직 변경 
	if (StatusAttribute->GetHealth() <= 0.0f){return false;}

	UAbilitySystemComponent* TargetASC =PlayerState->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)){return false;}
	
	if (!ContributionRecoveryEffectClass){return false;}
	if (!RecoveryMagnitudeTag.IsValid()){return false;}

	FGameplayEffectContextHandle EffectContext =TargetASC->MakeEffectContext();

	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle EffectSpec =
		TargetASC->MakeOutgoingSpec(ContributionRecoveryEffectClass,1.0f,EffectContext);

	if (!EffectSpec.IsValid()){return false;}

	EffectSpec.Data->SetSetByCallerMagnitude(RecoveryMagnitudeTag,RecoveryAmount);

	TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());

	return true;
}
