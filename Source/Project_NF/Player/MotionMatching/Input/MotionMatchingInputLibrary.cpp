#include "Input/MotionMatchingInputLibrary.h"
#include "Input/MotionMatchingInputDataAsset.h" // [변경] v0.0.2
#include "Component/MotionMatchingComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogMotionMatchingInput, Log, All);

bool UMotionMatchingInputLibrary::BindMotionMatchingInput(APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig)
{
	if (IsValid(PlayerController) == false) return false;

	if (IsValid(InputConfig) == false)
	{
		UE_LOG(LogMotionMatchingInput, Warning, TEXT("[MotionMatching] InputConfig가 비어 있습니다. 컨트롤러 Details에서 DA_MotionMatchingInputConfig를 지정하세요."));
		return false;
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return false;

	AddMappingContext(PlayerController, InputConfig);
	BindMoveInput(EnhancedInputComponent, PlayerController, InputConfig);
	BindLookInput(EnhancedInputComponent, PlayerController, InputConfig);
	BindStateInput(EnhancedInputComponent, PlayerController, InputConfig);
	BindJumpInput(EnhancedInputComponent, PlayerController, InputConfig);
	return true;
}


UMotionMatchingComponent* UMotionMatchingInputLibrary::GetControlledMotionMatchingComponent(const APlayerController* PlayerController)
{
	if (IsValid(PlayerController) == false) return nullptr;

	const APawn* ControlledPawn = PlayerController->GetPawn();
	if (IsValid(ControlledPawn) == false) return nullptr;

	return ControlledPawn->FindComponentByClass<UMotionMatchingComponent>();
}


void UMotionMatchingInputLibrary::AddMappingContext(APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig)
{
	if (PlayerController->IsLocalPlayerController() == false) return;
	if (IsValid(InputConfig->MappingContext) == false) return;

	UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (IsValid(EnhancedInputSubsystem) == false) return;

	EnhancedInputSubsystem->AddMappingContext(InputConfig->MappingContext, InputConfig->MappingContextPriority);
}


void UMotionMatchingInputLibrary::BindMoveInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig)
{
	if (InputConfig->bIsMoveBindingEnabled == false) return;

	const TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);

	if (IsValid(InputConfig->MoveAction) == true)
	{
		auto HandleMove = [WeakPlayerController](const FInputActionValue& Value)
		{
			UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
			if (IsValid(MotionMatchingComponent) == false) return;

			MotionMatchingComponent->ApplyMoveInput(Value.Get<FVector2D>());
		};

		EnhancedInputComponent->BindActionValueLambda(InputConfig->MoveAction, ETriggerEvent::Triggered, HandleMove);
		EnhancedInputComponent->BindActionValueLambda(InputConfig->MoveAction, ETriggerEvent::Completed, HandleMove);
	}

	if (IsValid(InputConfig->MoveWorldSpaceAction) == true)
	{
		auto HandleMoveWorldSpace = [WeakPlayerController](const FInputActionValue& Value)
		{
			UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
			if (IsValid(MotionMatchingComponent) == false) return;

			MotionMatchingComponent->ApplyMoveWorldSpaceInput(Value.Get<FVector2D>());
		};

		EnhancedInputComponent->BindActionValueLambda(InputConfig->MoveWorldSpaceAction, ETriggerEvent::Triggered, HandleMoveWorldSpace);
		EnhancedInputComponent->BindActionValueLambda(InputConfig->MoveWorldSpaceAction, ETriggerEvent::Completed, HandleMoveWorldSpace);
	}
}


