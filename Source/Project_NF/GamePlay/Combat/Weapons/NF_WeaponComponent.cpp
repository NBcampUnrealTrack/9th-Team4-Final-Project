#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"

#include "GameFramework/Character.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GAS/Abilities/NF_GA_WeaponFire.h"
#include "GAS/Abilities/NF_GA_WeaponZoom.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

UNF_WeaponComponent::UNF_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
	FireAbilityClass = UNF_GA_WeaponFire::StaticClass();
	ZoomAbilityClass = UNF_GA_WeaponZoom::StaticClass();
}

void UNF_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::OnPawnControllerChanged);
	}
	RefreshCrosshair();

	if (GetOwner()->HasAuthority() == false)
		return;
	
	if (IsValid(EquippedWeapon) == false && DefaultWeaponClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());

		if (ANF_WeaponBase* Weapon =
			GetWorld()->SpawnActor<ANF_WeaponBase>(
				DefaultWeaponClass,
				FTransform::Identity,
				Params))
		{
			if (EquipWeapon(Weapon) == false)
			{
				Weapon->Destroy();
			}
		}
	}
}

bool UNF_WeaponComponent::EquipWeapon(ANF_WeaponBase* NewWeapon)
{
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	
	if (IsValid(NewWeapon) == false || IsValid(OwnerCharacter) == false)
		return false;
	
	if (OwnerCharacter->HasAuthority() == false)
		return false;
	
	if(bChangingWeapon)
		return false;
	
	if (NewWeapon->GetWorld() != GetWorld() || IsValid(OwnerCharacter->GetMesh()) ==false ||
		IsValid(NewWeapon->GetRootComponent()) == false)
		return false;

	if (NewWeapon->GetOwner() && NewWeapon->GetOwner() != OwnerCharacter)
		return false;

	if (EquippedWeapon == NewWeapon) 
		return true;

	TGuardValue<bool> ChangeGuard(bChangingWeapon, true);
	
	if (NewWeapon->AttachToComponent(OwnerCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocketName) == false)
		return false;

	UnequipWeaponInternal();
	NewWeapon->CancelReload();

	EquippedWeapon = NewWeapon;
	EquippedWeapon->SetOwner(OwnerCharacter);
	EquippedWeapon->SetInstigator(OwnerCharacter);
	EquippedWeapon->ForceNetUpdate();
	OwnerCharacter->ForceNetUpdate();
	RefreshCrosshair();

	return true;
}

ANF_WeaponBase* UNF_WeaponComponent::UnequipWeapon()
{
	if (IsValid(GetOwner()) == false || GetOwner()->HasAuthority() == false || bChangingWeapon)
		return nullptr;

	TGuardValue<bool> ChangeGuard(bChangingWeapon, true);
	return UnequipWeaponInternal();
}

ANF_WeaponBase* UNF_WeaponComponent::UnequipWeaponInternal()
{
	StopFire();
	StopZoom();
	
	ANF_WeaponBase* OldWeapon = nullptr;
	
	if (IsValid(EquippedWeapon))
	{
		OldWeapon = EquippedWeapon.Get();
	}
	
	EquippedWeapon = nullptr;
	RemoveCrosshair();
	
	if (OldWeapon)
	{
		OldWeapon->CancelReload();
		OldWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		OldWeapon->SetOwner(nullptr);
		OldWeapon->SetInstigator(nullptr);
		OldWeapon->ForceNetUpdate();
	}
	GetOwner()->ForceNetUpdate();
	return OldWeapon;
}

void UNF_WeaponComponent::RemoveCrosshair()
{
	if (CrosshairWidget)
	{
		CrosshairWidget->RemoveFromParent();
		CrosshairWidget = nullptr;
	}
}

