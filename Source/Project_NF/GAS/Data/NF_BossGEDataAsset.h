#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NF_BossGEDataAsset.generated.h"

class UGameplayEffect;

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_BossGEDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/* 아래 형식으로 GE 클래스 참조해 주시면 됩니다
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Boss")
	TSubclassOf<UGameplayEffect> @@@EffectClass;
	 */

	// 플레이어 무기 투사체가 보스에 피해를 줄 때 사용 (보스 IncomingDamage에 양수 Data.Damage)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Boss")
	TSubclassOf<UGameplayEffect> PlayerDamageToBossEffectClass;

	// 보스 패턴(CenterBlast, LastGasp)이 플레이어를 기절시킬 때 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Boss")
	TSubclassOf<UGameplayEffect> BossStunEffectClass;

	// 보스 패턴(Relocate)이 플레이어를 공중에 띄울 때 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Effects|Boss")
	TSubclassOf<UGameplayEffect> BossAirborneEffectClass;
};
