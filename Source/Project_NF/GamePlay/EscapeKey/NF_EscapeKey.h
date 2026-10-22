#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "NF_EscapeKey.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UNF_EscapeKeyComponent;

UCLASS()
class PROJECT_NF_API ANF_EscapeKey : public AActor, public INF_InteractionInterface
{
	GENERATED_BODY()

public:
	ANF_EscapeKey();

	void InitializePickup(UNF_EscapeKeyComponent* InKeyComponent);
	void ActivatePickup();

	virtual void NF_Interaction(const APlayerController* InteractedController) override;

	bool IsPickupEnabled() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "NF|Key")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "NF|Key")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	TWeakObjectPtr<UNF_EscapeKeyComponent> KeyComponent;

	bool bPickupEnabled = false;
};