#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_GrabHookVisual.generated.h"

class ANF_GrabHook;
class UStaticMeshComponent;

/** Local presentation only. Destruction/retraction never controls hits or character movement. */
UCLASS(Blueprintable, Transient)
class PROJECT_NF_API ANF_GrabHookVisual : public AActor
{
	GENERATED_BODY()
public:
	ANF_GrabHookVisual();
	void Initialize(ANF_GrabHook* Hook, AActor* Source);
	void BeginReturn();
	virtual void Tick(float DeltaSeconds) override;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Grab Visual")
	TObjectPtr<UStaticMeshComponent> HookMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Grab Visual")
	TObjectPtr<UStaticMeshComponent> TetherMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Visual", meta = (ClampMin = "1", Units = "cm/s"))
	float ReturnSpeed = 2500.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Visual", meta = (ClampMin = "0.1", Units = "cm"))
	float TetherRadius = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Visual")
	bool bShowTether = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Visual|Anchor")
	FName SourceMeshComponentName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NF|Grab Visual|Anchor")
	FName SourceSocketName;
private:
	FVector GetAnchor() const;
	TWeakObjectPtr<ANF_GrabHook> Simulation;
	TWeakObjectPtr<AActor> SourceActor;
	FVector FallbackOffset = FVector(40, 20, -20);
	bool bReturning = false;
};