void UNF_WeaponComponent::RefreshCrosshair()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	
	if (IsValid(OwnerPawn) == false || IsValid(EquippedWeapon) == false)
	{
		RemoveCrosshair();
		return;
	}

	APlayerController* Controller = Cast<APlayerController>(OwnerPawn->GetController());
	
	if (IsValid(Controller) == false || Controller->IsLocalController() == false || Controller->GetLocalPlayer() == nullptr)
	{
		RemoveCrosshair();
		return;
	}
	
	const UNF_WeaponDataAsset* Data = EquippedWeapon->GetWeaponData();
	
	if (IsValid(Data) == false || Data->CrosshairWidgetClass == nullptr)
	{
		RemoveCrosshair();
		return;
	}

	if (CrosshairWidget && CrosshairWidget->GetOwningPlayer() == Controller &&
		CrosshairWidget->GetClass() == Data->CrosshairWidgetClass.Get())
	{
		return;
	}

	RemoveCrosshair();
	CrosshairWidget = CreateWidget<UUserWidget>(Controller, Data->CrosshairWidgetClass);
	
	if (CrosshairWidget)
	{
		CrosshairWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (CrosshairWidget->AddToPlayerScreen() == false)
		{
			RemoveCrosshair();
		}
	}
}

void UNF_WeaponComponent::OnRep_EquippedWeapon()
{
	RefreshCrosshair();
}

void UNF_WeaponComponent::OnPawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (GetOwner()->HasAuthority()) StopZoom();
	RestoreZoomCamera();
	SetComponentTickEnabled(bZoomActive);
	RefreshCrosshair();
}

void UNF_WeaponComponent::StartFire()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerStartFire();
		return;
	}

	if (bChangingWeapon || !IsValid(EquippedWeapon)) return;
	
	// 빠르게 재클릭해도 RPM 제한 유지
	if (bFireInputHeld) return;
	
	bFireInputHeld = true;
	
	if (EnsureFireAbility()) 
		FireASC->TryActivateAbility(FireAbilityHandle);
}

void UNF_WeaponComponent::StopFire()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerStopFire();
		return;
	}
	
	bFireInputHeld = false;
	
	if (FireASC.IsValid() && FireAbilityHandle.IsValid())
	{
		FireASC->CancelAbilityHandle(FireAbilityHandle);
	}
}

void UNF_WeaponComponent::Reload()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerReload();
		return;
	}

	if (bChangingWeapon) return;
	
	StopFire();
	StopZoom();
	
	if (IsValid(EquippedWeapon))
	{
		EquippedWeapon->Reload();
	}
}

bool UNF_WeaponComponent::EnsureFireAbility()
{
	if (FireASC.IsValid() && FireASC->GetAvatarActor() == GetOwner() &&
		FireASC->FindAbilitySpecFromHandle(FireAbilityHandle)) return true;
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (ASC == nullptr || ASC->GetAvatarActor() != GetOwner() || FireAbilityClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("WeaponFire: %s needs an initialized ASC (NF_Player / NF_PlayerState)."),
			*GetNameSafe(GetOwner()));
		return false;
	}
	
	FireASC = ASC;
	FireAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(FireAbilityClass, 1, INDEX_NONE, this));
	return FireAbilityHandle.IsValid();
}

void UNF_WeaponComponent::ServerStartFire_Implementation() { StartFire(); }
void UNF_WeaponComponent::ServerStopFire_Implementation() { StopFire(); }
void UNF_WeaponComponent::ServerReload_Implementation() { Reload(); }

void UNF_WeaponComponent::ToggleZoom()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerToggleZoom();
		return;
	}

	if (bZoomActive)
	{
		StopZoom();
	}
	else
	{
		StartZoom();
	}
}

void UNF_WeaponComponent::StartZoom()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerStartZoom();
		return;
	}
	
	if (bChangingWeapon || IsValid(EquippedWeapon) == false || bZoomActive)
		return;
	
	if (EnsureZoomAbility())
		ZoomASC->TryActivateAbility(ZoomAbilityHandle);
}

void UNF_WeaponComponent::StopZoom()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerStopZoom();
		return;
	}
	
	if (ZoomASC.IsValid() && ZoomAbilityHandle.IsValid())
	{
		ZoomASC->CancelAbilityHandle(ZoomAbilityHandle);
	}
	
	SetZoomActive(false);
}

