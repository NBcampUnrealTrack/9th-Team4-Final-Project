#include "NF_Player.h"

#include "AbilitySystemComponent.h"
#include "GA_Test.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Component/MotionMatchingComponent.h"
#include "Component/MotionMatchingCharacterMovementComponent.h"
#include "MotionWarpingComponent.h"
#include "Data/HelperFunction/NF_GASFunctionLibraryComponent.h"
#include "Player/State/NF_PlayerState.h"


ANF_Player::ANF_Player(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMotionMatchingCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	
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

	
	//Helper
	NF_GASFunctionLibraryComponent = CreateDefaultSubobject<UNF_GASFunctionLibraryComponent>(TEXT("NF_GASFunctionLibraryComponent"));
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

	UAbilitySystemComponent* ASC = NF_PlayerState->GetAbilitySystemComponent();
	if (IsValid(ASC) == false) return;

	ASC->InitAbilityActorInfo(NF_PlayerState, this);
	
	
	if (HasAuthority() == true)
	{
		GameplayAbilityArray.Add(UGA_Test::StaticClass());
	
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
