#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePlay/Combat/Components/NF_DeployableComponent.h"
#include "NF_RepeatApplicationComponent.generated.h"

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 서버에서 적용 타이밍을 반복한다. 실제 피해/GE 적용은 OnApplication에서 조합한다.
UENUM(BlueprintType)
enum class ENF_RepeatStopReason : uint8 { Manual, DurationExpired, CountReached, DeploymentEnded, OwnerRemoved };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_RepeatedApplication, int32, ApplicationIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_RepeatStopped, ENF_RepeatStopReason, Reason);

UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_RepeatApplicationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_RepeatApplicationComponent();
	bool HasValidSettings() const;
	bool HasFiniteLifetime() const { return Duration > 0.0f || MaxApplications > 0; }
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Repeat Application")
	bool StartRepeating();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Repeat Application")
	void StopRepeating(ENF_RepeatStopReason Reason = ENF_RepeatStopReason::Manual);
	UFUNCTION(BlueprintPure, Category = "NF|Repeat Application")
	bool IsRepeating() const { return bRepeating; }
	UPROPERTY(BlueprintAssignable, Category = "NF|Repeat Application")
	FNF_RepeatedApplication OnApplication;
	UPROPERTY(BlueprintAssignable, Category = "NF|Repeat Application")
	FNF_RepeatStopped OnRepeatStopped;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Repeat Application", meta = (ClampMin = "0.01", Units = "s"))
	float Interval = 1.0f;
	// 0: 시간 제한 없이 반복. 설치물 종료 또는 StopRepeating으로 중단한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Repeat Application", meta = (ClampMin = "0.0", Units = "s"))
	float Duration = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Repeat Application", meta = (ClampMin = "0"))
	int32 MaxApplications = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Repeat Application")
	bool bApplyImmediately = true;
private:
	void ApplyOnce();
	void ExpireRepeating();
	void ScheduleNext();
	UFUNCTION()
	void HandleDeploymentEnded(ENF_DeploymentEndReason Reason);
	TWeakObjectPtr<UNF_DeployableComponent> Deployment;
	FTimerHandle ApplicationTimer;
	FTimerHandle DurationTimer;
	bool bRepeating = false;
	bool bEndingPlay = false;
	int32 ApplicationCount = 0;
	uint32 Generation = 0;
	double EndTime = 0.0;
	float ActiveInterval = 1.0f;
	int32 ActiveMaxApplications = 0;
};
