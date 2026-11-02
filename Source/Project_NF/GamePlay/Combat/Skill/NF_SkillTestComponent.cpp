#include "GamePlay/Combat/Skill/NF_SkillTestComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "Project_NF.h"

namespace
{
	FKey NF_TestSkillKey(int32 SlotIndex)
	{
		switch (SlotIndex)
		{
		case 0: return EKeys::E;
		case 1: return EKeys::Q;
		default: return EKeys::LeftShift;
		}
	}
}

UNF_SkillTestComponent::UNF_SkillTestComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	SlotStates.SetNum(SlotCount);
}

void UNF_SkillTestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNF_SkillTestComponent, TestSkillClass);
	DOREPLIFETIME(UNF_SkillTestComponent, TestSkillClassQ);
	DOREPLIFETIME(UNF_SkillTestComponent, TestSkillClassShift);
}

TSubclassOf<UNFGA_SkillBase> UNF_SkillTestComponent::GetTestSkillClass(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return TestSkillClass;
	case 1: return TestSkillClassQ;
	case 2: return TestSkillClassShift;
	default: return nullptr;
	}
}

FGameplayAbilitySpec* UNF_SkillTestComponent::FindTestSkillSpec(int32 SlotIndex)
{
	if (!SlotStates.IsValidIndex(SlotIndex)) { return nullptr; }
	// Resolve by slot, not class: the same GA class may be assigned to multiple test keys.
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.InputID == FirstTestInputID + SlotIndex) { return &Spec; }
	}
	return nullptr;
}

void UNF_SkillTestComponent::BeginPlay()
{
	Super::BeginPlay();
	InitAbilityActorInfo(GetOwner(), GetOwner());
	if (GetOwner()->HasAuthority())
	{
		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
		{
			if (const auto SkillClass = GetTestSkillClass(SlotIndex))
			{
				TestSkillHandles.Add(GiveAbility(FGameplayAbilitySpec(SkillClass, 1, FirstTestInputID + SlotIndex, this)));
			}
		}
	}
	RefreshTestInput();
}

void UNF_SkillTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Possession / controller replication can complete after BeginPlay.
	RefreshTestInput();
}

void UNF_SkillTestComponent::RefreshTestInput()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) { return; }
	if (CachedController.Get() != Pawn->GetController())
	{
		CachedController = Pawn->GetController();
		InitAbilityActorInfo(Pawn, Pawn);
	}
	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	if (!PlayerController || !PlayerController->IsLocalController()) { RemoveTestInput(); return; }
	if (BoundPlayerController.Get() == PlayerController && TestInput) { return; }
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem) { return; }

	RemoveTestInput();
	TestMapping = NewObject<UInputMappingContext>(this);
	TestInput = NewObject<UEnhancedInputComponent>(GetOwner());
	TestInput->RegisterComponent();
	TestInput->bBlockInput = false;
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		Action->ValueType = EInputActionValueType::Boolean;
		Action->bConsumeInput = false;
		SlotStates[SlotIndex].Action = Action;
		TestMapping->MapKey(Action, NF_TestSkillKey(SlotIndex));
		TestInput->BindAction(Action, ETriggerEvent::Started, this, &UNF_SkillTestComponent::BeginTestSkillAim, SlotIndex);
		TestInput->BindAction(Action, ETriggerEvent::Completed, this, &UNF_SkillTestComponent::ReleaseTestSkillAim, SlotIndex);
		TestInput->BindAction(Action, ETriggerEvent::Canceled, this, &UNF_SkillTestComponent::CancelTestSkillAim, SlotIndex);
	}
	PlayerController->PushInputComponent(TestInput);
	Subsystem->AddMappingContext(TestMapping, 1);
	BoundPlayerController = PlayerController;
	InputSubsystem = Subsystem;
}

void UNF_SkillTestComponent::BeginTestSkillAim(int32 SlotIndex)
{
	if (!SlotStates.IsValidIndex(SlotIndex)) { return; }
	UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] %s pressed | Owner=%s | GA=%s | Authority=%s"),
		*NF_TestSkillKey(SlotIndex).ToString(), *GetNameSafe(GetOwner()), *GetNameSafe(GetTestSkillClass(SlotIndex).Get()),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));
	CancelTestSkillAim(SlotIndex);
	FGameplayAbilitySpec* Spec = FindTestSkillSpec(SlotIndex);
	UNFGA_SkillBase* Ability = Spec ? Cast<UNFGA_SkillBase>(Spec->GetPrimaryInstance()) : nullptr;
	if (!Ability)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillTest] %s skill not assigned or granted instance is not ready."), *NF_TestSkillKey(SlotIndex).ToString());
		return;
	}
	SlotStates[SlotIndex].bPressed = true;
	if (!Ability->ShouldActivateOnInputRelease()) { ActivateTestSkill(SlotIndex); return; }
	if (!Ability->BeginLocalInput())
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillTest] Input rejected. Check skill availability, tags, cost and cooldown."));
		return;
	}
	SlotStates[SlotIndex].PendingReleaseAbility = Ability;
	SlotStates[SlotIndex].bWaitingForRelease = true;
}

