#include "NF_BuildUISubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "NF_BuildShopScreen.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"


void UNF_BuildSubSystem::ConfigureScreen(TSubclassOf<UNF_BuildShopScreen> InScreenClass)
{
	if (ScreenClass != InScreenClass)
	{
		CloseScreen();
		ScreenClass = InScreenClass;
	}
	
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	APlayerController* Controller = LocalPlayer ? LocalPlayer->GetPlayerController(LocalPlayer->GetWorld()) : nullptr;
	
	BindController(Controller);
	
	RefreshScreen();
}

void UNF_BuildSubSystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	BindController(NewPlayerController);
}

void UNF_BuildSubSystem::BindController(APlayerController* Controller)
{
	if (BoundController.Get() == Controller)
	{
		HandlePawnChanged(Controller ? Controller->GetPawn() : nullptr);
		return;
	}
	
	UnbindController();
	
	BoundController = Controller;
	
	if (IsValid(Controller))
	{
		PawnChangedHandle = 
			Controller -> GetOnNewPawnNotifier().AddUObject(
				this,
				&ThisClass::HandlePawnChanged);
	}
	
	// 이미 소유한 Pawn도 즉시 연결한다.
	HandlePawnChanged(IsValid(Controller) ? Controller->GetPawn() : nullptr);
	
}

void UNF_BuildSubSystem::UnbindController()
{
	if (APlayerController* Controller = BoundController.Get())
	{
		Controller -> GetOnNewPawnNotifier().Remove(PawnChangedHandle);
	}
	
	PawnChangedHandle.Reset();
	BoundController.Reset();
}

void UNF_BuildSubSystem::HandlePawnChanged(APawn* NewPawn)
{
	UNF_BuildComponent* NewBuild = IsValid(NewPawn) ? NewPawn -> FindComponentByClass<UNF_BuildComponent>() : nullptr;
	
	if (ObservedBuild.Get() == NewBuild)
	{
		RefreshScreen();
		return;
	}
	
	CloseScreen();
	UnbindBuild();
	
	ObservedBuild = NewBuild;
	
	if (IsValid(NewBuild))
	{
		NewBuild->OnBuildShopStateChanged.AddUniqueDynamic(this, &ThisClass::HandleShopStateChanged);
	}
	
	RefreshScreen();
	
}

void UNF_BuildSubSystem::UnbindBuild()
{
	if (UNF_BuildComponent* Build = ObservedBuild.Get())
	{
		Build->OnBuildShopStateChanged.RemoveDynamic(this, &ThisClass::HandleShopStateChanged);
	}
	
	ObservedBuild.Reset();
}

void UNF_BuildSubSystem::HandleShopStateChanged(ENF_BuildShopUIState State)
{
	if (State == ENF_BuildShopUIState::BuildClosed)
	{
		CloseScreen();
		return;
	}
	
	RefreshScreen();
}

void UNF_BuildSubSystem::RefreshScreen()
{
	UNF_BuildComponent* Build = ObservedBuild.Get();
	
	if (!IsValid(Build) || !IsValid(Build->GetCurrentShop()))
	{
		CloseScreen();
		return;
	}
	
	if (!ScreenClass)
	{
		return;
	}
	
	if (IsValid(Screen) && Screen -> GetParent()!=nullptr)
	{
		return;
	}
	
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UNF_UIManagerSubsystem* UIManager = LocalPlayer ? LocalPlayer -> GetSubsystem<UNF_UIManagerSubsystem>() : nullptr;
	
	if (IsValid(UIManager))
	{
		Screen = Cast<UNF_BuildShopScreen>(
			UIManager->PushScreen(
				ScreenClass,
				ENF_ScreenInputMode::GameAndUI));
	}
}

void UNF_BuildSubSystem::CloseScreen()
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UNF_UIManagerSubsystem* UIManager = LocalPlayer
		? LocalPlayer->GetSubsystem<UNF_UIManagerSubsystem>()
		: nullptr;

	if (IsValid(Screen) && IsValid(UIManager))
	{
		UIManager->RemoveWidget(Screen);
	}

	Screen = nullptr;
}

void UNF_BuildSubSystem::Deinitialize()
{
	UnbindController();
	UnbindBuild();
	CloseScreen();
	
	Super::Deinitialize();
}