void UMotionMatchingInputLibrary::BindLookInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig)
{
	if (InputConfig->bIsLookBindingEnabled == false) return;

	const TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);

	if (IsValid(InputConfig->LookAction) == true)
	{
		EnhancedInputComponent->BindActionValueLambda(InputConfig->LookAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
		{
			APlayerController* OwnerPlayerController = WeakPlayerController.Get();
			if (IsValid(OwnerPlayerController) == false) return;

			const FVector2D LookValue = Value.Get<FVector2D>();
			OwnerPlayerController->AddYawInput(LookValue.X);
			OwnerPlayerController->AddPitchInput(LookValue.Y);
		});
	}

	// 게임패드는 프레임마다 값이 들어오므로 DeltaSeconds를 곱해 프레임 속도와 무관하게 만든다
	if (IsValid(InputConfig->LookGamepadAction) == true)
	{
		EnhancedInputComponent->BindActionValueLambda(InputConfig->LookGamepadAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
		{
			APlayerController* OwnerPlayerController = WeakPlayerController.Get();
			if (IsValid(OwnerPlayerController) == false) return;

			const UWorld* World = OwnerPlayerController->GetWorld();
			if (IsValid(World) == false) return;

			const FVector2D LookValue = Value.Get<FVector2D>() * World->GetDeltaSeconds();
			OwnerPlayerController->AddYawInput(LookValue.X);
			OwnerPlayerController->AddPitchInput(LookValue.Y);
		});
	}
}


// 질주·걷기·옆걸음·조준·웅크리기
void UMotionMatchingInputLibrary::BindStateInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig)
{
	const TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);

	if (InputConfig->bIsSprintBindingEnabled == true && IsValid(InputConfig->SprintAction) == true)
	{
		EnhancedInputComponent->BindActionValueLambda(InputConfig->SprintAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
		{
			UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
			if (IsValid(MotionMatchingComponent) == false) return;

			MotionMatchingComponent->SetWantsToSprint(Value.Get<bool>());
		});
	}

	if (InputConfig->bIsWalkBindingEnabled == true && IsValid(InputConfig->WalkAction) == true)
	{
		EnhancedInputComponent->BindActionValueLambda(InputConfig->WalkAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
		{
			UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
			if (IsValid(MotionMatchingComponent) == false) return;

			MotionMatchingComponent->ToggleWantsToWalk();
		});
	}

	if (InputConfig->bIsStrafeBindingEnabled == true && IsValid(InputConfig->StrafeAction) == true)
	{
		EnhancedInputComponent->BindActionValueLambda(InputConfig->StrafeAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
		{
			UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
			if (IsValid(MotionMatchingComponent) == false) return;

			MotionMatchingComponent->ToggleWantsToStrafe();
		});
	}

	if (InputConfig->bIsAimBindingEnabled == true && IsValid(InputConfig->AimAction) == true)
	{
		EnhancedInputComponent->BindActionValueLambda(InputConfig->AimAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
		{
			UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
			if (IsValid(MotionMatchingComponent) == false) return;

			MotionMatchingComponent->SetWantsToAim(Value.Get<bool>());
		});
	}

	if (InputConfig->bIsCrouchBindingEnabled == true && IsValid(InputConfig->CrouchAction) == true)
	{
		EnhancedInputComponent->BindActionValueLambda(InputConfig->CrouchAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
		{
			UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
			if (IsValid(MotionMatchingComponent) == false) return;

			MotionMatchingComponent->ToggleCrouch();
		});
	}
}


// 누른 순간: 트래버설 시도 후 실패하면 점프 / 누르고 있는 동안: 트래버설만 계속 시도
void UMotionMatchingInputLibrary::BindJumpInput(UEnhancedInputComponent* EnhancedInputComponent, APlayerController* PlayerController, const UMotionMatchingInputDataAsset* InputConfig)
{
	if (InputConfig->bIsJumpBindingEnabled == false) return;
	if (IsValid(InputConfig->JumpAction) == false) return;

	const TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);

	EnhancedInputComponent->BindActionValueLambda(InputConfig->JumpAction, ETriggerEvent::Started, [WeakPlayerController](const FInputActionValue& Value)
	{
		UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
		if (IsValid(MotionMatchingComponent) == false) return;

		MotionMatchingComponent->StartJumpInput();
	});

	EnhancedInputComponent->BindActionValueLambda(InputConfig->JumpAction, ETriggerEvent::Triggered, [WeakPlayerController](const FInputActionValue& Value)
	{
		UMotionMatchingComponent* MotionMatchingComponent = GetControlledMotionMatchingComponent(WeakPlayerController.Get());
		if (IsValid(MotionMatchingComponent) == false) return;

		MotionMatchingComponent->HoldJumpInput();
	});
}
