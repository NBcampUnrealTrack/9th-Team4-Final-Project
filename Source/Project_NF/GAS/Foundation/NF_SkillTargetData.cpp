#include "GAS/Foundation/NF_SkillTargetData.h"

bool FNF_SkillTargetData::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	uint8 Value = static_cast<uint8>(Kind);
	Ar.SerializeBits(&Value, 2);
	Kind = static_cast<ENF_SkillTargetKind>(Value);
	bool bLocationValid = true;
	bool bDirectionValid = true;
	Location.NetSerialize(Ar, Map, bLocationValid);
	AimDirection.NetSerialize(Ar, Map, bDirectionValid);
	bOutSuccess = bLocationValid && bDirectionValid && Value <= static_cast<uint8>(ENF_SkillTargetKind::GroundPoint);
	return true;
}

const FNF_SkillTargetData* FNF_SkillTargetData::Get(const FGameplayAbilityTargetDataHandle& Handle)
{
	const FGameplayAbilityTargetData* Data = Handle.Num() == 1 ? Handle.Get(0) : nullptr;
	return Data && Data->GetScriptStruct() == StaticStruct() ? static_cast<const FNF_SkillTargetData*>(Data) : nullptr;
}
