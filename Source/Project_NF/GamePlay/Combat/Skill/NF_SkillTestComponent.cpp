#include "GamePlay/Combat/Skill/NF_SkillTestComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "Project_NF.h"

UNF_SkillTestComponent::UNF_SkillTestComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void UNF_SkillTestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UNF_SkillTestComponent, TestSkillClass);
}

void UNF_SkillTestComponent::BeginPlay()
{
	Super::BeginPlay();
	InitAbilityActorInfo(GetOwner(), GetOwner());
	if (GetOwner()->HasAuthority() && TestSkillClass)
	{
		TestSkillHandle = GiveAbility(FGameplayAbilitySpec(TestSkillClass, 1));
	}
	RefreshTestInput();
}

void UNF_SkillTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	// Possess/클라이언트 Controller 복제 시점이 BeginPlay보다 늦어도 입력을 연결한다.
	RefreshTestInput();
}

void UNF_SkillTestComponent::RefreshTestInput()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return;
	}
	if (CachedController.Get() != Pawn->GetController())
	{
		CachedController = Pawn->GetController();
		InitAbilityActorInfo(Pawn, Pawn);
	}

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		RemoveTestInput();
		return;
	}
	if (BoundPlayerController.Get() == PlayerController && TestInput)
	{
		return;
	}
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	RemoveTestInput();
	TestAction = NewObject<UInputAction>(this);
	TestAction->ValueType = EInputActionValueType::Boolean;
	TestAction->bConsumeInput = false;
	TestMapping = NewObject<UInputMappingContext>(this);
	TestMapping->MapKey(TestAction, EKeys::E);
	TestInput = NewObject<UEnhancedInputComponent>(GetOwner());
	TestInput->RegisterComponent();
	TestInput->bBlockInput = false;
	TestInput->BindAction(TestAction, ETriggerEvent::Started, this, &UNF_SkillTestComponent::BeginTestSkillAim);
	TestInput->BindAction(TestAction, ETriggerEvent::Completed, this, &UNF_SkillTestComponent::ReleaseTestSkillAim);
	TestInput->BindAction(TestAction, ETriggerEvent::Canceled, this, &UNF_SkillTestComponent::CancelTestSkillAim);
	PlayerController->PushInputComponent(TestInput);
	Subsystem->AddMappingContext(TestMapping, 1);
	BoundPlayerController = PlayerController;
	InputSubsystem = Subsystem;
}

void UNF_SkillTestComponent::BeginTestSkillAim()
{
	UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] E pressed | Owner=%s | GA=%s | Authority=%s"),
		*GetNameSafe(GetOwner()), *GetNameSafe(TestSkillClass.Get()),
		GetOwner() && GetOwner()->HasAuthority() ? TEXT("true") : TEXT("false"));

	if (!TestSkillClass)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillTest] Test Skill Class is not set."));
		return;
	}
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromClass(TestSkillClass);
	UNFGA_SkillBase* Ability = Spec ? Cast<UNFGA_SkillBase>(Spec->GetPrimaryInstance()) : nullptr;
	if (!Ability)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillTest] Granted skill instance is not ready."));
		return;
	}
	if (!Ability->ShouldActivateOnInputRelease())
	{
		ActivateTestSkill();
		return;
	}
	CancelTestSkillAim();
	if (!Ability->BeginLocalInput())
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillTest] Input rejected. Check skill availability, tags, cost and cooldown."));
		return;
	}
	PendingReleaseAbility = Ability;
	bWaitingForRelease = true;
}

void UNF_SkillTestComponent::ReleaseTestSkillAim()
{
	if (!bWaitingForRelease)
	{
		if (TestSkillClass)
		{
			if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromClass(TestSkillClass)) { ReleaseSkillInput(Spec->Handle); }
		}
		return;
	}
	UNFGA_SkillBase* Ability = PendingReleaseAbility.Get();
	FGameplayAbilityTargetDataHandle TargetData;
	bWaitingForRelease = false;
	PendingReleaseAbility.Reset();
	if (!Ability || !Ability->ConfirmLocalInput(TargetData))
	{
		UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] Release ignored: input canceled or target invalid."));
		return;
	}
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromClass(TestSkillClass);
	const bool bRequested = Spec && (TargetData.Num() > 0
		? RequestSkillActivationWithTarget(Spec->Handle, TargetData) : TryActivateAbility(Spec->Handle));
	UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] E released | Activation request=%s"), bRequested ? TEXT("true") : TEXT("false"));
}

void UNF_SkillTestComponent::CancelTestSkillAim()
{
	if (!bWaitingForRelease && TestSkillClass)
	{
		if (FGameplayAbilitySpec* Spec = FindAbilitySpecFromClass(TestSkillClass)) { ReleaseSkillInput(Spec->Handle); }
	}
	bWaitingForRelease = false;
	if (UNFGA_SkillBase* Ability = PendingReleaseAbility.Get())
	{
		Ability->CancelLocalInput();
	}
	PendingReleaseAbility.Reset();
}

void UNF_SkillTestComponent::ActivateTestSkill()
{
	if (!TestSkillClass)
	{
		return;
	}

	// 원격 서버의 실제 투척 완료 여부가 아니라 로컬 발동 시도/요청 결과다.
	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(TestSkillClass);
	UNFGA_SkillBase* Ability = AbilitySpec ? Cast<UNFGA_SkillBase>(AbilitySpec->GetPrimaryInstance()) : nullptr;
	FGameplayAbilityTargetDataHandle TargetData;
	const bool bActivationRequested = Ability && Ability->BuildConfirmedTargetData(TargetData)
		&& (TargetData.Num() > 0 ? RequestSkillActivationWithTarget(AbilitySpec->Handle, TargetData)
			: TryActivateAbility(AbilitySpec->Handle));
	if (bActivationRequested)
	{
		UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] Activation attempt/request accepted | GA=%s"),
			*GetNameSafe(TestSkillClass.Get()));
	}
	else
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillTest] Activation attempt/request failed | GA=%s | Check ability grant, tags, cost and cooldown."),
			*GetNameSafe(TestSkillClass.Get()));
	}
}

void UNF_SkillTestComponent::RemoveTestInput()
{
	CancelTestSkillAim();
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = InputSubsystem.Get())
	{
		if (TestMapping)
		{
			Subsystem->RemoveMappingContext(TestMapping);
		}
	}
	if (APlayerController* PlayerController = BoundPlayerController.Get())
	{
		if (TestInput)
		{
			PlayerController->PopInputComponent(TestInput);
		}
	}
	if (TestInput)
	{
		TestInput->DestroyComponent();
	}
	TestInput = nullptr;
	TestAction = nullptr;
	TestMapping = nullptr;
	BoundPlayerController.Reset();
	InputSubsystem.Reset();
}

void UNF_SkillTestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveTestInput();
	if (GetOwner()->HasAuthority() && TestSkillHandle.IsValid())
	{
		ClearAbility(TestSkillHandle);
	}
	Super::EndPlay(EndPlayReason);
}
