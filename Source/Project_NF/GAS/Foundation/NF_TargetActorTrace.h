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
	virtual void Tick(float DeltaTime) override;
	
#pragma region TestTrace
	/*UFUNCTION(BlueprintCallable)
	FHitResult GetTestTraceResult(AActor* InSourceActor);*/
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TestTrace")
	float SphereTraceRange = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TestTrace")
	float SphereTraceRadius = 50.0f;
#pragma endregion 
	
	
#pragma region MeleeTrace
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MeleeTrace")
	int32 SubStepCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MeleeTrace")
	FName MeleeSocket_Start = TEXT("MeleeSocket_Start");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MeleeTrace")
	FName MeleeSocket_End = TEXT("MeleeSocket_End");
	
	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;
	
	UPROPERTY()
	TObjectPtr<UMeshComponent> WeaponMesh;
	
	UMeshComponent* GetWeaponMeshFromSourceActor(AActor* InSourceActor) const;
	
	UFUNCTION(BlueprintCallable)
	TArray<FHitResult> GetMeleeTraceResults(AActor* InSourceActor);
	
	void ResetMeleeTrace();

	float MeleeTraceRadius = 20.0f;
	FVector PreviousStartLocation = FVector::ZeroVector;
	FVector PreviousEndLocation = FVector::ZeroVector;
	bool bIsPreviousLocationValid = false;
#pragma endregion 
	
};
