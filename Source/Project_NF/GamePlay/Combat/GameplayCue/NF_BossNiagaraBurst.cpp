#include "GamePlay/Combat/GameplayCue/NF_BossNiagaraBurst.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

ANF_BossNiagaraBurst::ANF_BossNiagaraBurst()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

ANF_BossNiagaraBurst* ANF_BossNiagaraBurst::SpawnBurst(AActor* AuthorityActor, UNiagaraSystem* System,
	FVector Location, FRotator Rotation, FVector Scale)
{
	if (!IsValid(AuthorityActor) || !AuthorityActor->HasAuthority() || !System || !AuthorityActor->GetWorld()) return nullptr;
	const FTransform Transform(Rotation, Location);
	auto* Burst = AuthorityActor->GetWorld()->SpawnActorDeferred<ANF_BossNiagaraBurst>(StaticClass(), Transform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Burst) return nullptr;
	Burst->NiagaraSystem = System;
	Burst->EffectScale = Scale;
	Burst->FinishSpawning(Transform);
	return Burst;
}

void ANF_BossNiagaraBurst::BeginPlay()
{
	Super::BeginPlay();
	PlayBurst();
	// The independent world-space Niagara component auto-destroys on completion.
	if (HasAuthority()) SetLifeSpan(3.f);
}

void ANF_BossNiagaraBurst::OnRep_System()
{
	if (HasActorBegunPlay()) PlayBurst();
}

void ANF_BossNiagaraBurst::PlayBurst()
{
	if (bPlayed || !NiagaraSystem || GetNetMode() == NM_DedicatedServer) return;
	bPlayed = true;
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, NiagaraSystem, GetActorLocation(), GetActorRotation(),
		EffectScale, true, true, ENCPoolMethod::AutoRelease);
}

void ANF_BossNiagaraBurst::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANF_BossNiagaraBurst, NiagaraSystem, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_BossNiagaraBurst, EffectScale, COND_InitialOnly);
}
