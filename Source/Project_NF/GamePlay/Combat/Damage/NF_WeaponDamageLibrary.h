#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NF_WeaponDamageLibrary.generated.h"

class UAbilitySystemComponent;
UCLASS()
class PROJECT_NF_API UNF_WeaponDamageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// @Owner: 이정준 (구현: Codex) | 2026-10-21
	static bool EnsureOffenseAttributes(UAbilitySystemComponent* ASC);
	UFUNCTION(BlueprintPure, Category = "NF|Weapon|Damage")
	static float CalculateShotDamage(AActor* Shooter, float BaseDamage, bool bLastRound);
	static float EvaluateDamage(float BaseDamage, double Bonus, double Multiplier, bool bLastRound);
};
