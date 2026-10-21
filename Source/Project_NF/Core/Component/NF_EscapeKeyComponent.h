#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Data/Enum/NF_KeyInitState.h"
#include "Data/Enum/NF_KeyState.h"
#include "Data/Struct/NF_KeyPublicState.h"
#include "NF_EscapeKeyComponent.generated.h"

class ANF_PlayerState;
class UGameplayEffect;
class UAbilitySystemComponent;
struct FNF_BossDefeatResult;
class APawn;
class ANF_EscapeKey;

DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnKeyOperationFailed,const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnInitialKeyAssignmentFailed, const FString&);
DECLARE_MULTICAST_DELEGATE_OneParam(FNF_OnKeyStateChanged, const FNF_KeyPublicState&);

UCLASS(ClassGroup = (NF))
class PROJECT_NF_API UNF_EscapeKeyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_EscapeKeyComponent();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 최초 지급이 이미 완료됐다면 중복 지급 없이 true.
	// 서버에서 확정된 처치 결과로 최초 지급 절차를 시작한다.
	void BeginInitialAssignment(const FNF_BossDefeatResult& DefeatResult);

	// 매치 종료·중단 시 호출.
	void CleanupKey();

	// 소유권 해제 및 보유자 GE 제거.
	// 바닥 열쇠 액터 생성은 별도 처리.
	bool DropKey();
	
	bool TryPickup(ANF_PlayerState* PlayerState,ANF_EscapeKey* Pickup);

	void HandlePlayerLeaving(ANF_PlayerState* PlayerState,APawn* LastPawn);

	bool IsKeyHolder(const ANF_PlayerState* PlayerState) const;

	ANF_PlayerState* GetKeyHolder() const;
	ENF_KeyInitState GetInitState() const;
	FNF_KeyPublicState GetPublicState() const;

	FNF_OnKeyStateChanged OnKeyStateChanged;

	FNF_OnInitialKeyAssignmentFailed OnInitialKeyAssignmentFailed;

	FNF_OnKeyOperationFailed OnKeyOperationFailed;
	
private:
	bool HasServerAuthority() const;

	bool ApplyKeyHolderEffect(ANF_PlayerState* PlayerState, FActiveGameplayEffectHandle& OutEffectHandle);

	void TryInitialAssignment();
	void FailInitialAssignment(const FString& Reason);
	
	bool TransferKey(ANF_PlayerState* NewKeyHolder);
	
	bool IsEscapePhase() const;
	
	bool CanReceiveKey(ANF_PlayerState* PlayerState,bool bRequireAlive) const;

	bool CreateDroppedKeyAt(const FVector& Location);
	void CompleteInitialAssignmentByDrop();

	void StartHolderMonitoring();
	void MonitorHolder();
	void PublishKeyState();
	
	UPROPERTY(EditDefaultsOnly,Category = "NF|Escape|Key|Init",meta = (ClampMin = "1"))
	int32 MaxInitialAssignmentAttempts = 3;

	UPROPERTY(EditDefaultsOnly,Category = "NF|Escape|Key|Init",meta = (ClampMin = "0.1"))
	float InitialAssignmentRetryInterval = 1.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "NF|Escape|Key")
	TSubclassOf<UGameplayEffect> KeyHolderRewardEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "NF|Escape|Key")
	TSubclassOf<ANF_EscapeKey> KeyPickupClass;

	UPROPERTY(EditDefaultsOnly,Category = "NF|Escape|Key",meta = (ClampMin = "1.0"))
	float PickupDistance = 250.0f;

	UPROPERTY(Transient)
	TObjectPtr<ANF_EscapeKey> DroppedKeyActor;

	bool bHasInitialDropLocation = false;
	bool bHolderLeaving = false;
	bool bProcessingPickup = false;
	bool bKeyCleanupComplete = false;
	bool bChangingKeyHolder = false;
	
	FTimerHandle HolderMonitorTimerHandle;
	FTimerHandle InitialAssignmentRetryTimerHandle;
	
	// GE를 적용한 ASC
	TWeakObjectPtr<UAbilitySystemComponent> KeyHolderEffectASC;
	
	TWeakObjectPtr<ANF_PlayerState> KeyHolder;
	TWeakObjectPtr<ANF_PlayerState> InitialRecipient;
	TWeakObjectPtr<APawn> LastPublishedHolderPawn;
	ENF_KeyInitState InitState =ENF_KeyInitState::NotStarted;
	ENF_KeyState KeyState = ENF_KeyState::Unassigned;
	FVector InitialDropLocation = FVector::ZeroVector;
	FVector LastHolderLocation = FVector::ZeroVector;
	FActiveGameplayEffectHandle KeyHolderEffectHandle;
	int32 InitialAssignmentAttempts = 0;
	
	
};
