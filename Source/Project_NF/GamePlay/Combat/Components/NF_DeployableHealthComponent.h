#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_DeployableHealthComponent.generated.h"

class UAbilitySystemComponent;
class UNF_DeployableHealthAttribute;
struct FOnAttributeChangeData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNF_DeployableHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_DeployableHealthDepleted);

// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 설치물 ASC에 체력 Attribute를 등록하고 체력 소진을 설치물 종료로 연결한다.
// 액터에 ASC를 하나 추가해야 하며, 기존 캐릭터 ASC의 체력은 수정하지 않는다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_DeployableHealthComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_DeployableHealthComponent();
	// 자동 초기화 외에 ASC가 늦게 준비되는 액터는 이 함수를 다시 호출한다.
	UFUNCTION(BlueprintCallable, Category = "NF|Deployable Health")
	bool InitializeHealth();
	UFUNCTION(BlueprintPure, Category = "NF|Deployable Health")
	float GetHealth() const;
	UFUNCTION(BlueprintPure, Category = "NF|Deployable Health")
	float GetMaxHealth() const;
	UPROPERTY(BlueprintAssignable, Category = "NF|Deployable Health")
	FNF_DeployableHealthChanged OnHealthChanged;
	// 소진 이벤트는 서버에서만 호출한다. 클라이언트 연출은 설치 종료 이벤트를 사용한다.
	UPROPERTY(BlueprintAssignable, Category = "NF|Deployable Health")
	FNF_DeployableHealthDepleted OnHealthDepleted;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Deployable Health", meta = (ClampMin = "1.0"))
	float InitialMaxHealth = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|Deployable Health")
	bool bEndDeploymentOnDepleted = true;
private:
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);
	void NotifyHealth();
	TWeakObjectPtr<UAbilitySystemComponent> ASC;
	const UNF_DeployableHealthAttribute* FindAttributes() const;
	FDelegateHandle HealthDelegate;
	FDelegateHandle MaxHealthDelegate;
	bool bInitialized = false;
	bool bDepletedNotified = false;
	bool bEndingPlay = false;
};
