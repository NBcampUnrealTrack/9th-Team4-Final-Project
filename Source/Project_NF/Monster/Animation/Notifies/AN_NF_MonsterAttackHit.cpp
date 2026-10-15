#include "Monster/Animation/Notifies/AN_NF_MonsterAttackHit.h"

#include "Monster/NF_Monster.h"

void UAN_NF_MonsterAttackHit::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	ANF_Monster* Monster = Cast<ANF_Monster>(MeshComp->GetOwner());

	if (!Monster)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("ApplyBasicAttackHit called. Monster: %s"),
		*GetNameSafe(Monster));
	
	Monster->ApplyBasicAttackHit();
}

FString UAN_NF_MonsterAttackHit::GetNotifyName_Implementation() const
{
	return TEXT("NF Monster Attack Hit");
}
