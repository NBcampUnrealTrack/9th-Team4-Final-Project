
#include "NF_PlayerController.h"
#include "Core/Match/NF_MatchmakingSubsystem.h"
#include "Engine/GameInstance.h"


#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/Data/NF_PlayerControllerDataAsset.h"
#include "TimerManager.h"
#include "Component/MotionMatchingComponent.h"
#include "Engine/World.h"
#include "GamePlay/BuildSystem/UI/NF_BuildUISubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Player/Component/NF_MatchResultPresenterComponent.h"
#include "UI/SubSystem/NF_UIManagerSubsystem.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UI/Widget/NF_GameHUDWidget.h"


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


void ANF_PlayerController::AcknowledgePossession(class APawn* P)
{
	Super::AcknowledgePossession(P);
	
	if (IsLocalController() == true)
	{
		auto* BuildSubSystem = ULocalPlayer::GetSubsystem<UNF_BuildSubSystem>(GetLocalPlayer());
		if (IsValid(BuildSubSystem) == false) return;
		
		BuildSubSystem->RefreshPawnBinding();
		
		RefreshViewModel();
	}
}


void ANF_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	ConfigDataAsset();
	ConfigBindDelegate();
	InputConfig();
	RefreshMiniMap();
	CreateGameHUD();
}


void ANF_PlayerController::ConfigDataAsset()
{
	checkf(IsValid(NF_PlayerControllerDataAsset) == true, TEXT("[PlayerController]의 [NF_PlayerControllerDataAsset]가 삽입되지않았습니다."));
	checkf(IsValid(NF_PlayerControllerDataAsset->GameHUDClass) == true, TEXT("[NF_PlayerControllerDataAsset]의 [GameHUDClass]가 삽입되지않았습니다."));
	checkf(IsValid(NF_PlayerControllerDataAsset->OptionsWidgetClass) == true, TEXT("[NF_PlayerControllerDataAsset]의 [OptionsWidgetClass]가 삽입되지않았습니다."));
}


void ANF_PlayerController::OnToggleZoomChanged(bool bNewIsToggleZoom)
{
	if (IsLocalController() == true)
	{
		bIsToggleZoom = bNewIsToggleZoom;
	}
}


void ANF_PlayerController::InputConfig()
{
	if (NF_PlayerControllerDataAsset == nullptr) return;
	
	if (IsLocalController() == true)
	{
		auto* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer());
		if (IsValid(EILPS) == false) return;
		
		EILPS->AddMappingContext(NF_PlayerControllerDataAsset->NF_PlayerIMC, 0);
	}
}


void ANF_PlayerController::ConfigBindDelegate()
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
	if (NF_PlayerControllerDataAsset == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
		return;
	}
	
	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;
	
	//Test
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->ZoomToggleSettingButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::ToggleSettingButton);
	
	
	//Attack
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->MeleeAttack, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::MeleeAttack);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->FireButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Fire);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->FireButton, ETriggerEvent::Completed, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::StopFire);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->FireButton, ETriggerEvent::Canceled, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::StopFire);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->ReloadButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Reload);
	
	
	//Zoom
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->ZoomButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::ZoomIn);
	
	if (NF_PlayerControllerDataAsset->bIsTestMode == false)
	{
		EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->ZoomButton, ETriggerEvent::Completed, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::ZoomOut);
		EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->ZoomButton, ETriggerEvent::Canceled, this, 
			&ThisClass::TryAction, ENF_PlayerActionType::ZoomOut);
	}
	
	
	
	//Skill
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->SkillButton_A, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillA);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->SkillButton_B, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillB);
	
	
	//상호작용
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->InteractButton, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::Interaction);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->UseItemButton, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::UseItem);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->OpenOptionButton, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::OpenOption);
	
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->TestVisualMesh_Next_Button, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::TestMeshSwap_Next);
	EnhancedInputComponent->BindAction(NF_PlayerControllerDataAsset->TestVisualMesh_Pre_Button, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::TestMeshSwap_Pre);
}


