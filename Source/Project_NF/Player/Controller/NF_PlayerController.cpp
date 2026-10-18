
#include "NF_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/Data/NF_PlayerInputDataAsset.h"


void ANF_PlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, CurrentBuildShopType);
}


void ANF_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	InputConfig();
	
	BindDelegate();
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
	
	//Combat
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Fire);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Completed, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::StopFire);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Canceled, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::StopFire);
	
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ZoomButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Zoom);
	
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ReloadButton, ETriggerEvent::Started, this, 
	&ThisClass::TryAction, ENF_PlayerActionType::Reload);
	
	
	//Skill
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_A, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillA);
	
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_B, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillB);
	
	
	//상호작용
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->InteractButton, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::Interaction);
}


void ANF_PlayerController::TryAction(ENF_PlayerActionType PlayerActionType)
{
	if (CurrentBuildShopType == ENF_BuildShopUIState::BuildOpened)
	{
		if (PlayerActionType == ENF_PlayerActionType::SkillA)
		{
			//NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
	
		if (PlayerActionType == ENF_PlayerActionType::SkillB)
		{
			//NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
			return;
		}
		
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
	
	if (PlayerActionType == ENF_PlayerActionType::Zoom)
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


#pragma region BuildShop

void ANF_PlayerController::OnInteractBuildShop(ENF_BuildShopUIState BuildShopType)
{
	CurrentBuildShopType = BuildShopType;
}

#pragma endregion
