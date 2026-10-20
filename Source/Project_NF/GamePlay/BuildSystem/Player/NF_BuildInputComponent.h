#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "NF_BuildInputComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnBuildSlotKeysChanged);

// @Owner: 황준현 | 2026-10-19
// 로컬 슬롯 키 설정만 보관한다. 입력 매핑과 스킬 실행은 플레이어가 담당한다.
UCLASS(ClassGroup = (NF), meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_BuildInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNF_BuildInputComponent();

	/** 슬롯 인덱스는 0/1이며 중복 키, 보조 키, 축 입력과 Esc는 허용하지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "NF|Build|Input")
	bool SetSlotKey(int32 SlotIndex, FKey Key);

	UFUNCTION(BlueprintPure, Category = "NF|Build|Input")
	FKey GetSlotKey(int32 SlotIndex) const;

	/** 설정 변경 알림. 실제 전투 키 적용과 입력 충돌 검사는 플레이어 입력 담당에서 처리한다. */
	UPROPERTY(BlueprintAssignable, Category = "NF|Build|Input")
	FNF_OnBuildSlotKeysChanged OnSlotKeysChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|Input")
	FKey SlotOneKey = EKeys::One;

	UPROPERTY(EditDefaultsOnly, Category = "NF|Build|Input")
	FKey SlotTwoKey = EKeys::Two;
};
