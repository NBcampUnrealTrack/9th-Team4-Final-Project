#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponentConfigDataAsset.h"

#include "GameFramework/Character.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GAS/Abilities/NFGA_WeaponFire.h"
#include "GAS/Abilities/NFGA_WeaponZoom.h"
#include "GAS/Abilities/NFGA_WeaponReload.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"
#include "Player/Character/NF_Player.h"

UNF_WeaponComponent::UNF_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
}

void UNF_WeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsValid(Config) == false)
		return;
	
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::OnPawnControllerChanged);
	}

	if (GetOwner()->HasAuthority() == false)
		return;
	
	if (IsValid(Config) && IsValid(EquippedWeapon) == false && Config->DefaultWeaponClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();
		Params.Instigator = Cast<APawn>(GetOwner());

		if (ANF_WeaponBase* Weapon =
			GetWorld()->SpawnActor<ANF_WeaponBase>(
				Config->DefaultWeaponClass,
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
	auto* OwnerCharacter = Cast<ANF_Player>(GetOwner());
	
	if (IsValid(Config) == false || IsValid(NewWeapon) == false || IsValid(OwnerCharacter) == false)
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
	const UNF_WeaponDataAsset* WeaponData = NewWeapon->GetWeaponData();
	
	if (IsValid(WeaponData) == false)
	{
		WeaponData = GetDefault<UNF_WeaponDataAsset>();
	}
	
	if (NewWeapon->AttachToComponent(OwnerCharacter->VisualMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponData->WeaponSocketName) == false)
		return false;

	UnequipWeaponInternal();
	NewWeapon->CancelReload();

	EquippedWeapon = NewWeapon;
	EquippedWeapon->SetOwner(OwnerCharacter);
	EquippedWeapon->SetInstigator(OwnerCharacter);
	EquippedWeapon->ForceNetUpdate();
	OwnerCharacter->ForceNetUpdate();
	OnEquippedWeaponChanged.Broadcast(EquippedWeapon.Get());

	return true;
}

ANF_WeaponBase* UNF_WeaponComponent::UnequipWeapon()
{
	if (IsValid(GetOwner()) == false || GetOwner()->HasAuthority() == false || bChangingWeapon)
		return nullptr;

	TGuardValue<bool> ChangeGuard(bChangingWeapon, true);
	const bool bHadWeapon = EquippedWeapon != nullptr;
	ANF_WeaponBase* OldWeapon = UnequipWeaponInternal();
	if (bHadWeapon)
	{
		OnEquippedWeaponChanged.Broadcast(nullptr);
	}
	return OldWeapon;
}

ANF_WeaponBase* UNF_WeaponComponent::UnequipWeaponInternal()
{
	CancelReload();
	StopFire();
	StopZoom();
	RestoreZoomCamera();
	
	ANF_WeaponBase* OldWeapon = nullptr;
	
	if (IsValid(EquippedWeapon))
	{
		OldWeapon = EquippedWeapon.Get();
	}
	
	EquippedWeapon = nullptr;
	
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

void UNF_WeaponComponent::RefreshWeaponDataPresentation(ANF_WeaponBase* ChangedWeapon)
{
	if (!IsValid(ChangedWeapon) || EquippedWeapon != ChangedWeapon) return;
	RestoreZoomCamera();
	SetComponentTickEnabled(bZoomActive);
}

void UNF_WeaponComponent::OnRep_EquippedWeapon()
{
	RestoreZoomCamera();
	SetComponentTickEnabled(bZoomActive);
	OnEquippedWeaponChanged.Broadcast(EquippedWeapon.Get());
}

void UNF_WeaponComponent::OnPawnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (GetOwner()->HasAuthority()) StopZoom();
	RestoreZoomCamera();
	SetComponentTickEnabled(bZoomActive);
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

	if (bChangingWeapon || IsValid(EquippedWeapon) == false)
		return;

	if (EnsureReloadAbility())
	{
		ReloadASC->TryActivateAbility(ReloadAbilityHandle);
	}
}

void UNF_WeaponComponent::CancelReload()
{
	if (GetOwner()->HasAuthority() == false)
		return;
	
	if (ReloadASC.IsValid() && ReloadAbilityHandle.IsValid())
		ReloadASC->CancelAbilityHandle(ReloadAbilityHandle);
}

bool UNF_WeaponComponent::EnsureReloadAbility()
{
	if (IsValid(Config) == false)
		return false;

	if (ReloadASC.IsValid() && ReloadASC->GetAvatarActor() == GetOwner() &&
		ReloadASC->FindAbilitySpecFromHandle(ReloadAbilityHandle)) return true;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	if (ASC == nullptr || ASC->GetAvatarActor() != GetOwner() || Config->ReloadAbilityClass == nullptr)
		return false;

	if (ReloadASC.IsValid() && ReloadAbilityHandle.IsValid())
		ReloadASC->ClearAbility(ReloadAbilityHandle);
	
	ReloadASC = ASC;
	ReloadAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(Config->ReloadAbilityClass, 1, INDEX_NONE, this));
	return ReloadAbilityHandle.IsValid();
}

bool UNF_WeaponComponent::EnsureFireAbility()
{
	if (IsValid(Config) == false)
		return false;

	if (FireASC.IsValid() && FireASC->GetAvatarActor() == GetOwner() &&
		FireASC->FindAbilitySpecFromHandle(FireAbilityHandle)) return true;
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (ASC == nullptr || ASC->GetAvatarActor() != GetOwner() || Config->FireAbilityClass == nullptr)
	{
		return false;
	}
	
	FireASC = ASC;
	FireAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(Config->FireAbilityClass, 1, INDEX_NONE, this));
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

bool UNF_WeaponComponent::CanZoom() const
{
	const UNF_WeaponDataAsset* Data = IsValid(EquippedWeapon) ? EquippedWeapon->GetWeaponData() : nullptr;
	return !bChangingWeapon && IsValid(Data) && Data->ZoomMode != ENF_ZoomMode::None;
}

float UNF_WeaponComponent::GetZoomSensitivityMultiplier() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled() || !bZoomActive || !CanZoom()) return 1.f;
	return FMath::Clamp(EquippedWeapon->GetWeaponData()->ZoomSensitivityMultiplier, 0.01f, 1.f);
}