void UNF_SkillTestComponent::ReleaseTestSkillAim(int32 SlotIndex)
{
	if (!SlotStates.IsValidIndex(SlotIndex) || !SlotStates[SlotIndex].bPressed) { return; }
	FNF_TestSkillSlotState& State = SlotStates[SlotIndex];
	State.bPressed = false;
	if (!State.bWaitingForRelease)
	{
		if (FGameplayAbilitySpec* Spec = FindTestSkillSpec(SlotIndex)) { ReleaseSkillInput(Spec->Handle); }
		return;
	}
	UNFGA_SkillBase* Ability = State.PendingReleaseAbility.Get();
	State.bWaitingForRelease = false;
	State.PendingReleaseAbility.Reset();
	FGameplayAbilityTargetDataHandle TargetData;
	if (!Ability || !Ability->ConfirmLocalInput(TargetData))
	{
		UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] Release ignored: input canceled or target invalid."));
		return;
	}
	FGameplayAbilitySpec* Spec = FindTestSkillSpec(SlotIndex);
	const bool bRequested = Spec && (TargetData.Num() > 0
		? RequestSkillActivationWithTarget(Spec->Handle, TargetData) : TryActivateAbility(Spec->Handle));
	UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] %s released | Activation request=%s"),
		*NF_TestSkillKey(SlotIndex).ToString(), bRequested ? TEXT("true") : TEXT("false"));
}

void UNF_SkillTestComponent::CancelTestSkillAim(int32 SlotIndex)
{
	if (!SlotStates.IsValidIndex(SlotIndex)) { return; }
	FNF_TestSkillSlotState& State = SlotStates[SlotIndex];
	const bool bReleaseActive = State.bPressed && !State.bWaitingForRelease;
	UNFGA_SkillBase* Pending = State.PendingReleaseAbility.Get();
	State.bPressed = false; State.bWaitingForRelease = false; State.PendingReleaseAbility.Reset();
	if (Pending) { Pending->CancelLocalInput(); }
	if (bReleaseActive)
	{
		if (FGameplayAbilitySpec* Spec = FindTestSkillSpec(SlotIndex)) { ReleaseSkillInput(Spec->Handle); }
	}
}

void UNF_SkillTestComponent::ActivateTestSkill(int32 SlotIndex)
{
	FGameplayAbilitySpec* Spec = FindTestSkillSpec(SlotIndex);
	UNFGA_ActiveSkillBase* Ability = Spec ? Cast<UNFGA_ActiveSkillBase>(Spec->GetPrimaryInstance()) : nullptr;
	FGameplayAbilityTargetDataHandle TargetData;
	const bool bRequested = Ability && Ability->BuildConfirmedTargetData(TargetData)
		&& (TargetData.Num() > 0 ? RequestSkillActivationWithTarget(Spec->Handle, TargetData) : TryActivateAbility(Spec->Handle));
	UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] %s activation attempt/request=%s | GA=%s"),
		*NF_TestSkillKey(SlotIndex).ToString(), bRequested ? TEXT("true") : TEXT("false"), *GetNameSafe(GetTestSkillClass(SlotIndex).Get()));
}

void UNF_SkillTestComponent::RemoveTestInput()
{
	// A remote server Pawn has no local binding. Do not release its active skills on every server tick.
	if (TestInput || BoundPlayerController.IsValid())
	{
		for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex) { CancelTestSkillAim(SlotIndex); }
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = InputSubsystem.Get())
	{
		if (TestMapping) { Subsystem->RemoveMappingContext(TestMapping); }
	}
	if (APlayerController* PlayerController = BoundPlayerController.Get())
	{
		if (TestInput) { PlayerController->PopInputComponent(TestInput); }
	}
	if (TestInput) { TestInput->DestroyComponent(); }
	TestInput = nullptr; TestMapping = nullptr;
	for (auto& State : SlotStates) { State.Action = nullptr; }
	BoundPlayerController.Reset(); InputSubsystem.Reset();
}

void UNF_SkillTestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveTestInput();
	if (GetOwner()->HasAuthority())
	{
		for (const auto& Handle : TestSkillHandles) { if (Handle.IsValid()) { ClearAbility(Handle); } }
	}
	TestSkillHandles.Reset();
	Super::EndPlay(EndPlayReason);
}
