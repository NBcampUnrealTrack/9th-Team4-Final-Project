#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/Sonar/NF_SilhouetteView.h"
#include "NF_SonarReveal.generated.h"

class APlayerController;

USTRUCT()
struct FNF_SonarRevealFrame
{
	GENERATED_BODY()
	UPROPERTY()
	FGuid SourceId;
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> Targets;
	UPROPERTY()
	TObjectPtr<UMaterialInterface> Material;
	UPROPERTY()
	FNF_SilhouetteStyle Style;
	UPROPERTY()
	double EndsAtServerTime = 0.0;
	UPROPERTY()
	bool bActive = false;
};

/** Owner-only delivery. No NF controller dependency, RPC additions, or target asset edits. */
UCLASS(NotBlueprintable, Transient)
class PROJECT_NF_API ANF_SonarReveal : public AActor
{
	GENERATED_BODY()
public:
	ANF_SonarReveal();
	bool Initialize(APlayerController* Recipient, const TArray<TObjectPtr<ACharacter>>& Targets, float Duration,
		UMaterialInterface* Material, const FNF_SilhouetteStyle& Style);
	void StopReveal();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	UFUNCTION()
	void OnRep_Frame();
	void RefreshLocalView();
	UPROPERTY(ReplicatedUsing = OnRep_Frame)
	FNF_SonarRevealFrame Frame;
	TWeakObjectPtr<ANF_SilhouetteView> LocalView;
	double ServerExpiresAt = 0.0;
};
