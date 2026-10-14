#include "BuildSystem/Player/NF_BuildInputComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "BuildSystem/Player/NF_BuildComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputMappingQuery.h"

UNF_BuildInputComponent::UNF_BuildInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNF_BuildInputComponent::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (Controller)
	{
		Controller->OnPossessedPawnChanged.AddUniqueDynamic(this, &UNF_BuildInputComponent::HandlePawnChanged);
		InitializeInput(Cast<UEnhancedInputComponent>(Controller->InputComponent));
	}
}

void UNF_BuildInputComponent::InitializeInput(UEnhancedInputComponent* Input)
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController() || !Input)
	{
		return;
	}
	if (BoundInput == Input && !BindingHandles.IsEmpty())
	{
		RebuildMapping();
		return;
	}
	RemoveInputBindings();
	BoundInput = Input;
	if (!MappingContext)
	{
		MappingContext = NewObject<UInputMappingContext>(this, NAME_None, RF_Transient);
		for (int32 Slot = 0; Slot < 2; ++Slot)
		{
			UInputAction* Action = NewObject<UInputAction>(this, NAME_None, RF_Transient);
			Action->ValueType = EInputActionValueType::Boolean;
			SlotActions.Add(Action);
		}
	}
	for (int32 Slot = 0; Slot < 2; ++Slot)
	{
		BindingHandles.Add(Input->BindAction(SlotActions[Slot], ETriggerEvent::Started,
		                                     this, &UNF_BuildInputComponent::HandleSlotPressed, Slot).GetHandle());
		BindingHandles.Add(Input->BindAction(SlotActions[Slot], ETriggerEvent::Completed,
		                                     this, &UNF_BuildInputComponent::HandleSlotReleased, Slot).GetHandle());
		BindingHandles.Add(Input->BindAction(SlotActions[Slot], ETriggerEvent::Canceled,
		                                     this, &UNF_BuildInputComponent::HandleSlotReleased, Slot).GetHandle());
	}
	RebuildMapping();
}

FKey UNF_BuildInputComponent::GetSlotKey(int32 SlotIndex) const
{
	return SlotIndex == 0 ? SlotOneKey : SlotIndex == 1 ? SlotTwoKey : FKey();
}

void UNF_BuildInputComponent::SetLegacySkillActions(UInputAction* First, UInputAction* Second)
{
	LegacySkillActions = {First, Second};
}

bool UNF_BuildInputComponent::SetSlotKey(int32 SlotIndex, FKey Key)
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (!Controller || !Controller->IsLocalController() || SlotIndex < 0 || SlotIndex > 1 ||
		!Key.IsValid() || Key.IsAnalog() || Key.IsModifierKey() || Key == EKeys::Escape ||
		Key == GetSlotKey(1 - SlotIndex))
	{
		return false;
	}
	ULocalPlayer* LocalPlayer = Controller->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		                                                ? ULocalPlayer::GetSubsystem<
			                                                UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		                                                : nullptr;
	if (Subsystem && MappingContext && SlotActions.IsValidIndex(SlotIndex) && Subsystem->HasMappingContext(
		MappingContext))
	{
		TArray<FMappingQueryIssue> Issues;
		const EMappingQueryResult Result = Subsystem->QueryMapKeyInActiveContextSet(
			MappingContext, SlotActions[SlotIndex], Key, Issues, EMappingQueryIssue::NoIssue);
		if (Result != EMappingQueryResult::MappingAvailable && Result != EMappingQueryResult::NotMappable)
		{
			return false;
		}
		for (const FMappingQueryIssue& Issue : Issues)
		{
			if (Issue.Issue == EMappingQueryIssue::NoIssue) continue;
			bool bSkillAction = false;
			for (const UInputAction* Action : SlotActions) bSkillAction |= Action && Action == Issue.BlockingAction;
			for (const UInputAction* Action : LegacySkillActions) bSkillAction |= Action && Action == Issue.
				BlockingAction;
			if (!bSkillAction) return false;
		}
	}
	HandleSlotReleased(SlotIndex);
	(SlotIndex == 0 ? SlotOneKey : SlotTwoKey) = Key;
	RebuildMapping();
	OnSlotKeysChanged.Broadcast();
	return true;
}

void UNF_BuildInputComponent::RebuildMapping()
{
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		                                                ? ULocalPlayer::GetSubsystem<
			                                                UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		                                                : nullptr;
	if (!Subsystem || !MappingContext || SlotActions.Num() != 2)
	{
		return;
	}
	Subsystem->RemoveMappingContext(MappingContext);
	APawn* Pawn = Controller->GetPawn();
	if (!Pawn || !Pawn->FindComponentByClass<UNF_BuildComponent>())
	{
		return;
	}
	MappingContext->UnmapAll();
	MappingContext->MapKey(SlotActions[0], SlotOneKey);
	MappingContext->MapKey(SlotActions[1], SlotTwoKey);
	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	Subsystem->AddMappingContext(MappingContext, MappingPriority, Options);
}

void UNF_BuildInputComponent::HandleSlotPressed(int32 SlotIndex)
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (UNF_BuildComponent* Build = Pawn ? Pawn->FindComponentByClass<UNF_BuildComponent>() : nullptr)
	{
		Build->PressActiveSkillInput(SlotIndex);
	}
}

void UNF_BuildInputComponent::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (BoundInput.IsValid())
	{
		// Clear held input state when changing the controlled pawn (including a respawn).
		if (const IAbilitySystemInterface* Provider = Cast<IAbilitySystemInterface>(OldPawn))
		{
			if (UAbilitySystemComponent* ASC = Provider->GetAbilitySystemComponent())
			{
				ASC->AbilityLocalInputReleased(1);
				ASC->AbilityLocalInputReleased(2);
			}
		}
	}
	RebuildMapping();
}

void UNF_BuildInputComponent::HandleSlotReleased(int32 SlotIndex)
{
	const APlayerController* Controller = Cast<APlayerController>(GetOwner());
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (UNF_BuildComponent* Build = Pawn ? Pawn->FindComponentByClass<UNF_BuildComponent>() : nullptr)
	{
		Build->ReleaseActiveSkillInput(SlotIndex);
	}
}

void UNF_BuildInputComponent::RemoveInputBindings()
{
	if (BoundInput.IsValid())
	{
		for (uint32 Handle : BindingHandles)
		{
			BoundInput->RemoveBindingByHandle(Handle);
		}
	}
	BindingHandles.Empty();
	BoundInput.Reset();
}

void UNF_BuildInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HandleSlotReleased(0);
	HandleSlotReleased(1);
	APlayerController* Controller = Cast<APlayerController>(GetOwner());
	if (Controller)
	{
		Controller->OnPossessedPawnChanged.RemoveDynamic(this, &UNF_BuildInputComponent::HandlePawnChanged);
	}
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		                                                    ? ULocalPlayer::GetSubsystem<
			                                                    UEnhancedInputLocalPlayerSubsystem>(LocalPlayer)
		                                                    : nullptr)
	{
		if (MappingContext)
		{
			Subsystem->RemoveMappingContext(MappingContext);
		}
	}
	RemoveInputBindings();
	Super::EndPlay(EndPlayReason);
}
