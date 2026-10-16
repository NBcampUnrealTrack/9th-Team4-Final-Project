#include "NF_Player.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "NFGA_Death.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Component/MotionMatchingComponent.h"
#include "Component/MotionMatchingCharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "NF_PlayerStatusAttribute.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Player/Controller/NF_PlayerController.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/State/NF_PlayerState.h"


ANF_Player::ANF_Player(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMotionMatchingCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	//VisualMesh
	VisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VisualMesh"));
	if (IsValid(GetMesh()) == true)
	{
		GetMesh()->SetHiddenInGame(true);
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		VisualMesh->SetupAttachment(GetMesh());
		VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
	
	
	//CameraSet
	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArmComponent->SetupAttachment(GetRootComponent());
	SpringArmComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	SpringArmComponent->TargetArmLength = 300.0f;
	SpringArmComponent->bUsePawnControlRotation = true;
	SpringArmComponent->bEnableCameraLag = true;
	SpringArmComponent->CameraLagSpeed = 10.0f;
	
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(SpringArmComponent, USpringArmComponent::SocketName);
	CameraComponent->SetFieldOfView(100.0f);
	CameraComponent->bUsePawnControlRotation = false;

	
	//MotionMatching
	MotionMatchingComponent = CreateDefaultSubobject<UMotionMatchingComponent>(TEXT("MotionMatchingComponent"));
	CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarping"));
	
	//Build
	NF_BuildComponent = CreateDefaultSubobject<UNF_BuildComponent>(TEXT("NF_BuildComponent"));
	
	NF_WeaponComponent = CreateDefaultSubobject<UNF_WeaponComponent>(TEXT("NF_WeaponComponent"));
}


void ANF_Player::BeginPlay()
{
	Super::BeginPlay();
	
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnPlayerAction, TryAction);
}


void ANF_Player::TryAction(APlayerController* SendController, ENF_PlayerActionType ActionType)
{
	if (UNF_PlayerActionDelegateSubsystem::IsControllerMatchedToThisPawn(SendController, this) == false) return;
	
	if (ActionType == ENF_PlayerActionType::Fire)
	{
		TryStartAttack();
		return;
	}
	
	if (ActionType == ENF_PlayerActionType::StopFire)
	{
		TryStopAttack();
		return;
	}
	
	if (ActionType == ENF_PlayerActionType::Reload)
	{
		TryReload();
		return;
	}
	
	if (ActionType == ENF_PlayerActionType::SkillA)
	{
		TrySkillActionWithInputID(SkillButtonA_InputID);
		return;
	}
	
	if (ActionType == ENF_PlayerActionType::SkillB)
	{
		TrySkillActionWithInputID(SkillButtonB_InputID);
		return;
	}
		
	if (ActionType == ENF_PlayerActionType::Interaction)
	{
		TryInteraction(SendController);
		return;
	}
}


UAbilitySystemComponent* ANF_Player::GetAbilitySystemComponent() const
{
	auto* NF_PlayerState = GetPlayerState<ANF_PlayerState>();
	if (IsValid(NF_PlayerState) == false) return nullptr;
	
	return NF_PlayerState->GetAbilitySystemComponent();
}


void ANF_Player::TryStartAttack()
{
	NF_WeaponComponent->StartFire();
}


void ANF_Player::TryStopAttack()
{
	NF_WeaponComponent->StopFire();
}


void ANF_Player::TryReload()
{
	NF_WeaponComponent->Reload();
}


void ANF_Player::TrySkillActionWithInputID(int32 InputID)
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(this);
	if (IsValid(ASC) == false) return;

	ASC->AbilityLocalInputPressed(InputID);
}


void ANF_Player::TryInteraction(APlayerController* SendController)
{
	FHitResult TraceResult = GetActorFromLineTrace();
	ServerRPC_TryInteraction(SendController, TraceResult.GetActor());
}


void ANF_Player::ServerRPC_TryInteraction_Implementation(APlayerController* SendController, AActor* TargetActor)
{
	UE_LOG(LogTemp, Warning, TEXT("ServerRPC_상호작용"));
	
	auto* NF_InteractionInterface = Cast<INF_InteractionInterface>(TargetActor);
	if (NF_InteractionInterface == nullptr) return;
	
	NF_InteractionInterface->NF_Interaction(SendController);
}


