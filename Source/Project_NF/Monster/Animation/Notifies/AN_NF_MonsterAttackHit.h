#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_NF_MonsterAttackHit.generated.h"

/**
 * 몬스터 기본 공격 몽타주의 타격 프레임에서 피해 처리를 호출한다.
 */
UCLASS()
class PROJECT_NF_API UAN_NF_MonsterAttackHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
