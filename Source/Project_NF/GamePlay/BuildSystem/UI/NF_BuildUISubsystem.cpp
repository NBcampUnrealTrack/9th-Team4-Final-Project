#include "NF_BuildUISubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Subsystems/SubsystemCollection.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UI/ViewModel/BuildShop/NF_BuildShopViewModel.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "NF_BuildShopScreen.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"

void UNF_BuildUISubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UNF_UIManagerSubsystem>();
	Collection.InitializeDependency<UNF_ViewModelSubsystem>();
	ShopViewModel = NewObject<UNF_BuildShopViewModel>(this);
	GetLocalPlayer()->GetSubsystem<UNF_ViewModelSubsystem>()->RegisterViewModel(ShopViewModel);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::HandleWorldCleanup);
}

void UNF_BuildUISubSystem::ConfigureScreen(TSubclassOf<UNF_BuildShopScreen> InScreenClass)
{
	if (ScreenClass != InScreenClass)
	{
		CloseScreen();
		ScreenClass = InScreenClass;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* Controller = LocalPlayer ? LocalPlayer->GetPlayerController(LocalPlayer->GetWorld()) : nullptr;

	BindController(Controller);
}

void UNF_BuildUISubSystem::RefreshPawnBinding()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!IsValid(LocalPlayer))
	{
		return;
	}

	BindController(LocalPlayer->GetPlayerController(LocalPlayer->GetWorld()));
}

void UNF_BuildUISubSystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	BindController(NewPlayerController);
}

void UNF_BuildUISubSystem::BindController(APlayerController* Controller)
{
	if (BoundController.Get() == Controller)
	{
		HandlePawnChanged(Controller ? Controller->GetPawn() : nullptr);
		return;
	}

	UnbindController();
	CloseScreen();

	BoundController = Controller;
	BoundWorld = IsValid(Controller) ? Controller->GetWorld() : nullptr;

	if (IsValid(Controller))
	{
		PawnChangedHandle =
			Controller-> GetOnNewPawnNotifier().AddUObject(
				this,
				&ThisClass::HandlePawnChanged);
	}

	// 이미 소유한 Pawn도 즉시 연결한다.
	HandlePawnChanged(IsValid(Controller) ? Controller->GetPawn() : nullptr);

}

void UNF_BuildUISubSystem::UnbindController()
{
	if (APlayerController* Controller = BoundController.Get())
	{
		Controller-> GetOnNewPawnNotifier().Remove(PawnChangedHandle);
	}

	PawnChangedHandle.Reset();
	BoundController.Reset();
}

void UNF_BuildUISubSystem::HandlePawnChanged(APawn* NewPawn)
{
	UNF_BuildComponent* NewBuild = IsValid(NewPawn) ? NewPawn->FindComponentByClass<UNF_BuildComponent>() : nullptr;
	SetObservedBuild(NewBuild);
	RefreshScreen();
}

void UNF_BuildUISubSystem::SetObservedBuild(UNF_BuildComponent* NewBuild)
{
	if (ObservedBuild.Get() == NewBuild && !ObservedBuild.IsStale())
	{
		return;
	}

	CloseScreen();
	if (UNF_BuildComponent* PreviousBuild = ObservedBuild.Get())
	{
		PreviousBuild->OnBuildShopStateChanged.RemoveDynamic(this, &ThisClass::HandleShopStateChanged);
	}

	ObservedBuild = NewBuild;
	if (IsValid(ShopViewModel.Get()))
	{
		ShopViewModel->SetBuildSource(NewBuild);
	}
	if (NewBuild)
	{
		NewBuild->OnBuildShopStateChanged.AddUniqueDynamic(this, &ThisClass::HandleShopStateChanged);
	}
}

void UNF_BuildUISubSystem::HandleShopStateChanged(ENF_BuildShopUIState State)
{
	if (State == ENF_BuildShopUIState::BuildClosed)
	{
		CloseScreen();
		return;
	}

	RefreshScreen();
}

void UNF_BuildUISubSystem::RefreshScreen()
{
	UNF_BuildComponent* Build = ObservedBuild.Get();

	if (!IsValid(Build) || !IsValid(Build->GetCurrentShop()))
	{
		CloseScreen();
		return;
	}

	if (!ScreenClass || !IsValid(ShopViewModel.Get()))
	{
		return;
	}

	if (IsValid(Screen) && Screen-> GetParent() != nullptr)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UNF_UIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer-> GetSubsystem<UNF_UIManagerSubsystem>() : nullptr;

	if (IsValid(UIManager))
	{
		Screen = Cast<UNF_BuildShopScreen>(
			UIManager->PushScreen(
				ScreenClass,
				ENF_ScreenInputMode::GameAndUI, false));
		if (IsValid(Screen) && !Screen->SetShopViewModel(ShopViewModel))
		{
			UE_LOG(LogTemp, Error, TEXT("BuildShop Manual ViewModel injection failed; closing the local screen."));
			CloseScreen();
		}
	}
}

void UNF_BuildUISubSystem::CloseScreen()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UNF_UIManagerSubsystem* UIManager = LocalPlayer
		? LocalPlayer->GetSubsystem<UNF_UIManagerSubsystem>()
		: nullptr;

	if (IsValid(Screen) && IsValid(UIManager))
	{
		Screen->SetShopViewModel(nullptr);
		UIManager->RemoveWidget(Screen);
	}

	Screen = nullptr;
}

void UNF_BuildUISubSystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World != BoundWorld.Get()) return;
	CloseScreen();
	SetObservedBuild(nullptr);
	UnbindController();
	BoundWorld.Reset();
}

void UNF_BuildUISubSystem::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	WorldCleanupHandle.Reset();
	CloseScreen();
	UnbindController();
	SetObservedBuild(nullptr);
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UNF_ViewModelSubsystem* Models = LocalPlayer->GetSubsystem<UNF_ViewModelSubsystem>())
		{
			Models->UnregisterViewModel(ShopViewModel);
		}
	}
	ShopViewModel = nullptr;
	BoundWorld.Reset();

	Super::Deinitialize();
}
