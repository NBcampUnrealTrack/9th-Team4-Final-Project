#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "NF_ArmorComponent.generated.h"

class ANF_ArmorBase;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NF_API UNF_ArmorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNF_ArmorComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	bool EquipArmor(ANF_ArmorBase* NewArmor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	ANF_ArmorBase* UnequipArmor(ENF_ArmorSlot Slot);

	UFUNCTION(BlueprintPure, Category="Armor")
	ANF_ArmorBase* GetEquippedArmor(ENF_ArmorSlot Slot) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="Armor")
	TArray<TSubclassOf<ANF_ArmorBase>> DefaultArmorClasses;

	UPROPERTY(EditDefaultsOnly, Category="Armor|Sockets")
	FName HeadSocketName = TEXT("head");

	UPROPERTY(EditDefaultsOnly, Category="Armor|Sockets")
	FName UpperSocketName = TEXT("spine_03");

	UPROPERTY(EditDefaultsOnly, Category="Armor|Sockets")
	FName BottomSocketName = TEXT("pelvis");

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Armor")
	TArray<TObjectPtr<ANF_ArmorBase>> EquippedArmors;

private:
	FName GetSocketName(ENF_ArmorSlot Slot) const;
	ANF_ArmorBase* UnequipArmorInternal(ENF_ArmorSlot Slot);
	bool bChangingArmor = false;
};
