#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "NF_TargetActorTrace.generated.h"

UCLASS()
class PROJECT_NF_API ANF_TargetActorTrace : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()

public:
	ANF_TargetActorTrace();
	
	virtual  void StartTargeting(UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;
	
protected:
	//UFUNCTION(BlueprintImplementableEvent)
	FHitResult GetSphereTraceResult(AActor* InSourceActor);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Range = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Radius = 50.0f;
	
	
	
private:
	
	
	
};
