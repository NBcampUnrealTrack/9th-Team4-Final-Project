#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NF_BossNiagaraBurst.generated.h"

class UNiagaraSystem;

/** One-shot cosmetic relay independent of the missile/GA lifetime. Use non-looping systems. */
UCLASS(NotBlueprintable, Transient)
class PROJECT_NF_API ANF_BossNiagaraBurst : public AActor
{
	GENERATED_BODY()
public:
	ANF_BossNiagaraBurst();
	/** Call on the server from a GA/projectile BP event. Each relevant client plays the system locally. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Boss|VFX", meta = (DisplayName = "Spawn Replicated Boss Niagara"))
	static ANF_BossNiagaraBurst* SpawnBurst(AActor* AuthorityActor, UNiagaraSystem* System,
		FVector Location, FRotator Rotation, FVector Scale);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
private:
	UFUNCTION()
	void OnRep_System();
	void PlayBurst();
	UPROPERTY(ReplicatedUsing = OnRep_System)
	TObjectPtr<UNiagaraSystem> NiagaraSystem;
	UPROPERTY(Replicated)
	FVector EffectScale = FVector::OneVector;
	bool bPlayed = false;
};
