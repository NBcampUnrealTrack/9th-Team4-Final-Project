#pragma once

#include "CoreMinimal.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "GameFramework/Actor.h"
#include "NF_BuildShopZone.generated.h"

class UBoxComponent;

/** Place at spawn/base or field shops. Area eligibility is checked again on the server. */
UCLASS()
class PROJECT_NF_API ANF_BuildShopZone : public AActor, public INF_InteractionInterface
{
	GENERATED_BODY()

public:
	ANF_BuildShopZone();
	virtual void NF_Interaction(const APlayerController* InteractedController) override;

	UFUNCTION(BlueprintPure, Category = "NF|BuildShop")
	bool CanBuild(AActor* PlayerActor) const;

	/** Team ownership and match phase restrictions belong to their respective systems. */
	UFUNCTION(BlueprintNativeEvent, Category = "NF|BuildShop")
	bool IsPlayerAllowed(AActor* PlayerActor) const;
	virtual bool IsPlayerAllowed_Implementation(AActor* PlayerActor) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|BuildShop")
	void SetShopEnabled(bool bEnabled);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|BuildShop")
	TObjectPtr<UBoxComponent> BuildArea;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "NF|BuildShop")
	bool bShopEnabled = true;

	/** Measured from the player to this actor. Interaction trace collision is separate from authorization. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NF|BuildShop", meta = (ClampMin = "0"))
	float InteractionDistance = 300.f;
};
