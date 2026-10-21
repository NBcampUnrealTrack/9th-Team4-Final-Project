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
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Net/UnrealNetwork.h"
#include "Player/Controller/NF_PlayerController.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/State/NF_PlayerState.h"
#include "UpperBodyBlend/UpperBodyLayer.h"


ANF_Player::ANF_Player(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UMotionMatchingCharacterMovementComponent>
		(ACharacter::CharacterMovementComponentName))
{
	//VisualMesh
	VisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VisualMesh"));
	if (IsValid(GetMesh()) == true)
	{
		GetMesh()->SetHiddenInGame(true);
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		VisualMesh->SetupAttachment(GetMesh());
		VisualMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		VisualMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
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
	MotionMatchingComponent->AddTraversalOwnedTag(NFTag_Player::Player_State_Traversal);
	
	
	//Build
	NF_BuildComponent = CreateDefaultSubobject<UNF_BuildComponent>(TEXT("NF_BuildComponent"));
	
	
	//Equipment
	NF_WeaponComponent = CreateDefaultSubobject<UNF_WeaponComponent>(TEXT("NF_WeaponComponent"));
	NF_ArmorComponent = CreateDefaultSubobject<UNF_ArmorComponent>(TEXT("NF_ArmorComponent"));
	
	
	//Life
	NF_PlayerLifeComponent = CreateDefaultSubobject<UNF_PlayerLifeComponent>(TEXT("NF_PlayerLifeComponent"));
}


void ANF_Player::BeginPlay()
{
	Super::BeginPlay();
	BindDelegate();
}


void ANF_Player::BindDelegate()
{
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnPlayerAction, OnTryAction);
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnPlayerBindingSkill, OnSkillBind);
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnInteractBuildShop, OnEnablePlayerMovementFromType);
}


#pragma region OnTryAction
void ANF_Player::OnTryAction(APlayerController* SendController, ENF_PlayerActionType ActionType)
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
		
	if (ActionType == ENF_PlayerActionType::ZoomIn)
	{
		TryZoom(ActionType);
		return;
	}
	
	if (ActionType == ENF_PlayerActionType::ZoomOut)
	{
		TryZoom(ActionType);
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

	#pragma region TryAction Function
	void ANF_Player::TryStartAttack()
	{
		UUpperBodyLayerLibrary::SetUpperBodyFireHeld(this, true);
		NF_WeaponComponent->StartFire();
	}
	void ANF_Player::TryStopAttack()
	{
		NF_WeaponComponent->StopFire();
		UUpperBodyLayerLibrary::SetUpperBodyFireHeld(this, false);
	}

	void ANF_Player::TryZoom(ENF_PlayerActionType ActionType)
	{
		if (ActionType == ENF_PlayerActionType::ZoomIn)
		{
			NF_WeaponComponent->StartZoom();
			UUpperBodyLayerLibrary::SetUpperBodyZoom(this, true);
		}
	
		if (ActionType == ENF_PlayerActionType::ZoomOut)
		{
			NF_WeaponComponent->StopZoom();
			UUpperBodyLayerLibrary::SetUpperBodyZoom(this, false);
		}	
	}

	void ANF_Player::TryReload()
	{
		NF_WeaponComponent->Reload();
	}

	void ANF_Player::TrySkillActionWithInputID(int32 InputID)
	{
		UKismetSystemLibrary::PrintString(this,
			FString::Printf(TEXT("%d 번 스킬을 사용 합니다."), InputID + 1),
				true, true, FColor::Magenta, 5.0f);
			
		GetAbilitySystemComponent()->AbilityLocalInputPressed(InputID);
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
	#pragma endregion 
#pragma endregion 


void ANF_Player::OnSkillBind(APlayerController* SendController, ENF_PlayerActionType ActionType)
{
	if (UNF_PlayerActionDelegateSubsystem::IsControllerMatchedToThisPawn(SendController, this) == false) return;
	
	if (ActionType == ENF_PlayerActionType::SkillA)
	{
		NF_BuildComponent->AssignSelectedActiveSkill(SkillButtonA_InputID);
		return;
	}
	
	if (ActionType == ENF_PlayerActionType::SkillB)
	{
		NF_BuildComponent->AssignSelectedActiveSkill(SkillButtonB_InputID);
		return;
	}
}


void ANF_Player::OnEnablePlayerMovementFromType(ENF_BuildShopUIState BuildShopType)
{
	if (IsLocallyControlled() == true)
	{
		if (BuildShopType == ENF_BuildShopUIState::BuildOpened)
		{
			GetCharacterMovement()->SetMovementMode(MOVE_None);
		}
	
		if (BuildShopType == ENF_BuildShopUIState::BuildClosed)
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}
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


UAbilitySystemComponent* ANF_Player::GetAbilitySystemComponent() const
{
	auto* NF_PlayerState = GetPlayerState<ANF_PlayerState>();
	if (IsValid(NF_PlayerState) == false) return nullptr;
	
	return NF_PlayerState->GetAbilitySystemComponent();
}

void ANF_Player::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, NF_WeaponType);
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
	
	GiveDefaultAbility();
}


void ANF_Player::GiveDefaultAbility()
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
	GameplayAbilityArray.Add(UNFGA_Death::StaticClass());
}


void ANF_Player::OnOutOfHealthCpp()
{
	UKismetSystemLibrary::PrintString(this,
									  (TEXT("Death 실행 예정")),
									  true, true, FColor::Magenta, 10.0f);
	
	//TODO 지우님 관련 코드로 나중에 다시 정해 질 예정
	/*if (NF_LifeComponent->check동료가있나없나() == Enum::동료있음) //➡️동료가 살았음
	{
		NF_LifeComponent->기절();
		
	}
	
	else if (NF_LifeComponent->check동료가있나없나() == Enum::동료없음) //➡️동료 없음
	{
		NF_LifeComponent->죽음();
	}
	
	else if (NF_LifeComponent->check동료가있나없나() == Enum::Unknown)  //➡️보류
	{
		NF_LifeComponent->bPendingHealthZeroCheck = true;
		
		return;
	}*/
	
	
	UNF_GASFunctionLibrary::NF_TryAbility(this, NFTag_Player::Player_Ability_Death);
	
	if (HasAuthority() == true)
	{
		NF_PlayerLifeComponent->StartHealthZeroDetection();
		
		APlayerState* KillerPlayerState = nullptr;
		if (const ANF_PlayerState* VictimPlayerState = GetPlayerState<ANF_PlayerState>())
		{
			if (const UAbilitySystemComponent* ASC = VictimPlayerState->GetAbilitySystemComponent())
			{
				if (const UNF_PlayerStatusAttribute* PlayerStatusAttribute =
					ASC->GetSet<UNF_PlayerStatusAttribute>())
				{
					KillerPlayerState = PlayerStatusAttribute->GetLethalDamageInstigatorPlayerState();
				}
			}
		}

		/*if (UWorld* World = GetWorld())
		{
			if (auto* GameMode = World->GetAuthGameMode<ANF_GameMode>())
			{
				GameMode->HandlePlayerEliminated(
					GetPlayerState<ANF_PlayerState>(), KillerPlayerState, GetActorLocation(), this);
			}
		}*/
	}
}


void ANF_Player::OnOutOfShieldCpp()
{
		UKismetSystemLibrary::PrintString(this,
    										  (TEXT("쉴드가 깨짐에 대한 로직 실행 예정")),
    										  true, true, FColor::Magenta, 10.0f);
}

