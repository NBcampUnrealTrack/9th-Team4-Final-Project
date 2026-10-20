// Fill out your copyright notice in the Description page of Project Settings.


#include "NF_ViewModelSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossRegistrySubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "UI/ViewModel/Status/NF_MonsterStatusViewModel.h"

void UNF_ViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    APlayerController* PlayerController = nullptr;
    
    if (IsValid(LocalPlayer)) 
    {
        PlayerController = LocalPlayer->GetPlayerController(LocalPlayer->GetWorld());
    }

    MonsterStatusViewModel = NewObject<UNF_MonsterStatusViewModel>(this);
    
    BindPlayerController(PlayerController);
    BindBossRegistry(PlayerController ? PlayerController->GetWorld() : (LocalPlayer ? LocalPlayer->GetWorld() : nullptr));
    RefreshViewModelContext();
}

void UNF_ViewModelSubsystem::Deinitialize()
{
    UnbindBossRegistry();
    if (IsValid(MonsterStatusViewModel))
    {
        MonsterStatusViewModel->ClearObservedUnit();
    }
    MonsterStatusViewModel = nullptr;

    UnbindPlayerController();

    for (UNF_ViewModelBase* ViewModel : ManagedViewModels)
    {
        if (IsValid(ViewModel))
        {
            ViewModel->DeinitializeViewModel();
        }
    }

    ManagedViewModels.Reset();
    CurrentViewModelContext.Reset();

    Super::Deinitialize();
}

void UNF_ViewModelSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
    Super::PlayerControllerChanged(NewPlayerController);

    BindPlayerController(NewPlayerController);
    BindBossRegistry(NewPlayerController ? NewPlayerController->GetWorld() : nullptr);
    RefreshViewModelContext();
}

void UNF_ViewModelSubsystem::BindBossRegistry(UWorld* World)
{
	UNF_BossRegistrySubsystem* NewRegistry = IsValid(World)
		? World->GetSubsystem<UNF_BossRegistrySubsystem>()
		: nullptr;

	if (BoundBossRegistry.Get() == NewRegistry)
	{
		HandleActiveBossChanged(NewRegistry ? NewRegistry->GetActiveBoss() : nullptr);
		return;
	}

	UnbindBossRegistry();
	if (!IsValid(NewRegistry))
	{
		HandleActiveBossChanged(nullptr);
		return;
	}

	BoundBossRegistry = NewRegistry;
	ActiveBossChangedDelegateHandle = NewRegistry->OnActiveBossChanged.AddUObject(
		this, &ThisClass::HandleActiveBossChanged);
	HandleActiveBossChanged(NewRegistry->GetActiveBoss());
}

void UNF_ViewModelSubsystem::UnbindBossRegistry()
{
	if (UNF_BossRegistrySubsystem* Registry = BoundBossRegistry.Get())
	{
		if (ActiveBossChangedDelegateHandle.IsValid())
		{
			Registry->OnActiveBossChanged.Remove(ActiveBossChangedDelegateHandle);
		}
	}

	ActiveBossChangedDelegateHandle.Reset();
	BoundBossRegistry.Reset();
}

void UNF_ViewModelSubsystem::HandleActiveBossChanged(ANF_BossBase* NewBoss)
{
	if (!IsValid(MonsterStatusViewModel))
	{
		return;
	}

	if (!IsValid(NewBoss))
	{
		MonsterStatusViewModel->ClearObservedUnit();
		return;
	}

	MonsterStatusViewModel->SetObservedUnit(
		NewBoss->GetAbilitySystemComponent(), NewBoss->GetBossDisplayName());
}

void UNF_ViewModelSubsystem::BindPlayerController(APlayerController* NewPlayerController)
{
    if (BoundPlayerController.Get() == NewPlayerController) return;

    UnbindPlayerController();

    BoundPlayerController = NewPlayerController;

    if (!BoundPlayerController.IsValid()) return;

    PawnChangedDelegateHandle =
        BoundPlayerController->GetOnNewPawnNotifier().AddUObject(this,&ThisClass::HandlePawnChanged);
}

void UNF_ViewModelSubsystem::UnbindPlayerController()
{
    if (BoundPlayerController.IsValid() && PawnChangedDelegateHandle.IsValid())
    {
        BoundPlayerController->GetOnNewPawnNotifier().Remove(PawnChangedDelegateHandle);
    }

    PawnChangedDelegateHandle.Reset();
    BoundPlayerController.Reset();
}

void UNF_ViewModelSubsystem::HandlePawnChanged(APawn* NewPawn)
{
    // Pawn은 BuildViewModelContext()에서 다시 가져오므로
    // 인자를 직접 저장할 필요는 없다.
    RefreshViewModelContext();
}

FNF_ViewModelContext UNF_ViewModelSubsystem::BuildViewModelContext() const
{
    FNF_ViewModelContext NewContext;

    ULocalPlayer* LocalPlayer = GetLocalPlayer();
    APlayerController* PlayerController = BoundPlayerController.Get();

    NewContext.LocalPlayer = LocalPlayer;
    NewContext.PlayerController = PlayerController;

    if (!IsValid(PlayerController)) { return NewContext; }

    APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>();

    APawn* Pawn = PlayerController->GetPawn();

    NewContext.PlayerState = PlayerState;
    NewContext.Pawn = Pawn;
    
    if (IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(PlayerState))
    {
        NewContext.AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
    }

    //Monster는 ASC 소유방법에 대해 확인 
    else if (IAbilitySystemInterface* PawnAbilitySystemInterface = Cast<IAbilitySystemInterface>(Pawn))
    {
        NewContext.AbilitySystemComponent = PawnAbilitySystemInterface->GetAbilitySystemComponent();
    }

    return NewContext;
}

void UNF_ViewModelSubsystem::RefreshViewModelContext()
{
    CurrentViewModelContext = BuildViewModelContext();

    for (UNF_ViewModelBase* ViewModel : ManagedViewModels)
    {
        if (!IsValid(ViewModel)) continue;
        
        if (ViewModel->IsViewModelInitialized())
        {
            ViewModel->UpdateViewModelContext(CurrentViewModelContext);
        }
        else
        {
            ViewModel->InitializeViewModel(CurrentViewModelContext);
        }
    }
}

void UNF_ViewModelSubsystem::RegisterViewModel(UNF_ViewModelBase* ViewModel)
{
    if (!IsValid(ViewModel)) return;
    if (ManagedViewModels.Contains(ViewModel)) return;
    

    ManagedViewModels.Add(ViewModel);

    ViewModel->InitializeViewModel(CurrentViewModelContext);
}

void UNF_ViewModelSubsystem::UnregisterViewModel(UNF_ViewModelBase* ViewModel)
{
    if (!IsValid(ViewModel)) return;

    ViewModel->DeinitializeViewModel();
    ManagedViewModels.Remove(ViewModel);
}
