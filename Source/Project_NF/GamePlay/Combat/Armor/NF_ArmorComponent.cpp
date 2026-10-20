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

	ConfigureProjectileCollision();

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
	if (Slot == ENF_ArmorSlot::Unknown) return nullptr;
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
	
	if (NewArmor->GetWorld() != GetWorld() || IsValid(NewArmor->GetRootComponent()) == false)
		return false;
	
	if (NewArmor->GetOwner() && NewArmor->GetOwner() != Character) 
		return false;
	
	if (NewArmor->GetAttachParentActor() && NewArmor->GetAttachParentActor() != Character) 
		return false;
	

	const ENF_ArmorSlot Slot = NewArmor->GetArmorSlot();
	if (Slot != ENF_ArmorSlot::Head && Slot != ENF_ArmorSlot::Upper && Slot != ENF_ArmorSlot::Bottom) return false;
	const FName SocketName = GetSocketName(Slot);
	if (bAttachArmorToSockets && (!IsValid(Character->GetMesh()) || SocketName.IsNone() ||
		!Character->GetMesh()->DoesSocketExist(SocketName)))
		return false;
	
	if (GetEquippedArmor(Slot) == NewArmor)
		return true;

	TGuardValue<bool> ChangeGuard(bChangingArmor, true);
	if (bAttachArmorToSockets && !NewArmor->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName))
		return false;
	if (!bAttachArmorToSockets) NewArmor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	

	UnequipArmorInternal(Slot);
	EquippedArmors.RemoveAll([](const TObjectPtr<ANF_ArmorBase>& Armor) { return !IsValid(Armor.Get()); });
	EquippedArmors.Add(NewArmor);
	NewArmor->SetOwner(Character);
	NewArmor->SetInstigator(Character);
	NewArmor->ForceNetUpdate();
	Character->ForceNetUpdate();
	
	return true;
}

ENF_ArmorSlot UNF_ArmorComponent::ResolveHitRegion(FName BoneName) const
{
	if (BoneName.IsNone()) return ENF_ArmorSlot::Unknown;
	const USkeletalMeshComponent* HitMesh = GetHitMesh();
	if (!IsValid(HitMesh) || HitMesh->GetBoneIndex(BoneName) == INDEX_NONE)
		return ENF_ArmorSlot::Unknown;

	// Closest entry wins, including an explicit Unknown override.
	// Only real bones from the selected hit mesh can participate in this lookup.
	while (!BoneName.IsNone())
	{
		if (const ENF_ArmorSlot* Slot = BoneToSlot.Find(BoneName)) return *Slot;
		BoneName = HitMesh->GetParentBone(BoneName);
	}
	return ENF_ArmorSlot::Unknown;
}

float UNF_ArmorComponent::GetAdditionalDefenseForSlot(ENF_ArmorSlot Slot) const
{
	const ANF_ArmorBase* Armor = GetEquippedArmor(Slot);
	return Armor ? Armor->GetAdditionalDefense() : 0.f;
}

USkeletalMeshComponent* UNF_ArmorComponent::GetHitMesh() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character) return nullptr;
	if (HitMeshComponentName.IsNone()) return Character->GetMesh();
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(Character);
	for (USkeletalMeshComponent* Mesh : Meshes)
		if (Mesh->GetFName() == HitMeshComponentName) return Mesh;
	return nullptr;
}

void UNF_ArmorComponent::ConfigureProjectileCollision()
{
	USkeletalMeshComponent* HitMesh = GetHitMesh();
	if (!IsValid(HitMesh) || !HitMesh->GetPhysicsAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponHit] %s: hit mesh/Physics Asset missing. Configure ArmorComponent HitMeshComponentName; bone hits unavailable."), *GetNameSafe(GetOwner()));
		return;
	}
	// Only the new projectile channel is changed. Capsule movement and other attacks are unaffected.
	TInlineComponentArray<UPrimitiveComponent*> Primitives(GetOwner());
	for (UPrimitiveComponent* Primitive : Primitives)
		Primitive->SetCollisionResponseToChannel(NF_WeaponProjectileChannel, Primitive == HitMesh ? ECR_Block : ECR_Ignore);
	if (HitMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
		HitMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	else if (HitMesh->GetCollisionEnabled() == ECollisionEnabled::PhysicsOnly)
		HitMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HitMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
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

