
#include "NF_PlayerController.h"
#include "Core/Match/NF_MatchmakingSubsystem.h"
#include "Engine/GameInstance.h"


#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/Data/NF_PlayerInputDataAsset.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Kismet/KismetStringLibrary.h"
#include "Kismet/KismetStringTableLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UI/Widget/NF_GameHUDWidget.h"


class UNF_UIManagerSubsystem;

void ANF_PlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, CurrentBuildShopType);
}

void ANF_PlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	RefreshViewModel();
}


void ANF_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	GetMinimapRevealRegistry();
	GetWorldTimerManager().SetTimer(MinimapRevealTimer, this, &ThisClass::UpdateMinimapReveals, 0.2f, true);
	
	InputConfig();
	BindDelegate();
	CreateGameHUD();
}


void ANF_PlayerController::InputConfig()
{
	if (IsLocalController() == true)
	{
		auto* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer());
		if (IsValid(EILPS) == false) return;

		if (NF_PlayerInputDataAsset == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
			return;
		}
		
		EILPS->AddMappingContext(NF_PlayerInputDataAsset->NF_PlayerIMC, 0);
	}
}


void ANF_PlayerController::BindDelegate()
{
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnInteractBuildShop, OnInteractBuildShop);
}


void ANF_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UMotionMatchingInputLibrary::BindMotionMatchingInput(this, MotionMatchingInputDataAsset);
	
	BindInputButton();
}


void ANF_PlayerController::BindInputButton()
{
	if (NF_PlayerInputDataAsset == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
		return;
	}
	
	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;
	
	//Test
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ZoomToggleSettingButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::ToggleSettingButton);
	
	
	//Attack
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->MeleeAttack, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::MeleeAttack);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Fire);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Completed, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::StopFire);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Canceled, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::StopFire);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ReloadButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Reload);
	
	
	//Zoom
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ZoomButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::ZoomIn);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ZoomButton, ETriggerEvent::Completed, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::ZoomOut);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ZoomButton, ETriggerEvent::Canceled, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::ZoomOut);
	
	
	//Skill
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_A, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillA);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_B, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillB);
	
	
	//상호작용
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->InteractButton, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::Interaction);
}


#pragma region TryAction
void ANF_PlayerController::TryAction(ENF_PlayerActionType PlayerActionType)
{
	IsCombatAction(PlayerActionType);
	IsBuildShopAction(PlayerActionType);
}


	#pragma region Actions
	void ANF_PlayerController::IsBuildShopAction(ENF_PlayerActionType PlayerActionType)
	{
		if (CurrentBuildShopType == ENF_BuildShopUIState::BuildOpened)
		{
			if (PlayerActionType == ENF_PlayerActionType::SkillA)
			{
				NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerBindingSkill, this, PlayerActionType);
				return;
			}
		
			if (PlayerActionType == ENF_PlayerActionType::SkillB)
			{
				NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerBindingSkill, this, PlayerActionType);
				return;
			}
			
			return;
		}
	}


	void ANF_PlayerController::IsCombatAction(ENF_PlayerActionType PlayerActionType)
	{
		if (CurrentBuildShopType == ENF_BuildShopUIState::BuildOpened) return;
	
		//Toggle 옵션 키 (test용도)
		if (PlayerActionType == ENF_PlayerActionType::ToggleSettingButton)
		{
			bIsToggleZoom = !bIsToggleZoom;
			UKismetSystemLibrary::PrintString(this, FString::Printf(TEXT("토글 실행 %d"), bIsToggleZoom));
		}
	
		if (PlayerActionType == ENF_PlayerActionType::MeleeAttack)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
	
		if (PlayerActionType == ENF_PlayerActionType::Fire)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
		
		if (PlayerActionType == ENF_PlayerActionType::StopFire)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
		
		if (PlayerActionType == ENF_PlayerActionType::ZoomIn)
		{
			ZoomSelector(PlayerActionType);
			return;
		}
		
		if (bIsToggleZoom == false && PlayerActionType == ENF_PlayerActionType::ZoomOut)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
		
		if (PlayerActionType == ENF_PlayerActionType::Reload)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
		
		if (PlayerActionType == ENF_PlayerActionType::SkillA)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
		
		if (PlayerActionType == ENF_PlayerActionType::SkillB)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
		
		if (PlayerActionType == ENF_PlayerActionType::Interaction)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
	}


	void ANF_PlayerController::ZoomSelector(ENF_PlayerActionType PlayerActionType)
	{
		if (bIsToggleZoom == false)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
			
		if (bIsToggleZoom == true && bIsZoomIn == false)
		{
			bIsZoomIn = true;
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
		}
			
		else if (bIsToggleZoom == true && bIsZoomIn == true)
		{
			bIsZoomIn = false;
			PlayerActionType = ENF_PlayerActionType::ZoomOut;
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
		}
	}
	#pragma endregion
