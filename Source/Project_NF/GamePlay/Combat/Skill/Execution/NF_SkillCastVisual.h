#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_SkillCastVisual.generated.h"

class UAnimMontage;

// @Owner: 이정준 (구현: Codex) | 2026-10-28
// 서버 확정 시전의 일회성 연출. GA 종료와 독립적으로 소유 클라이언트와 관전자에게 재생한다.
UCLASS(NotBlueprintable, Transient)
class PROJECT_NF_API ANF_SkillCastVisual : public AActor
{
	GENERATED_BODY()
public:
	ANF_SkillCastVisual();
	static ANF_SkillCastVisual* SpawnForCast(AActor* Avatar, FName MeshName, UAnimMontage* Montage,
		float PlayRate, FName StartSection, FName AnimInstanceTag = NAME_None);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void TryPlay();
	UPROPERTY(Replicated)
	TObjectPtr<UAnimMontage> Montage;
	UPROPERTY(Replicated)
	FName MeshName;
	UPROPERTY(Replicated)
	FName AnimInstanceTag;
	UPROPERTY(Replicated)
	float PlayRate = 1.f;
	UPROPERTY(Replicated)
	float StartPosition = 0.f;
	UPROPERTY(Replicated)
	double ServerStartTime = 0.0;
	UPROPERTY(Replicated)
	bool bHasSynchronizedTime = false;
	FTimerHandle RetryTimer;
	int32 RetryCount = 0;
	bool bPlayed = false;
};
