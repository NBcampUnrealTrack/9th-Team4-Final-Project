#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "NF_ConsumablePickup.generated.h"

class UNF_ConsumableItemDataAsset;
class USphereComponent;
class UStaticMeshComponent;

// @Owner: 이정준 (구현: Codex) | 2026-10-27
// 월드 픽업 전용. DA의 ItemClass(사용 효과 액터)와는 별도 클래스다.
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_ConsumablePickup : public AActor, public INF_InteractionInterface
{
	GENERATED_BODY()
public:
	ANF_ConsumablePickup();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void NF_Interaction(const APlayerController* InteractedController) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Pickup")
	bool TryPickup(const APlayerController* InteractedController);

	// 드랍 생성 측에서 서버에 생성한 액터의 원본과 수량을 설정할 때 사용한다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Consumable|Pickup")
	bool InitializePickup(UNF_ConsumableItemDataAsset* InItemData, int32 InQuantity = 1);

	UFUNCTION(BlueprintPure, Category = "NF|Consumable|Pickup")
	UNF_ConsumableItemDataAsset* GetItemData() const { return ItemData.Get(); }
	UFUNCTION(BlueprintPure, Category = "NF|Consumable|Pickup")
	int32 GetQuantity() const { return Quantity; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Consumable|Pickup")
	TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Consumable|Pickup")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "NF|Consumable|Pickup", meta = (ExposeOnSpawn = "true", ToolTip = "주우면 슬롯에 보관할 소모품 원본 DA입니다. 사용 효과는 이 DA의 ItemClass가 담당합니다."))
	TObjectPtr<UNF_ConsumableItemDataAsset> ItemData;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "NF|Consumable|Pickup", meta = (ExposeOnSpawn = "true", ClampMin = "1", ToolTip = "이번 픽업으로 획득할 전체 수량입니다. 전량 보관 성공 시 월드 액터를 제거합니다."))
	int32 Quantity = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable|Pickup", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "캐릭터 위치 기준으로 서버가 허용할 최대 획득 거리입니다."))
	float MaxPickupDistance = 250.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable|Pickup", meta = (ToolTip = "서버에서 캐릭터 눈 위치와 아이템 사이의 Visibility 가림을 확인합니다."))
	bool bRequireLineOfSight = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Consumable|Pickup", meta = (ToolTip = "켜면 Sphere Collision을 물리 시뮬레이션하여 떨어뜨립니다. 메시 자체의 충돌은 사용하지 않습니다."))
	bool bSimulateDropPhysics = false;

private:
	bool bPickingUp = false;
};
