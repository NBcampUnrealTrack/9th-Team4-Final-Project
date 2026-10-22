#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "NF_PlayerLifeComponent.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

// 플레이어의 생존 상태
UENUM(BlueprintType)
enum class ENFPlayerLifeState : uint8
{
	Alive UMETA(DisplayName = "Alive"),
	Downed UMETA(DisplayName = "Downed"),
	Dead UMETA(DisplayName = "Dead"),
};

enum class ENFTeammateCheckResult : uint8
{
	Unknown,		// 팀원 정보를 확인할 수가 없어서 판정 불가
	AliveTeammate,  // 살아 있는 동료o
	NoAliveTeammate // 살아 있는 동료x
};

// 생존 상태가 변경됐을 때 알림
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnPlayerLifeStateChanged,
	ENFPlayerLifeState, NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthReachedZero);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_PlayerLifeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_PlayerLifeComponent();
	
	// 현재 상태 가져오기
	UFUNCTION(BlueprintPure, Category = "Life")
	ENFPlayerLifeState GetLifeState() const;
	
	// Alive에서 Downed로 변경
	UFUNCTION(BlueprintCallable, Category = "Life")
	bool EnterDowned();
	
	// Alive 또는 Downed에서 Dead로 변경
	UFUNCTION(BlueprintCallable, Category = "Life")
	bool EnterDead();
	
	// Health 회복 후 Downed에서 Alive로 변경
	UFUNCTION(BlueprintCallable, Category = "Life")
	bool Revive();
	
	// ASC가 준비된 후 현재 생존 상태에 맞게 태그
	UFUNCTION(BlueprintCallable, Category = "Life")
	bool SyncLifeStateTags();
	
	// ASc가 준비된 뒤 Health 변화 감지
	UFUNCTION(BlueprintCallable, Category = "Life|Health")
	bool StartHealthZeroDetection();
	
	// 서버에서 팀 ID 배정이 끝난 걸 LifeComp에 알림
	UFUNCTION(BlueprintCallable, Category = "Life|Team")
	bool ConfirmTeamAssignment();
	
	// 팀원 정보가 준비되면 보류 중인 체력 0 판정 다시함
	UFUNCTION(BlueprintCallable, Category = "Life|Health")
	bool RetryPendingHealthZeroCheck();
	
	// Health가 0이 됐을 때 살아 있는 팀원이 있는지 확인하고 생존 상태 결정
	bool HandleHealthReachedZero();
	
	// 팀원이 살아 있는지 확인
	ENFTeammateCheckResult CheckAliveTeammate() const;
	
	// 팀원 정보를 몰라서 체력 0 판정을 기다리도록
	bool MarkHealthZeroCheckPending();
	
	// 생존 상태 변경 알림
	UPROPERTY(BlueprintAssignable, Category = "Life")
	FOnPlayerLifeStateChanged OnLifeStateChanged;
	
	// 체력 0 되면 알림
	UPROPERTY(BlueprintAssignable, Category = "Life|Health")
	FOnHealthReachedZero OnHealthReachedZero;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	
	// 컴포넌트가 종료될 때 기절 타이머 정리
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	// 기절 유지 시간 (임시 30초)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Life|Downed", meta = (ClampMin = "0.1"))
	float DownedDurationSeconds = 30.0f;
	
	// 소생 시 Health를 회복시킬 GE
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Life|Revive")
	TSubclassOf<UGameplayEffect> ReviveHealthEffectClass;
	
	// 현재 생존 상태 (서버에서 변경, 클라에서 복제)
	UPROPERTY(ReplicatedUsing = OnRep_LifeState,
		BlueprintReadOnly, Category = "Life")
	ENFPlayerLifeState LifeState = ENFPlayerLifeState::Alive;
	
	// 클라에서 상태가 변경됐을 때 호출
	UFUNCTION()
	void OnRep_LifeState();
	
private:
	// 기절 제한 시간 관리용
	FTimerHandle DownedTimerHandle;
	
	// 기절 제한 시간이 끝나면 실행
	void OnDownedTimerExpired();
	
	// 서버에서만 생존 상태를 변경하는 내부 함수
	bool ChangeLifeState(ENFPlayerLifeState NewState);
	
	// 팀 ID 배정 완료를 알림 받았는지 저장
	bool bTeamAssignmentConfirmed = false;
	
	// 팀원 정보를 몰라서 체력 0 판정을 기다리고 있는지 저장
	bool bPendingHealthZeroCheck = false;
	
	// Health 변경 감지를 등록한 ASC 기억
	TWeakObjectPtr<UAbilitySystemComponent> ObserverASC;
	
	// Health 변경을 감지하도록 등록한 연결 정보
	FDelegateHandle HealthChangedHandle;
	
	// Health 값 변경될 때 호출
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	
	// 이전 Health 감지 연결 해제
	void StopHealthZeroDetection();
};
