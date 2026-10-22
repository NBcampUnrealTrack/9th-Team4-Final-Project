#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "Data/Struct/NF_EscapeState.h"
#include "NF_EscapeZone.generated.h"

class APawn;
class USphereComponent;
class UPrimitiveComponent;

UCLASS()
class PROJECT_NF_API ANF_EscapeZone : public AActor, public INF_InteractionInterface
{
	GENERATED_BODY()

public:
	ANF_EscapeZone();

	virtual void NF_Interaction(const APlayerController* InteractedController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool ContainsPawn(const APawn* Pawn) const;
	float GetTeamEscapeDuration() const;
	float GetSoloEscapeDuration() const;
	void AuthoritySetProgress(const FNF_EscapeProgress& NewProgress);

	UFUNCTION(BlueprintPure, Category = "NF|Escape")
	FNF_EscapeProgress GetEscapeProgress() const;

protected:
	// 클라이언트 UI와 문 연출용이며 서버 판정은 여기서 처리하지 않는다.
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Escape")
	void OnEscapeProgressChanged(const FNF_EscapeProgress& NewProgress);

private:
	UPROPERTY(VisibleAnywhere, Category = "NF|Escape")
	TObjectPtr<USphereComponent> EscapeRange;

	UPROPERTY(EditAnywhere, Category = "NF|Escape", meta = (ClampMin = "0.1"))
	float TeamEscapeDuration = 10.0f;

	UPROPERTY(EditAnywhere, Category = "NF|Escape", meta = (ClampMin = "0.1"))
	float SoloEscapeDuration = 20.0f;

	UPROPERTY(ReplicatedUsing = OnRep_EscapeProgress)
	FNF_EscapeProgress EscapeProgress;

	UFUNCTION()
	void OnRep_EscapeProgress();

	UFUNCTION()
	void HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
