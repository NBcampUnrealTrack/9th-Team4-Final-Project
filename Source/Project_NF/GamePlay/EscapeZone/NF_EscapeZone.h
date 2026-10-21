#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "NF_EscapeZone.generated.h"

class APawn;
class USphereComponent;
class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct FNF_EscapeProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bInProgress = false;

	UPROPERTY(BlueprintReadOnly)
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly)
	double EndServerTime = 0.0;
};

UCLASS()
class PROJECT_NF_API ANF_EscapeZone : public AActor, public INF_InteractionInterface
{
	GENERATED_BODY()

public:
	ANF_EscapeZone();

	virtual void NF_Interaction(const APlayerController* InteractedController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool ContainsPawn(const APawn* Pawn) const;
	float GetEscapeDuration() const;
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
	float EscapeDuration = 5.0f;

	UPROPERTY(ReplicatedUsing = OnRep_EscapeProgress)
	FNF_EscapeProgress EscapeProgress;

	UFUNCTION()
	void OnRep_EscapeProgress();

	UFUNCTION()
	void HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
