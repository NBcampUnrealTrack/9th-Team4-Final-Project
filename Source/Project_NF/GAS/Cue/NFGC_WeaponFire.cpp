#include "GAS/Cue/NFGC_WeaponFire.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/Audio/NF_AudioSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponCameraShakeBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

UNFGC_WeaponFire::UNFGC_WeaponFire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UNFGC_WeaponFire::HandlesEvent(EGameplayCueEvent::Type EventType) const
{
	return EventType == EGameplayCueEvent::Executed;
}

bool UNFGC_WeaponFire::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	// 발사 이펙트는 로컬 클라에서만
	UWorld* World = IsValid(MyTarget) ? MyTarget->GetWorld() : nullptr;
	if (IsValid(World) == false || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const UNF_WeaponDataAsset* Data = Cast<UNF_WeaponDataAsset>(Parameters.SourceObject.Get());
	if (IsValid(Data) == false)
	{
		return false;
	}

	if (IsValid(Data->FireSound))
	{
		if (UNF_AudioSubsystem* Audio = UNF_AudioSubsystem::Get(MyTarget))
		{
			Audio->PlaySFXAtLocation(Data->FireSound, Parameters.Location, Parameters.Normal.Rotation());
		}
	}

	if (IsValid(Data->MuzzleFlash))
	{
		const ANF_WeaponBase* Weapon = Cast<ANF_WeaponBase>(Parameters.EffectCauser.Get());
		USkeletalMeshComponent* Mesh = IsValid(Weapon) ? Weapon->GetWeaponMesh() : nullptr;
		if (IsValid(Mesh) && Mesh->DoesSocketExist(Data->MuzzleSocketName))
		{
			UGameplayStatics::SpawnEmitterAttached(Data->MuzzleFlash, Mesh, Data->MuzzleSocketName,
				FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
				EAttachLocation::SnapToTarget, true);
		}
	}

	APawn* Shooter = Cast<APawn>(Parameters.Instigator.Get());
	if (IsValid(Shooter) == false)
	{
		Shooter = Cast<APawn>(MyTarget);
	}
	if (Data->FireCameraShakeClass && IsValid(Shooter) && Shooter->IsLocallyControlled())
	{
		APlayerController* PC = Cast<APlayerController>(Shooter->GetController());
		if (IsValid(PC) && PC->IsLocalController() && IsValid(PC->PlayerCameraManager))
		{
			PC->PlayerCameraManager->StartCameraShake(Data->FireCameraShakeClass);
		}
	}

	return true;
}
