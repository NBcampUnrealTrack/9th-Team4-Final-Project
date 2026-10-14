#include "NF_Player.h"

#include "AbilitySystemComponent.h"
#include "NFGA_Death.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Component/MotionMatchingComponent.h"
#include "Component/MotionMatchingCharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "NF_PlayerStatusAttribute.h"
#include "BuildSystem/Player/NF_BuildComponent.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GAS/Tags/NFTag_Player.h"
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
}


void ANF_Player::BeginPlay()
{
	Super::BeginPlay();
	
	
}


class UAbilitySystemComponent* ANF_Player::GetAbilitySystemComponent() const
{
	auto* NF_PlayerState = GetPlayerState<ANF_PlayerState>();
	if (IsValid(NF_PlayerState) == false) return nullptr;
	
	return NF_PlayerState->GetAbilitySystemComponent();
}


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


