#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_BuildShopZone.generated.h"

class UBoxComponent;

/** Place at spawn/base or field shops. Area eligibility is checked again on the server. */
UCLASS()
class PROJECT_NF_API ANF_BuildShopZone : public AActor
{
	GENERATED_BODY()

public:
	ANF_BuildShopZone();

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

	/** Overlap events are UI prompts only; they never authorize a build request. */
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|BuildShop")
	void OnPlayerEntered(AActor* PlayerActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "NF|BuildShop")
	void OnPlayerExited(AActor* PlayerActor);

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);
};