void UNF_WeaponComponent::StartZoom()
{
	if (GetOwner()->HasAuthority() == false)
	{
		ServerStartZoom();
		return;
	}
	
	if (CanZoom() == false || bZoomActive)
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
	if (IsValid(Config) == false)
		return false;

	if (ZoomASC.IsValid() && ZoomASC->GetAvatarActor() == GetOwner() &&
		ZoomASC->FindAbilitySpecFromHandle(ZoomAbilityHandle)) return true;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	
	if (ASC == nullptr || ASC->GetAvatarActor() != GetOwner() || Config->ZoomAbilityClass == nullptr)
		return false;
	
	if (ZoomASC.IsValid() && ZoomAbilityHandle.IsValid())
		ZoomASC->ClearAbility(ZoomAbilityHandle);
	
	ZoomASC = ASC;
	ZoomAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(Config->ZoomAbilityClass, 1, INDEX_NONE, this));
	
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
	if (!bZoomActive) UpdateScopeCameraAttachment(false);
	if (bZoomActive == false) UpdateScopedMeshes(false);
	SetComponentTickEnabled(true);
	OnZoomActiveChanged.Broadcast(bZoomActive);
}

void UNF_WeaponComponent::UpdateScopedMeshes(bool bHide)
{
	if (bHide == false)
	{
		for (const auto& Mesh : HiddenScopeMeshes)
		{
			if (Mesh.IsValid()) Mesh->SetHiddenInGame(false);
		}
		HiddenScopeMeshes.Reset();
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn == nullptr || Pawn->IsLocallyControlled() == false) return;
	for (AActor* Actor : {GetOwner(), static_cast<AActor*>(EquippedWeapon.Get())})
	{
		if (IsValid(Actor) == false) continue;
		TInlineComponentArray<UMeshComponent*> Meshes(Actor);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (!Mesh->bHiddenInGame)
			{
				HiddenScopeMeshes.Add(Mesh);
				Mesh->SetHiddenInGame(true);
			}
		}
	}
}

