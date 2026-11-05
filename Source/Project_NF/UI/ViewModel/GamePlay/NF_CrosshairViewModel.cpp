#include "UI/ViewModel/GamePlay/NF_CrosshairViewModel.h"

#include "Blueprint/UserWidget.h"
#include "GamePlay/Combat/Weapons/NF_WeaponBase.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponDataAsset.h"

void UNF_CrosshairViewModel::OnInitializeViewModel()
{
	Super::OnInitializeViewModel();
	BindWeaponComponent();
}

void UNF_CrosshairViewModel::OnViewModelContextChanged()
{
	Super::OnViewModelContextChanged();
	BindWeaponComponent();
}

void UNF_CrosshairViewModel::OnDeinitializeViewModel()
{
	UnbindWeaponComponent();
	UE_MVVM_SET_PROPERTY_VALUE(ActiveCrosshairClass, TSubclassOf<UUserWidget>());
	Super::OnDeinitializeViewModel();
}

void UNF_CrosshairViewModel::BindWeaponComponent()
{
	UNF_WeaponComponent* NewComponent = GetViewModelContext().WeaponComponent.Get();

	// 같은 유효한 컴포넌트면 기존 구독을 유지한다.
	if (IsValid(NewComponent) && BoundWeaponComponent.Get() == NewComponent) return;


	UnbindWeaponComponent();
	BoundWeaponComponent = NewComponent;

	if (IsValid(NewComponent))
	{
		EquippedWeaponChangedHandle =
			NewComponent->OnEquippedWeaponChanged.AddUObject(this, &ThisClass::HandleEquippedWeaponChanged);
		ZoomActiveChangedHandle =
			NewComponent->OnZoomActiveChanged.AddUObject(this, &ThisClass::HandleZoomActiveChanged);
		LocalShotFiredHandle =
			NewComponent->OnLocalShotFired.AddUObject(this, &ThisClass::HandleLocalShotFired);
	}

	// 이미 장착된 무기와 줌 상태를 반영하고, 컴포넌트가 없으면 표시를 비운다.
	RefreshCrosshair();
}

void UNF_CrosshairViewModel::UnbindWeaponComponent()
{
	UNF_WeaponComponent* Component = BoundWeaponComponent.Get();
	if (IsValid(Component))
	{
		Component->OnEquippedWeaponChanged.Remove(EquippedWeaponChangedHandle);
		Component->OnZoomActiveChanged.Remove(ZoomActiveChangedHandle);
		Component->OnLocalShotFired.Remove(LocalShotFiredHandle);
	}

	EquippedWeaponChangedHandle.Reset();
	ZoomActiveChangedHandle.Reset();
	LocalShotFiredHandle.Reset();
	BoundWeaponComponent.Reset();
}

TSubclassOf<UUserWidget> UNF_CrosshairViewModel::ResolveCrosshairClass() const
{
	UNF_WeaponComponent* Component = BoundWeaponComponent.Get();
	if (!IsValid(Component)) return {};

	ANF_WeaponBase* Weapon = Component->GetEquippedWeapon();
	if (!IsValid(Weapon)) return {};

	const UNF_WeaponDataAsset* Data = Weapon->GetWeaponData();
	if (!IsValid(Data)) return {};

	if (Component->IsZooming() && Data->ZoomMode == ENF_ZoomMode::Scope && Data->ScopeWidgetClass != nullptr)
	{
		return Data->ScopeWidgetClass;
	}

	return Data->CrosshairWidgetClass;
}

void UNF_CrosshairViewModel::RefreshCrosshair()
{
	const TSubclassOf<UUserWidget> NewClass = ResolveCrosshairClass();
	UE_MVVM_SET_PROPERTY_VALUE(ActiveCrosshairClass, NewClass);
}

void UNF_CrosshairViewModel::HandleEquippedWeaponChanged(ANF_WeaponBase* /*NewWeapon*/)
{
	RefreshCrosshair();
}

void UNF_CrosshairViewModel::HandleZoomActiveChanged(bool /*bZoomActive*/)
{
	RefreshCrosshair();
}

void UNF_CrosshairViewModel::HandleLocalShotFired(ANF_WeaponBase* FiredWeapon)
{
	UNF_WeaponComponent* Component = BoundWeaponComponent.Get();
	if (!IsValid(Component) || !IsValid(FiredWeapon))return;

	// 현재 연결 대상의 장착 무기인지 다시 확인한다.
	if (Component->GetEquippedWeapon() != FiredWeapon)return;
	
	OnShotFeedbackRequested.Broadcast();
}
