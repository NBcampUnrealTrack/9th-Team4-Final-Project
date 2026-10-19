#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

UNF_ArmorComponent::UNF_ArmorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNF_ArmorComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	
	if (IsValid(Character) == false || Character->HasAuthority() == false)
		return;

	for (const TSubclassOf<ANF_ArmorBase>& ArmorClass : DefaultArmorClasses)
	{
		if (ArmorClass == nullptr)
			continue;

		const ANF_ArmorBase* DefaultArmor = ArmorClass->GetDefaultObject<ANF_ArmorBase>();
		if (IsValid(DefaultArmor->GetArmorData()) == false || GetEquippedArmor(DefaultArmor->GetArmorSlot()))
			continue;

		FActorSpawnParameters Params;
		Params.Owner = Character;
		Params.Instigator = Character;
		ANF_ArmorBase* Armor = GetWorld()->SpawnActor<ANF_ArmorBase>(ArmorClass, Character->GetActorTransform(), Params);
		
		if (Armor && EquipArmor(Armor) == false)
			Armor->Destroy();
	}
}

FName UNF_ArmorComponent::GetSocketName(ENF_ArmorSlot Slot) const
{
	switch (Slot)
	{
	case ENF_ArmorSlot::Head: return HeadSocketName;
	case ENF_ArmorSlot::Upper: return UpperSocketName;
	case ENF_ArmorSlot::Bottom: return BottomSocketName;
	default: return NAME_None;
	}
}

ANF_ArmorBase* UNF_ArmorComponent::GetEquippedArmor(ENF_ArmorSlot Slot) const
{
	for (ANF_ArmorBase* Armor : EquippedArmors)
	{
		if (IsValid(Armor) && Armor->GetArmorSlot() == Slot) return Armor;
	}
	return nullptr;
}

bool UNF_ArmorComponent::EquipArmor(ANF_ArmorBase* NewArmor)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	
	if (IsValid(Character) == false || Character->HasAuthority() == false || bChangingArmor || 
		IsValid(NewArmor) == false) 
		return false;

	if (IsValid(NewArmor->GetArmorData()) == false)
		return false;
	
	if (NewArmor->GetWorld() != GetWorld() || IsValid(NewArmor->GetRootComponent()) == false || 
		IsValid(Character->GetMesh()) == false)
		return false;
	
	if (NewArmor->GetOwner() && NewArmor->GetOwner() != Character) 
		return false;
	
	if (NewArmor->GetAttachParentActor() && NewArmor->GetAttachParentActor() != Character) 
		return false;
	

	const ENF_ArmorSlot Slot = NewArmor->GetArmorSlot();
	const FName SocketName = GetSocketName(Slot);
	if (SocketName.IsNone() || Character->GetMesh()->DoesSocketExist(SocketName) == false)
		return false;
	
	if (GetEquippedArmor(Slot) == NewArmor)
		return true;

	TGuardValue<bool> ChangeGuard(bChangingArmor, true);
	if (NewArmor->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName) == false)
		return false;
	

	UnequipArmorInternal(Slot);
	EquippedArmors.RemoveAll([](const TObjectPtr<ANF_ArmorBase>& Armor) { return !IsValid(Armor.Get()); });
	EquippedArmors.Add(NewArmor);
	NewArmor->SetOwner(Character);
	NewArmor->SetInstigator(Character);
	NewArmor->ForceNetUpdate();
	Character->ForceNetUpdate();
	
	return true;
}

ANF_ArmorBase* UNF_ArmorComponent::UnequipArmor(ENF_ArmorSlot Slot)
{
	if (IsValid(GetOwner()) == false || GetOwner()->HasAuthority() == false || bChangingArmor)
		return nullptr;
	
	TGuardValue<bool> ChangeGuard(bChangingArmor, true);
	
	return UnequipArmorInternal(Slot);
}

ANF_ArmorBase* UNF_ArmorComponent::UnequipArmorInternal(ENF_ArmorSlot Slot)
{
	ANF_ArmorBase* Armor = GetEquippedArmor(Slot);
	
	if (Armor == nullptr)
		return nullptr;
	
	EquippedArmors.Remove(Armor);
	Armor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Armor->SetOwner(nullptr);
	Armor->SetInstigator(nullptr);
	Armor->ForceNetUpdate();
	GetOwner()->ForceNetUpdate();
	
	return Armor;
}

void UNF_ArmorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(GetOwner()) && GetOwner()->HasAuthority())
	{
		TGuardValue<bool> ChangeGuard(bChangingArmor, true);
		const TArray<TObjectPtr<ANF_ArmorBase>> Armors = MoveTemp(EquippedArmors);
		for (ANF_ArmorBase* Armor : Armors)
		{
			if (IsValid(Armor)) Armor->Destroy();
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

void UNF_ArmorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNF_ArmorComponent, EquippedArmors);
}

