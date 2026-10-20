#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "Components/StaticMeshComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"
#include "Net/UnrealNetwork.h"

ANF_ArmorBase::ANF_ArmorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	// Logical equipment is not attached spatially; relevance must follow the wearer.
	bNetUseOwnerRelevancy = true;
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
		return ENF_ArmorSlot::Unknown;
	
	return ArmorData->ArmorSlot;
}

float ANF_ArmorBase::GetAdditionalDefense() const
{
	if (IsValid(ArmorData) == false)
		return 0.f;
	
	return FMath::Max(0.f, ArmorData->AdditionalDefence);
}

void ANF_ArmorBase::ApplyArmorAppearance()
{
	if (IsValid(ArmorMesh) == false) return;

	ArmorMesh->SetStaticMesh(bShowArmorAppearance && IsValid(ArmorData) ? ArmorData->ArmorMesh.Get() : nullptr);
	ArmorMesh->EmptyOverrideMaterials();
}

bool ANF_ArmorBase::SetArmorData(UNF_ArmorDataAsset* NewData)
{
	if (!HasAuthority() || !IsValid(NewData)) return false;
	if (NewData->ArmorSlot != ENF_ArmorSlot::HeadArmor && NewData->ArmorSlot != ENF_ArmorSlot::BodyArmor &&
		NewData->ArmorSlot != ENF_ArmorSlot::LegArmor) return false;
	if (IsValid(ArmorData) && ArmorData->ArmorSlot != NewData->ArmorSlot) return false;
	ArmorData = NewData;
	OnRep_ArmorData();
	ForceNetUpdate();
	return true;
}

void ANF_ArmorBase::OnRep_ArmorData()
{
	ApplyArmorAppearance();
}

void ANF_ArmorBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_ArmorBase, ArmorData);
}

