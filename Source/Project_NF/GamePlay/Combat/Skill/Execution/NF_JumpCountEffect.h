#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_JumpCountEffect.generated.h"

class ACharacter;
USTRUCT()
struct FNF_JumpCountFrame
{
	GENERATED_BODY()
	UPROPERTY()
	TObjectPtr<ACharacter> Target = nullptr;
	UPROPERTY()
	int32 RequiredCount = 0;
};

// @Owner: 이정준 (구현: Codex) | 2026-10-21
// A skill-owned replicated bridge. Multiple grants share one baseline per character.
UCLASS(NotBlueprintable)
class PROJECT_NF_API ANF_JumpCountEffect : public AActor
{
	GENERATED_BODY()
public:
	ANF_JumpCountEffect();
	static ANF_JumpCountEffect* FindOrCreate(ACharacter* Character);
	FGuid AddRequest(int32 Count);
	void RemoveRequest(FGuid Id);
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UPROPERTY(ReplicatedUsing = OnRep_Frame)
	FNF_JumpCountFrame Frame;
	UFUNCTION()
	void OnRep_Frame();
	void RefreshRequests();
	void ApplyLocal();
	void RemoveLocal();
	TMap<FGuid, int32> Requests;
	TWeakObjectPtr<ACharacter> AppliedCharacter;
	int32 OriginalCount = 1;
	int32 AppliedCount = 1;
	bool bClosing = false;
};
