#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_BossLineTelegraph.generated.h"

class UDecalComponent;
class UMaterialInterface;

/** Replicated, stationary rectangular warning. Its owning ability controls its lifetime. */
UCLASS(NotPlaceable)
class PROJECT_NF_API ANF_BossLineTelegraph : public AActor
{
	GENERATED_BODY()
public:
	ANF_BossLineTelegraph();
	void Initialize(UMaterialInterface* Material, FVector Extent);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
private:
	UFUNCTION() void UpdateDecal();
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDecalComponent> Decal;
	UPROPERTY(ReplicatedUsing=UpdateDecal) TObjectPtr<UMaterialInterface> WarningMaterial;
	UPROPERTY(ReplicatedUsing=UpdateDecal) FVector BoxExtent = FVector::ZeroVector;
};
