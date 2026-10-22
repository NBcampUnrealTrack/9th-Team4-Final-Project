#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_MonsterItemDrop.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class PROJECT_NF_API ANF_MonsterItemDrop : public AActor
{
	GENERATED_BODY()
	
public:	
	ANF_MonsterItemDrop();
	
	void InitializeDrop(int32 InQuantity);

	UFUNCTION(BlueprintPure, Category = "NF|Monster|ItemDrop")
	FName GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintPure, Category = "NF|Monster|ItemDrop")
	int32 GetQuantity() const { return Quantity; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|ItemDrop")
	TObjectPtr<USceneComponent> DropRootComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|ItemDrop")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|ItemDrop")
	TObjectPtr<USphereComponent> SphereCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|ItemDrop")
	FName ItemId = TEXT("HealthPotion");

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly,
		Category = "NF|Monster|ItemDrop",
		meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,
		Category = "NF|Monster|ItemDrop",
		meta = (ClampMin = "0.0", Units = "s"))
	float DropLifeSpan = 30.0f;
};
