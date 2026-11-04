#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponentConfigDataAsset.h"
#include "GamePlay/Combat/Armor/NF_ArmorBase.h"
#include "GamePlay/Combat/Armor/NF_ArmorDataAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Data/NF_EquipmentGEDataAsset.h"
#include "GAS/Manager/NF_GEManagerSubsystem.h"
#include "GAS/Tags/NFTag_Armor.h"
#include "Player/Character/NF_PlayerStatusAttribute.h"

UNF_ArmorComponent::UNF_ArmorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UNF_ArmorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Config) == false)
		return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	
	if (IsValid(Character) == false || Character->HasAuthority() == false)
		return;

	ConfigureProjectileCollision();

	for (const TSubclassOf<ANF_ArmorBase>& ArmorClass : Config->DefaultArmorClasses)
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
	if (IsValid(Config) == false)
		return NAME_None;

	switch (Slot)
	{
	case ENF_ArmorSlot::HeadArmor: return Config->HeadSocketName;
	case ENF_ArmorSlot::BodyArmor: return Config->UpperSocketName;
	case ENF_ArmorSlot::LegArmor: return Config->BottomSocketName;
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
	if (IsValid(Config) == false)
		return false;

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
	if (Slot != ENF_ArmorSlot::HeadArmor && Slot != ENF_ArmorSlot::BodyArmor && Slot != ENF_ArmorSlot::LegArmor) return false;
	const FName SocketName = GetSocketName(Slot);
	if (Config->bAttachArmorToSockets && (!IsValid(Character->GetMesh()) || SocketName.IsNone() ||
		!Character->GetMesh()->DoesSocketExist(SocketName)))
		return false;
	
	if (GetEquippedArmor(Slot) == NewArmor)
		return true;

	TGuardValue<bool> ChangeGuard(bChangingArmor, true);
	if (Config->bAttachArmorToSockets && !NewArmor->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName))
		return false;
	if (Config->bAttachArmorToSockets == false)
	{
		NewArmor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	UnequipArmorInternal(Slot);
	EquippedArmors.RemoveAll([](const TObjectPtr<ANF_ArmorBase>& Armor) { return !IsValid(Armor.Get()); });
	EquippedArmors.Add(NewArmor);
	NewArmor->SetOwner(Character);
	NewArmor->SetInstigator(Character);
	NewArmor->ForceNetUpdate();
	Character->ForceNetUpdate();
	RefreshArmorDefence();
	
	return true;
}

ENF_ArmorSlot UNF_ArmorComponent::ResolveHitRegion(FName BoneName) const
{
	if (IsValid(Config) == false || BoneName.IsNone())
		return ENF_ArmorSlot::Unknown;
	
	const USkeletalMeshComponent* HitMesh = GetHitMesh();
	if (IsValid(HitMesh) == false || HitMesh->GetBoneIndex(BoneName) == INDEX_NONE)
		return ENF_ArmorSlot::Unknown;

	const ENF_ArmorSlot* Slot = Config->BoneToSlot.Find(BoneName);
	return Slot ? *Slot : ENF_ArmorSlot::Unknown;
}

float UNF_ArmorComponent::GetAdditionalDefenseForSlot(ENF_ArmorSlot Slot) const
{
	const ANF_ArmorBase* Armor = GetEquippedArmor(Slot);
	return Armor ? Armor->GetAdditionalDefence() : 0.f;
}

float UNF_ArmorComponent::GetTotalBaseDefence() const
{
	float TotalBaseDefence = 0.f;
	for (const ANF_ArmorBase* Armor : EquippedArmors)
	{
		if (IsValid(Armor) == false)
			continue;

		TotalBaseDefence += Armor->GetBaseDefence();
	}

	return TotalBaseDefence;
}

void UNF_ArmorComponent::ClearArmorDefence()
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return;
	if (DefenceASC.IsValid() && DefenceEffectHandle.IsValid())
	{
		DefenceASC->RemoveActiveGameplayEffect(DefenceEffectHandle);
	}
	DefenceEffectHandle.Invalidate();
	DefenceASC.Reset();
}

void UNF_ArmorComponent::RefreshArmorDefence()
{
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority()) return;
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner(), true);
	if (DefenceASC.Get() != ASC) ClearArmorDefence();
	if (!IsValid(ASC) || ASC->GetAvatarActor() != GetOwner() || !ASC->GetSet<UNF_PlayerStatusAttribute>())
	{
		ClearArmorDefence();
		return;
	}

	const float TotalDefence = GetTotalBaseDefence();
	if (!FMath::IsFinite(TotalDefence) || TotalDefence <= 0.f)
	{
		ClearArmorDefence();
		return;
	}
	if (DefenceEffectHandle.IsValid() && ASC->GetActiveGameplayEffect(DefenceEffectHandle))
	{
		ASC->UpdateActiveGameplayEffectSetByCallerMagnitude(
			DefenceEffectHandle, NFTag_Armor::Data_Armor_BaseDefence, TotalDefence);
		return;
	}

	const auto ArmorDefenceGEClass = UNF_GEManagerSubsystem::FindEffect(
		this, &UNF_EquipmentGEDataAsset::NFGE_ArmorDefence);
	if (!ArmorDefenceGEClass) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ArmorDefenceGEClass, 1.f, Context);
	if (Spec.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(NFTag_Armor::Data_Armor_BaseDefence, TotalDefence);
		DefenceASC = ASC;
		DefenceEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

USkeletalMeshComponent* UNF_ArmorComponent::GetHitMesh() const
{
	if (IsValid(Config) == false)
		return nullptr;

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	
	if (Character == nullptr)
		return nullptr;
	
	if (Config->HitMeshComponentName.IsNone()) 
		return Character->GetMesh();
	
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(Character);
	
	for (USkeletalMeshComponent* Mesh : Meshes)
		if (Mesh->GetFName() == Config->HitMeshComponentName) return Mesh;
	
	return nullptr;
}

void UNF_ArmorComponent::ConfigureProjectileCollision()
{
	USkeletalMeshComponent* HitMesh = GetHitMesh();
	
	if (IsValid(HitMesh) == false || HitMesh->GetPhysicsAsset() == nullptr)
		return;
	
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
	
	ANF_ArmorBase* Armor = UnequipArmorInternal(Slot);
	RefreshArmorDefence();
	return Armor;
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
		ClearArmorDefence();
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

