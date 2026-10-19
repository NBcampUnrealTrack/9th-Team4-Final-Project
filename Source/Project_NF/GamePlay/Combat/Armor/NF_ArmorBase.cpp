#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "Components/StaticMeshComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"

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

void ANF_ArmorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyArmorAppearance();
}

void ANF_ArmorBase::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyArmorAppearance();
}

ENF_ArmorSlot ANF_ArmorBase::GetArmorSlot() const
{
	if (IsValid(ArmorData) == false) 
		return ENF_ArmorSlot::Head;
	
	return ArmorData->ArmorSlot;
}

float ANF_ArmorBase::GetAdditionalDefense() const
{
	if (IsValid(ArmorData) == false)
		return 0.f;
	
	return ArmorData->AdditionalDefense;
}

void ANF_ArmorBase::ApplyArmorAppearance()
{
	if (IsValid(ArmorMesh) == false) return;

	ArmorMesh->SetStaticMesh(IsValid(ArmorData) ? ArmorData->ArmorMesh.Get() : nullptr);
	ArmorMesh->EmptyOverrideMaterials();
}

