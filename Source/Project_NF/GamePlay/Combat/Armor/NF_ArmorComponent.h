#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorTypes.h"
#include "NF_ArmorComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECT_NF_API UNF_ArmorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UNF_ArmorComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	bool EquipArmor(class ANF_ArmorBase* NewArmor);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Armor")
	ANF_ArmorBase* UnequipArmor(ENF_ArmorSlot Slot);

	UFUNCTION(BlueprintPure, Category="Armor")
	ANF_ArmorBase* GetEquippedArmor(ENF_ArmorSlot Slot) const;

	UFUNCTION(BlueprintPure, Category="Armor|Hit Regions")
	ENF_ArmorSlot ResolveHitRegion(FName BoneName) const;

	UFUNCTION(BlueprintPure, Category="Armor|Hit Regions")
	float GetAdditionalDefenseForSlot(ENF_ArmorSlot Slot) const;

	class USkeletalMeshComponent* GetHitMesh() const;

	/** Match only the hit bone's name on the selected mesh; parents are not searched.
	 * Missing or unmapped bones return Unknown and receive no armor defense. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Armor|Hit Regions")
	TMap<FName, ENF_ArmorSlot> BoneToSlot;

	/** None uses Character.GetMesh(). Set VisualMesh to use NF_Player's visible mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Armor|Hit Regions")
	FName HitMeshComponentName;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="Armor")
	TArray<TSubclassOf<class ANF_ArmorBase>> DefaultArmorClasses;

	/** False allows logical equipment without a mesh, bone or socket. */
	UPROPERTY(EditDefaultsOnly, Category="Armor|Sockets")
	bool bAttachArmorToSockets = false;

	// 메시를 붙일 소켓 이름 (피격 부위 판정x)
	UPROPERTY(EditDefaultsOnly, Category="Armor|Sockets")
	FName HeadSocketName = TEXT("head");

	UPROPERTY(EditDefaultsOnly, Category="Armor|Sockets")
	FName UpperSocketName = TEXT("spine_03");

	UPROPERTY(EditDefaultsOnly, Category="Armor|Sockets")
	FName BottomSocketName = TEXT("pelvis");

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category="Armor")
	TArray<TObjectPtr<class ANF_ArmorBase>> EquippedArmors;

private:
	void ConfigureProjectileCollision();
	FName GetSocketName(ENF_ArmorSlot Slot) const;
	class ANF_ArmorBase* UnequipArmorInternal(ENF_ArmorSlot Slot);
	bool bChangingArmor = false;
};