#pragma endregion


#pragma region BuildShop
void ANF_PlayerController::OnInteractBuildShop(ENF_BuildShopUIState BuildShopType)
{
	CurrentBuildShopType = BuildShopType;
	
	if (BuildShopType == ENF_BuildShopUIState::BuildOpened)
	{
		ReleaseGameplayInputForUI();
	}
}



void ANF_PlayerController::ReleaseGameplayInputForUI()
{
	bIsZoomIn = false;
	NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction,this,ENF_PlayerActionType::ZoomOut);
	NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction,this,ENF_PlayerActionType::StopFire);
}
#pragma endregion

#pragma region UI
void ANF_PlayerController::CreateGameHUD()
{
	if (IsLocalController() == true)
	{
		if (GameHUDClass == nullptr) return;

		auto* UIManager = ULocalPlayer::GetSubsystem<UNF_UIManagerSubsystem>(GetLocalPlayer());
		if (IsValid(UIManager) == false) return;

		UIManager->SetHUD(GameHUDClass);
	}
}


void ANF_PlayerController::RefreshViewModel()
{
	auto* ViewModelSubsystem = ULocalPlayer::GetSubsystem<UNF_ViewModelSubsystem>(GetLocalPlayer());
	if (IsValid(ViewModelSubsystem) == false) return;

	ViewModelSubsystem->RefreshViewModelContext();
}
#pragma endregion 



#pragma region NotDevelopedByMe
void ANF_PlayerController::ClientHandleMatchCompleted_Implementation(const FNF_MatchResult& Result)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UNF_MatchmakingSubsystem* Match = GI->GetSubsystem<UNF_MatchmakingSubsystem>())
		{
			Match->HandleDedicatedMatchCompleted(Result);
		}
	}
}


UNF_MinimapRevealRegistry* ANF_PlayerController::GetMinimapRevealRegistry()
{
	if (!MinimapRevealRegistry)
	{
		MinimapRevealRegistry = NewObject<UNF_MinimapRevealRegistry>(this);
		MinimapRevealRegistry->Initialize(this);
	}
	return MinimapRevealRegistry;
}


void ANF_PlayerController::UpdateMinimapReveals()
{
	if (HasAuthority())
	{
		TArray<FNF_MinimapRevealMarker> Markers;
		GetMinimapRevealRegistry()->BuildServerSnapshot(Markers);
		if (!Markers.IsEmpty() || bSentMinimapReveals)
		{
			ClientReceiveMinimapReveals(Markers);
		}
		bSentMinimapReveals = !Markers.IsEmpty();
	}
	if (IsLocalController()) { GetMinimapRevealRegistry()->ExpireClientSnapshot(); }
}


void ANF_PlayerController::ClientReceiveMinimapReveals_Implementation(const TArray<FNF_MinimapRevealMarker>& Markers)
{
	GetMinimapRevealRegistry()->ReceiveSnapshot(Markers, 1.0f);
}


void ANF_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MinimapRevealTimer);
	if (MinimapRevealRegistry) { MinimapRevealRegistry->Clear(); }
	Super::EndPlay(EndPlayReason);
}
#pragma endregion