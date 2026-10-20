#pragma once
#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "GamePlay/Combat/Skill/Execution/NF_CharacterPullActor.h"
#include "NFAbilityTask_PullCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_PullCharacterFinished, bool, bReachedDestination);

/** Shared by player and AI abilities; executes only on authority. */
UCLASS()
class PROJECT_NF_API UNFAbilityTask_PullCharacter : public UAbilityTask
{
	GENERATED_BODY()
public:
	UNFAbilityTask_PullCharacter();
	UFUNCTION(BlueprintCallable, Category = "NF|Ability|Tasks", meta = (DefaultToSelf = "OwningAbility", HidePin = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UNFAbilityTask_PullCharacter* PullCharacter(UGameplayAbility* OwningAbility, ACharacter* Target,
		FVector Destination, FNF_CharacterPullSettings Settings);
	UPROPERTY(BlueprintAssignable)
	FNF_PullCharacterFinished OnFinished;
	UPROPERTY(BlueprintReadOnly, Category = "NF|Ability|Tasks")
	ENF_CharacterPullResult Result = ENF_CharacterPullResult::Cancelled;
	bool HasStartedPull() const { return bStartedPull; }
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
protected:
	virtual void OnDestroy(bool AbilityIsEnding) override;
private:
	void HandleFinished(bool bReached);
	TWeakObjectPtr<ACharacter> TargetCharacter;
	TWeakObjectPtr<AActor> OriginalAvatar;
	TWeakObjectPtr<ANF_CharacterPullActor> PullActor;
	FVector DestinationLocation = FVector::ZeroVector;
	FNF_CharacterPullSettings PullSettings;
	bool bStartedPull = false;
};