bool UNF_WeaponComponent::EnsureZoomAbility()
{
	if (ZoomASC.IsValid() && ZoomASC->GetAvatarActor() == GetOwner() &&
		ZoomASC->FindAbilitySpecFromHandle(ZoomAbilityHandle)) return true;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (ASC == nullptr || ASC->GetAvatarActor() != GetOwner() || ZoomAbilityClass == nullptr)
		return false;
	
	if (ZoomASC.IsValid() && ZoomAbilityHandle.IsValid())
		ZoomASC->ClearAbility(ZoomAbilityHandle);
	
	ZoomASC = ASC;
	ZoomAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(ZoomAbilityClass, 1, INDEX_NONE, this));
	
	return ZoomAbilityHandle.IsValid();
}

void UNF_WeaponComponent::SetZoomActive(bool bActive)
{
	if (bZoomActive == bActive) return;
	bZoomActive = bActive;
	OnRep_ZoomActive();
	GetOwner()->ForceNetUpdate();
}

void UNF_WeaponComponent::OnRep_ZoomActive()
{
	SetComponentTickEnabled(true);
}

void UNF_WeaponComponent::RestoreZoomCamera()
{
	if (ZoomCamera.IsValid()) ZoomCamera->SetFieldOfView(UnzoomedFieldOfView);
	ZoomCamera.Reset();
}

void UNF_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (GetOwner()->HasAuthority() && bZoomActive && IsValid(EquippedWeapon) == false)
		StopZoom();
	
	const APawn* Pawn = Cast<APawn>(GetOwner());
	
	if (Pawn == nullptr || Pawn->IsLocallyControlled() == false)
	{
		RestoreZoomCamera();
		SetComponentTickEnabled(GetOwner()->HasAuthority() && bZoomActive);
		return;
	}

	if (ZoomCamera.IsValid() && !ZoomCamera->IsActive()) 
		RestoreZoomCamera();
	
	if (ZoomCamera.IsValid() == false && bZoomActive)
	{
		TInlineComponentArray<UCameraComponent*> Cameras(GetOwner());
		for (UCameraComponent* Camera : Cameras)
		{
			if (Camera->IsActive())
			{
				ZoomCamera = Camera;
				UnzoomedFieldOfView = Camera->FieldOfView;
				break;
			}
		}
	}
	
	if (ZoomCamera.IsValid())
	{
		const float TargetFOV = bZoomActive
			? FMath::Min(UnzoomedFieldOfView, FMath::Clamp(ZoomFieldOfView, 5.f, 170.f))
			: UnzoomedFieldOfView;
		
		ZoomCamera->SetFieldOfView(FMath::FInterpTo(ZoomCamera->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed));
		
		if (bZoomActive == false && FMath::IsNearlyEqual(ZoomCamera->FieldOfView, TargetFOV, 0.01f)) 
			RestoreZoomCamera();
	}
	
	if (bZoomActive == false && ZoomCamera.IsValid() == false)
		SetComponentTickEnabled(false);
}

void UNF_WeaponComponent::ServerToggleZoom_Implementation() { ToggleZoom(); }
void UNF_WeaponComponent::ServerStartZoom_Implementation() { StartZoom(); }
void UNF_WeaponComponent::ServerStopZoom_Implementation() { StopZoom(); }

void UNF_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::OnPawnControllerChanged);
	}
	RemoveCrosshair();

	if (GetOwner()->HasAuthority())
	{
		StopZoom();
		if (ZoomASC.IsValid() && ZoomAbilityHandle.IsValid()) ZoomASC->ClearAbility(ZoomAbilityHandle);
	}
	RestoreZoomCamera();

	if (GetOwner()->HasAuthority() && FireASC.IsValid())
	{
		StopFire();
		if (FireAbilityHandle.IsValid()) FireASC->ClearAbility(FireAbilityHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

void UNF_WeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNF_WeaponComponent, EquippedWeapon);
	DOREPLIFETIME(UNF_WeaponComponent, bZoomActive);
}
