#include "Monster/NF_MonsterItemDrop.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

ANF_MonsterItemDrop::ANF_MonsterItemDrop()
{
	bReplicates = true;
	SetReplicateMovement(false);

	DropRootComponent =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("DropRootComponent"));

	SetRootComponent(DropRootComponent);

	ItemMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(
			TEXT("ItemMesh"));

	ItemMesh->SetupAttachment(DropRootComponent);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SphereCollision =
		CreateDefaultSubobject<USphereComponent>(
			TEXT("SphereCollision"));

	SphereCollision->SetupAttachment(DropRootComponent);
	SphereCollision->SetSphereRadius(80.0f);
	SphereCollision->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);

	SphereCollision->SetCollisionResponseToAllChannels(
		ECR_Ignore);

	SphereCollision->SetCollisionResponseToChannel(
		ECC_Pawn,
		ECR_Overlap);

	SphereCollision->SetGenerateOverlapEvents(true);

}

void ANF_MonsterItemDrop::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && DropLifeSpan > 0.0f)
	{
		SetLifeSpan(DropLifeSpan);
	}	
}

void ANF_MonsterItemDrop::InitializeDrop(
	const int32 InQuantity)
{
	if (!HasAuthority())
	{
		return;
	}

	Quantity = FMath::Max(InQuantity, 1);
}

void ANF_MonsterItemDrop::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANF_MonsterItemDrop, Quantity);
}

