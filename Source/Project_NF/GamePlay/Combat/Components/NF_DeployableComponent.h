#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_DeployableComponent.generated.h"

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 설치물의 시작/유지/종료를 관리한다. 공격과 체력은 별도 컴포넌트로 조합한다.
UENUM(BlueprintType)
enum class ENF_DeploymentState : uint8 { NotStarted, Active, Ended };
UENUM(BlueprintType)
enum class ENF_DeploymentEndReason : uint8 { Manual, Expired, Destroyed, OwnerRemoved };

USTRUCT()
struct FNF_DeploymentStatus
{
	GENERATED_BODY()
	UPROPERTY()
	ENF_DeploymentState State = ENF_DeploymentState::NotStarted;
	UPROPERTY()
	ENF_DeploymentEndReason Reason = ENF_DeploymentEndReason::Manual;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_DeploymentStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_DeploymentEnded, ENF_DeploymentEndReason, Reason);

UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_DeployableComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_DeployableComponent();
	// 공통 실행 액터가 Commit 이후 시작 순서를 소유한다.
	void SetManagedStart(bool bManaged) { bManagedStart = bManaged; }
	bool HasValidSettings() const { return FMath::IsFinite(Lifetime) && Lifetime >= 0.0f && FMath::IsFinite(DestroyDelay) && DestroyDelay > 0.0f; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Deployable")
	bool StartDeployment();
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Deployable")
	void EndDeployment(ENF_DeploymentEndReason Reason = ENF_DeploymentEndReason::Manual);
	UFUNCTION(BlueprintPure, Category = "NF|Deployable")
	bool IsDeploymentActive() const { return Status.State == ENF_DeploymentState::Active; }
	UFUNCTION(BlueprintPure, Category = "NF|Deployable")
	ENF_DeploymentState GetDeploymentState() const { return Status.State; }
	UPROPERTY(BlueprintAssignable, Category = "NF|Deployable")
	FNF_DeploymentStarted OnDeploymentStarted;
	UPROPERTY(BlueprintAssignable, Category = "NF|Deployable")
	FNF_DeploymentEnded OnDeploymentEnded;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Deployable")
	bool bAutoStart = true;
	// 0なら時間制限なし。開始後の延長・再利用は行わない。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Deployable", meta = (ClampMin = "0.0", Units = "s"))
	float Lifetime = 15.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Deployable")
	bool bDestroyOwnerOnEnd = true;
	// 終了を複製し、BPで終了演出を開始するための猶予。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Deployable", meta = (ClampMin = "0.01", Units = "s"))
	float DestroyDelay = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Deployable")
	bool bDisableOwnerCollisionOnEnd = true;

private:
	bool bManagedStart = false;
	UPROPERTY(ReplicatedUsing = OnRep_Status)
	FNF_DeploymentStatus Status;
	UFUNCTION()
	void OnRep_Status();
	void NotifyStatus();
	void ExpireDeployment();
	FTimerHandle LifetimeTimer;
	bool bStartedNotified = false;
	bool bEndedNotified = false;
};
