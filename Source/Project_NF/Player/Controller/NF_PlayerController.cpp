
#include "NF_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerInputDataAsset.h"


void ANF_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	OnChangedPlayerType.AddUObject(this, &ThisClass::OnChangedPlayerStateType);
	
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


void ANF_PlayerController::OnChangedPlayerStateType(ENF_PlayerStateType NewPlayerType)
{
	CurrentPlayerStateType = NewPlayerType;
	
	auto* NF_Player = Cast<ANF_Player>(this->GetPawn());
	if (IsValid(NF_Player) == false) return;
	
	if (CurrentPlayerStateType == ENF_PlayerStateType::InteractingBuild)
	{
		NF_Player->GetCharacterMovement()->SetMovementMode(MOVE_None);
	}
	
	else
	{
		NF_Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}


void ANF_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UMotionMatchingInputLibrary::BindMotionMatchingInput(this, MotionMatchingInputDataAsset);
	
	BindActiveSkill();
}


void ANF_PlayerController::BindActiveSkill()
{
	if (NF_PlayerInputDataAsset == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
		return;
	}
	
	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;
		
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_A, ETriggerEvent::Started, this, &ThisClass::TrySkillAction_A);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_B, ETriggerEvent::Started, this, &ThisClass::TrySkillAction_B);
}


void ANF_PlayerController::TrySkillAction_A()
{
	OnChangedPlayerType.Broadcast(ENF_PlayerStateType::InteractingBuild);
	
	auto* NF_Player = Cast<ANF_Player>(this->GetPawn());
	if (IsValid(NF_Player) == false) return;
	
	NF_Player->PlayAnimMontage(TestMontage);
	
	/*
	if (CurrentPlayerStateType == ENF_PlayerStateType::InteractingBuild)
	{
		if (SelectedActiveSkill_Tag.IsValid() == true)
		{
			BindActiveSkillButton_A();
			return;
		}
	}
	
	if (SkillButtonA_Tag.IsValid() == false) return;
	
	UE_LOG(LogTemp, Warning, TEXT("SkillA 발동함"));
	UNF_GASFunctionLibrary::NF_TryAbility(this, SkillButtonA_Tag);*/
	
}


void ANF_PlayerController::TrySkillAction_B()
{
	OnChangedPlayerType.Broadcast(ENF_PlayerStateType::None);
	
	/*if (CurrentPlayerStateType == ENF_PlayerStateType::InteractingBuild)
	{
		if (SelectedActiveSkill_Tag.IsValid() == true)
		{
			BindActiveSkillButton_B();
			return;
		}
	}
	
	if (SkillButtonB_Tag.IsValid() == false) return;
	
	UE_LOG(LogTemp, Warning, TEXT("SkillB 발동함"));
	UNF_GASFunctionLibrary::NF_TryAbility(this, SkillButtonB_Tag);*/
}



void ANF_PlayerController::BindActiveSkillButton_A()
{
	SkillButtonA_Tag = SelectedActiveSkill_Tag;
	SelectedActiveSkill_Tag = FGameplayTag();
}


void ANF_PlayerController::BindActiveSkillButton_B()
{
	SkillButtonB_Tag = SelectedActiveSkill_Tag;
	SelectedActiveSkill_Tag = FGameplayTag();
}




