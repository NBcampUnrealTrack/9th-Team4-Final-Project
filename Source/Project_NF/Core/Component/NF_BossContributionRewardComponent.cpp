#include "Core/Component/NF_BossContributionRewardComponent.h"

#include "AbilitySystemComponent.h"
#include "Core/NF_GameMode.h"
#include "Core/NF_GameState.h"
#include "Curves/CurveFloat.h"
#include "Data/Struct/NF_BossDefeatResult.h"
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