void UNF_WeaponComponent::UpdateScopeCameraAttachment(bool bAttach)
{
	if (bAttach == false)
	{
		if (bScopeCameraAttached && ZoomCamera.IsValid())
		{
			if (OriginalCameraParent.IsValid())
			{
				ZoomCamera->AttachToComponent(OriginalCameraParent.Get(), FAttachmentTransformRules::KeepWorldTransform, OriginalCameraSocket);
			}
			else
			{
				ZoomCamera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			}
			ZoomCamera->SetAbsolute(bOriginalAbsoluteLocation, bOriginalAbsoluteRotation, bOriginalAbsoluteScale);
			if (OriginalCameraParent.IsValid()) ZoomCamera->SetRelativeTransform(OriginalCameraTransform);
			ZoomCamera->bUsePawnControlRotation = bOriginalUsePawnControlRotation;
		}
		for (const auto& Entry : ScopeMeshTickOptions)
		{
			if (Entry.Key.IsValid()) Entry.Key->VisibilityBasedAnimTickOption = Entry.Value;
		}
		ScopeMeshTickOptions.Reset();
		OriginalCameraParent.Reset();
		bScopeCameraAttached = false;
		return;
	}

	if (bScopeCameraAttached || !ZoomCamera.IsValid() || !IsValid(EquippedWeapon)) return;
	UStaticMeshComponent* Mesh = EquippedWeapon->GetWeaponMesh();
	const FName Socket = EquippedWeapon->GetScopeSocketName();
	if (!IsValid(Mesh) || !Mesh->DoesSocketExist(Socket)) return;

	OriginalCameraParent = ZoomCamera->GetAttachParent();
	OriginalCameraSocket = ZoomCamera->GetAttachSocketName();
	OriginalCameraTransform = ZoomCamera->GetRelativeTransform();
	bOriginalAbsoluteLocation = ZoomCamera->IsUsingAbsoluteLocation();
	bOriginalAbsoluteRotation = ZoomCamera->IsUsingAbsoluteRotation();
	bOriginalAbsoluteScale = ZoomCamera->IsUsingAbsoluteScale();
	bOriginalUsePawnControlRotation = ZoomCamera->bUsePawnControlRotation;
	if (!ZoomCamera->AttachToComponent(Mesh, FAttachmentTransformRules::KeepWorldTransform, Socket)) return;
	
	ZoomCamera->SetAbsolute(false, true, true);
	ZoomCamera->SetRelativeLocation(FVector::ZeroVector);
	ZoomCamera->bUsePawnControlRotation = true;
	if (const APawn* Pawn = Cast<APawn>(GetOwner())) ZoomCamera->SetWorldRotation(Pawn->GetViewRotation());
	bScopeCameraAttached = true;
	
	for (AActor* Actor : {GetOwner(), static_cast<AActor*>(EquippedWeapon.Get())})
	{
		TInlineComponentArray<USkeletalMeshComponent*> Meshes(Actor);
		for (USkeletalMeshComponent* SkeletalMesh : Meshes)
		{
			ScopeMeshTickOptions.Add(SkeletalMesh, SkeletalMesh->VisibilityBasedAnimTickOption);
			SkeletalMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		}
	}
}

void UNF_WeaponComponent::RestoreZoomCamera()
{
	UpdateScopeCameraAttachment(false);
	UpdateScopedMeshes(false);
	if (ZoomCamera.IsValid()) ZoomCamera->SetFieldOfView(UnzoomedFieldOfView);
	ZoomCamera.Reset();
	PresentedZoomWeapon.Reset();
}

void UNF_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (GetOwner()->HasAuthority() && bZoomActive && !CanZoom())
		StopZoom();

	const APawn* Pawn = Cast<APawn>(GetOwner());
	
	if (Pawn == nullptr || Pawn->IsLocallyControlled() == false)
	{
		RestoreZoomCamera();
		SetComponentTickEnabled(GetOwner()->HasAuthority() && bZoomActive);
		return;
	}

	if (ZoomCamera.IsValid() && (!ZoomCamera->IsActive() || PresentedZoomWeapon.Get() != EquippedWeapon.Get()))
		RestoreZoomCamera();

	const bool bApplyZoom = bZoomActive && CanZoom();
	const UNF_WeaponDataAsset* ZoomData = bApplyZoom ? EquippedWeapon->GetWeaponData() : nullptr;
	
	if (ZoomCamera.IsValid() == false && bApplyZoom)
	{
		TInlineComponentArray<UCameraComponent*> Cameras(GetOwner());
		for (UCameraComponent* Camera : Cameras)
		{
			if (Camera->IsActive())
			{
				ZoomCamera = Camera;
				UnzoomedFieldOfView = Camera->FieldOfView;
				PresentedZoomWeapon = EquippedWeapon;
				break;
			}
		}
	}
	
	if (ZoomCamera.IsValid())
	{
		if (ZoomData) ActiveZoomInterpSpeed = FMath::Max(0.f, ZoomData->ZoomInterpSpeed);
		UpdateScopeCameraAttachment(ZoomData && ZoomData->ZoomMode == ENF_ZoomMode::Scope && ZoomData->bAttachScopeCameraToMuzzle);
		UpdateScopedMeshes(ZoomData && ZoomData->ZoomMode == ENF_ZoomMode::Scope && ZoomData->bHideMeshesWhileScoped);
		const float TargetFOV = bApplyZoom
			? FMath::Min(UnzoomedFieldOfView, FMath::Clamp(ZoomData->ZoomFieldOfView, 5.f, 170.f))
			: UnzoomedFieldOfView;
		
		ZoomCamera->SetFieldOfView(FMath::FInterpTo(ZoomCamera->FieldOfView, TargetFOV, DeltaTime, ActiveZoomInterpSpeed));
		
		if (!bApplyZoom && FMath::IsNearlyEqual(ZoomCamera->FieldOfView, TargetFOV, 0.01f))
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

	if (GetOwner()->HasAuthority())
	{
		CancelReload();
		if (ReloadASC.IsValid() && ReloadAbilityHandle.IsValid()) ReloadASC->ClearAbility(ReloadAbilityHandle);
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
