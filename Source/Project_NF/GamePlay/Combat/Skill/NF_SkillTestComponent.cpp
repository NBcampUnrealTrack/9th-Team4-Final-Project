#include "GamePlay/Combat/Skill/NF_SkillTestComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "GamePlay/Combat/Skill/Active/Attack/NFGA_Grenade.h"
#include "GamePlay/Combat/Projectiles/NF_Grenade.h"
#include "GamePlay/Combat/Projectiles/NF_ProjectileTrajectoryComponent.h"
#include "GamePlay/Combat/Components/NF_AreaIndicatorComponent.h"
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
	UpdateTestSkillPreview();
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
	// 다른 스킬은 기존 즉시 발동 방식 유지. 수류탄만 누르는 동안 조준한다.
	if (!TestSkillClass->IsChildOf(UNFGA_Grenade::StaticClass()))
	{
		ActivateTestSkill();
		return;
	}
	bAiming = true;
	if (!TrajectoryPreview)
	{
		TrajectoryPreview = GetOwner()->FindComponentByClass<UNF_ProjectileTrajectoryComponent>();
		if (!TrajectoryPreview)
		{
			TrajectoryPreview = NewObject<UNF_ProjectileTrajectoryComponent>(GetOwner());
			TrajectoryPreview->RegisterComponent();
		}
	}
	TrajectoryPreview->StartPreview();
	if (!AreaPreview)
	{
		AreaPreview = GetOwner()->FindComponentByClass<UNF_AreaIndicatorComponent>();
		if (!AreaPreview)
		{
			AreaPreview = NewObject<UNF_AreaIndicatorComponent>(GetOwner());
			AreaPreview->RegisterComponent();
		}
		AreaPreview->AddTickPrerequisiteComponent(this);
	}
	UpdateTestSkillPreview();
}

void UNF_SkillTestComponent::ReleaseTestSkillAim()
{
	if (!bAiming)
	{
		return;
	}
	CancelTestSkillAim();
	UE_LOG(LogProject_NF, Log, TEXT("[SkillTest] E released: request grenade throw."));
	ActivateTestSkill();
}

void UNF_SkillTestComponent::CancelTestSkillAim()
{
	bAiming = false;
	if (TrajectoryPreview)
	{
		TrajectoryPreview->StopPreview();
	}
	if (AreaPreview)
	{
		AreaPreview->HideArea();
	}
}

void UNF_SkillTestComponent::UpdateTestSkillPreview()
{
	// 유효한 예측이 없으면 이전 위치를 숨긴다. 유효하면 숨김 없이 위치만 갱신한다.
	const auto HideAreaPreview = [this]()
	{
		if (AreaPreview)
		{
			AreaPreview->HideArea();
		}
	};
	if (!bAiming || !TrajectoryPreview || !TestSkillClass)
	{
		HideAreaPreview();
		return;
	}
	const UNFGA_Grenade* GrenadeAbility = Cast<UNFGA_Grenade>(TestSkillClass->GetDefaultObject());
	if (!GrenadeAbility || !GrenadeAbility->GetGrenadeClass())
	{
		HideAreaPreview();
		return;
	}
	FVector SpawnLocation;
	FVector Velocity;
	if (!GrenadeAbility->CalculateThrowParameters(GetOwner(), SpawnLocation, Velocity))
	{
		HideAreaPreview();
		return;
	}
	const ANF_Grenade* GrenadeDefaults = GrenadeAbility->GetGrenadeClass()->GetDefaultObject<ANF_Grenade>();
	TrajectoryPreview->UpdatePreview(SpawnLocation, Velocity, GrenadeDefaults->GetCollisionRadius(),
		GetWorld()->GetGravityZ() * GrenadeDefaults->GetProjectileGravityScale(), GrenadeDefaults->GetFuseTime());
	FHitResult LandingHit;
	if (AreaPreview && TrajectoryPreview->GetPredictedImpact(LandingHit))
	{
		AreaPreview->ShowArea(LandingHit.ImpactPoint, GrenadeDefaults->GetExplosionRadius(), LandingHit.ImpactNormal);
	}
	else
	{
		HideAreaPreview();
	}
}

void UNF_SkillTestComponent::ActivateTestSkill()
{
	if (!TestSkillClass)
	{
		return;
	}

	// 원격 서버의 실제 투척 완료 여부가 아니라 로컬 발동 시도/요청 결과다.
	const bool bActivationRequested = TryActivateAbilityByClass(TestSkillClass);
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
