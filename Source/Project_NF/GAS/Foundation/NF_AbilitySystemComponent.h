#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "NF_AbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UNF_AbilitySystemComponent();

protected:
	virtual void BeginPlay() override;

public:
};
