#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "Components/StaticMeshComponent.h"

ANF_ArmorBase::ANF_ArmorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	ArmorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArmorMesh"));
	SetRootComponent(ArmorMesh);
	ArmorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArmorMesh->SetGenerateOverlapEvents(false);
}