FHitResult ANF_Player::GetActorFromLineTrace()
{
	FHitResult TraceResult;
	
	const FVector ViewLocation = CameraComponent->GetComponentLocation(); 
	const FVector ViewDirection = CameraComponent->GetForwardVector();
	
	//카메라가 캐릭터 뒤에 있으니 그만큼 사거리 보정
	const float CameraToPawnDistance = FVector::Dist(ViewLocation, this->GetActorLocation());   
	
	const FVector Start = ViewLocation;
	const FVector End = (Start + (ViewDirection * (InteractionRange + CameraToPawnDistance))); 
	
	TArray<AActor*> ActorsToIgnore;   //자기 자신은 검출 제외
	ActorsToIgnore.Add(this);
	
	const bool bHit = UKismetSystemLibrary::LineTraceSingle( 
	this,
	Start,
	End,
	ETraceTypeQuery::TraceTypeQuery1,
	false,
	ActorsToIgnore, 
	EDrawDebugTrace::ForDuration,
	TraceResult,
	true,
	FColor::Red,
	FColor::Green,
	5.0f
	);
	
	if (bHit == false) return TraceResult;
	
	return TraceResult;
}


/*void ANF_Player::EnablePlayerMovementFromType(ENF_PlayerInteractTypeEnum NewPlayerType)
{
	CurrentPlayerInteractType = NewPlayerType;
	
	auto* NF_Player = Cast<ANF_Player>(this->GetPawn());
	if (IsValid(NF_Player) == false) return;
	
	// TODO 상점 이용 중에는 스킬 사용을 막음 (이는 준현님의 브로드캐스트를 따를 예정)
	if (CurrentPlayerInteractType == ENF_PlayerInteractTypeEnum::InteractingBuild)
	{
		NF_Player->GetCharacterMovement()->SetMovementMode(MOVE_None);
	}
	
	else
	{
		NF_Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}*/


void ANF_Player::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
}


void ANF_Player::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilitySystem();
	
}


void ANF_Player::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	//클라이언트를 위한 ASC 초기화
	InitAbilitySystem();
}


void ANF_Player::InitAbilitySystem()
{
	auto* NF_PlayerState = GetPlayerState<ANF_PlayerState>();
	if (IsValid(NF_PlayerState) == false) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (IsValid(ASC) == false) return;

	ASC->InitAbilityActorInfo(NF_PlayerState, this);
	
	const UNF_PlayerStatusAttribute* PlayerStatusAttribute = ASC->GetSet<UNF_PlayerStatusAttribute>(); 
	if (IsValid(PlayerStatusAttribute) == true)
	{
		PlayerStatusAttribute->OnOutOfHealth.AddUniqueDynamic(this, &ThisClass::OnOutOfHealthCpp);  
		PlayerStatusAttribute->OnOutOfShield.AddUniqueDynamic(this, &ThisClass::OnOutOfShieldCpp); 
	}
	
	GiveAbility();
}


void ANF_Player::GiveAbility()
{
	if (HasAuthority() == true)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
		if (IsValid(ASC) == false) return;
		
		AddGameplayAbility();
		for (const auto& EachAbility : GameplayAbilityArray)
		{
			if (IsValid(EachAbility) == false) continue;
			
			if (ASC->FindAbilitySpecFromClass(EachAbility) == nullptr)
			{
				ASC->GiveAbility(FGameplayAbilitySpec(EachAbility));
			}
		}
	}
}


void ANF_Player::AddGameplayAbility()
{
	//GameplayAbilityArray.Add(UNFGA_Death::StaticClass()); //이건 예시이고, 실제로 어빌리티 왕창 추가해놔야함
	
}


void ANF_Player::OnOutOfHealthCpp()
{
	UKismetSystemLibrary::PrintString(this,
										  (TEXT("Death 실행 예정")),
										  true, true, FColor::Magenta, 10.0f);
	
	UNF_GASFunctionLibrary::NF_TryAbility(this, NFTag_Player::Player_Ability_Death);
}


void ANF_Player::OnOutOfShieldCpp()
{
		UKismetSystemLibrary::PrintString(this,
    										  (TEXT("쉴드가 깨짐에 대한 로직 실행 예정")),
    										  true, true, FColor::Magenta, 10.0f);
}

