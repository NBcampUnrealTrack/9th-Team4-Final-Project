#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Engine/NetSerialization.h"
#include "NF_SkillTargetData.generated.h"

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
// 목적: 확정한 방향/지점을 GAS TargetData로 전달한다. 피해량과 생성 위치는 서버가 결정한다.
UENUM(BlueprintType)
enum class ENF_SkillTargetKind : uint8
{
	None,
	Direction,
	GroundPoint
};

USTRUCT()
struct PROJECT_NF_API FNF_SkillTargetData : public FGameplayAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY()
	ENF_SkillTargetKind Kind = ENF_SkillTargetKind::None;
	UPROPERTY()
	FVector_NetQuantize Location = FVector::ZeroVector;
	UPROPERTY()
	FVector_NetQuantizeNormal AimDirection = FVector::ForwardVector;

	virtual UScriptStruct* GetScriptStruct() const override { return StaticStruct(); }
	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
	static const FNF_SkillTargetData* Get(const FGameplayAbilityTargetDataHandle& Handle);
};

template<>
struct TStructOpsTypeTraits<FNF_SkillTargetData> : TStructOpsTypeTraitsBase2<FNF_SkillTargetData>
{
	enum { WithNetSerializer = true };
};
