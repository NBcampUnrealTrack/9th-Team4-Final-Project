#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePlay/ConsumableItems/NF_ConsumableSlot.h"
#include "GamePlay/ConsumableItems/NF_UseItemInterface.h"
#include "NF_ConsumableComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNF_OnConsumableSlotsChanged);

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// 5칸 소모품 보관 및 서버 사용 요청. 키 입력은 외부에서 연결한다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_ConsumableComponent : public UActorComponent, public INF_UseItemInterface
{
	GENERATED_BODY()
public:
	UNF_ConsumableComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void NF_UseItem_Implementation(int32 SlotIndex) override;

	// 서버에서 즉시 사용하고 성공 시 1개 차감한다. 입력 측은 NF_UseItem 인터페이스로 요청한다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Use")
	bool TryUseItem(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "NF|Consumable|Slots")
	int32 GetSlotCount() const { return SlotCount; }

	// 복사본을 반환하므로 외부에서 내부 배열의 크기/수량을 직접 변경할 수 없다.
	UFUNCTION(BlueprintPure, Category = "NF|Consumable|Slots")
	TArray<FNF_ConsumableSlot> GetSlots() const { return Slots; }

	UFUNCTION(BlueprintPure, Category = "NF|Consumable|Slots")
	bool GetSlot(int32 SlotIndex, FNF_ConsumableSlot& OutSlot) const;

	// 같은 원본 DA가 있으면 합산, 없으면 첫 빈 슬롯. 전량 보관에 실패하면 변경하지 않는다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Pickup", meta = (ToolTip = "소모품을 전량 보관합니다. 같은 DA는 기존 슬롯에 합치고, 없으면 첫 빈칸을 사용합니다. 공간 부족이나 수량 범위 초과 시 기존 슬롯을 덮어쓰지 않습니다. 서버 전용입니다."))
	bool TryAddItem(UNF_ConsumableItemDataAsset* ItemData, int32 Quantity = 1);

	// 명시적 슬롯 설정 API. 자동 교체 정책이 아니며 호출자가 대상 슬롯을 정한다.
	// 유효 DA+양수 수량 또는 nullptr+0만 허용한다. 서버에서 호출한다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Slots", meta = (ToolTip = "지정 슬롯의 원본 DA와 수량을 설정합니다. 기존 내용은 교체되며, 빈 슬롯은 DA=None과 수량=0으로 설정합니다. 서버 전용입니다."))
	bool SetSlot(int32 SlotIndex, UNF_ConsumableItemDataAsset* ItemData, int32 Quantity);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Slots", meta = (ToolTip = "지정 슬롯에서 수량을 차감합니다. 부족하면 변경하지 않고 실패합니다. 0개가 되면 DA 참조도 비웁니다. 효과를 실행하는 함수는 아닙니다."))
	bool RemoveFromSlot(int32 SlotIndex, int32 Quantity = 1);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Slots")
	bool ClearSlot(int32 SlotIndex);

	// 서버의 성공한 변경 및 소유 클라이언트의 복제 수신 시 알림. UI를 직접 참조하지 않는다.
	UPROPERTY(BlueprintAssignable, Category = "NF|Consumable|Slots")
	FNF_OnConsumableSlotsChanged OnSlotsChanged;

private:
	static constexpr int32 SlotCount = 5;
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Slots)
	TArray<FNF_ConsumableSlot> Slots;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION(Server, Reliable)
	void ServerUseItem(int32 SlotIndex);
	bool bChangingSlots = false;
};
