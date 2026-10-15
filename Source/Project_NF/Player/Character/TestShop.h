#pragma once

#include "CoreMinimal.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "GameFramework/Actor.h"
#include "TestShop.generated.h"

UCLASS()
class PROJECT_NF_API ATestShop : public AActor, public INF_InteractionInterface
{
	GENERATED_BODY()

public:
	ATestShop();
	
	virtual void NF_Interaction(const APlayerController* InteractedController) override;
	
	void OpenShop(const APlayerController* InteractedController);

};