#pragma region TryAction
void ANF_PlayerController::TryAction(ENF_PlayerActionType PlayerActionType)
{
	OpenOption(PlayerActionType);
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
	
		if (PlayerActionType == ENF_PlayerActionType::UseItem)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
	
		if (PlayerActionType == ENF_PlayerActionType::TestMeshSwap_Next)
		{
			NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
	
		if (PlayerActionType == ENF_PlayerActionType::TestMeshSwap_Pre)
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

	void ANF_PlayerController::OpenOption(ENF_PlayerActionType PlayerActionType)
	{	
		if (PlayerActionType == ENF_PlayerActionType::OpenOption)
		{
			if (IsValid(NF_PlayerControllerDataAsset) == false) return;
	
			if (IsLocalController() == false || GetLocalPlayer() == nullptr) return;

			if (NF_PlayerControllerDataAsset->OptionsWidgetClass == nullptr || NF_PlayerControllerDataAsset->OptionsWidgetClass->HasAnyClassFlags(CLASS_Abstract))
			{
				UE_LOG(LogTemp, Warning, TEXT("Assign WBP_Option to OptionsWidgetClass on the PlayerController defaults."));
				return;
			}

			auto* NF_UIManagerSubsystem = GetLocalPlayer()->GetSubsystem<UNF_UIManagerSubsystem>();
			if (IsValid(NF_UIManagerSubsystem) == false) return;
		
			NF_UIManagerSubsystem->PushModal(NF_PlayerControllerDataAsset->OptionsWidgetClass);
			
			return;
		}
	}
	#pragma endregion
#pragma endregion


#pragma region BuildShop
void ANF_PlayerController::OnInteractBuildShop(APlayerController* SendController, ENF_BuildShopUIState BuildShopType)
{
	if (SendController != this) return;
	
	CurrentBuildShopType = BuildShopType;
	
	if (BuildShopType == ENF_BuildShopUIState::BuildOpened)
	{
		ReleaseGameplayInputForUI();
		
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}
	else if (BuildShopType == ENF_BuildShopUIState::BuildClosed)
	{
		ResetIgnoreMoveInput();
		ResetIgnoreLookInput();
	}
	
	if (IsLocalController() == true)
	{
		if (IsValid(GetPawn()) == false) return;
		
		auto* MotionMatchingComponent = GetPawn()->FindComponentByClass<UMotionMatchingComponent>();
		if (IsValid(MotionMatchingComponent) == false) return;
		
		if (BuildShopType == ENF_BuildShopUIState::BuildOpened)
		{
			MotionMatchingComponent->SetFeatureEnabled(EMotionMatchingFeature::Jump, false);
			MotionMatchingComponent->SetFeatureEnabled(EMotionMatchingFeature::Traversal, false);
		}
		
		if (BuildShopType == ENF_BuildShopUIState::BuildClosed)
		{
			MotionMatchingComponent->SetFeatureEnabled(EMotionMatchingFeature::Jump, true);
			MotionMatchingComponent->SetFeatureEnabled(EMotionMatchingFeature::Traversal, true);
		}
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
	if (NF_PlayerControllerDataAsset->GameHUDClass == nullptr) return;
	
	if (IsLocalController() == true)
	{
		auto* UIManager = ULocalPlayer::GetSubsystem<UNF_UIManagerSubsystem>(GetLocalPlayer());
		if (IsValid(UIManager) == false) return;

		UIManager->SetHUD(NF_PlayerControllerDataAsset->GameHUDClass);
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
void ANF_PlayerController::RefreshMiniMap()
{
	GetMinimapRevealRegistry();
	GetWorldTimerManager().SetTimer(MinimapRevealTimer, this, &ThisClass::UpdateMinimapReveals, 0.2f, true);
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


ANF_PlayerController::ANF_PlayerController()
{
	NF_MatchResultPresenterComponent = CreateDefaultSubobject<UNF_MatchResultPresenterComponent>("NF_MatchResultPresenterComponent");
}


void ANF_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MinimapRevealTimer);
	if (MinimapRevealRegistry) { MinimapRevealRegistry->Clear(); }
	Super::EndPlay(EndPlayReason);
}


void ANF_PlayerController::ClientHandleMatchCompleted_Implementation(const FNF_MatchResult& Result)
{
	if (IsValid(NF_MatchResultPresenterComponent) == false)
	{
		UE_LOG(LogTemp,Error,TEXT("[NF MatchResult] 결과 Presenter가 없어 자동 복귀를 처리할 수 없습니다."));

		return;
	}

	NF_MatchResultPresenterComponent->RequestReturnToMatchmaking(Result);
}
#pragma endregion