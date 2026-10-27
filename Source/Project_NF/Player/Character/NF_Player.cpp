#include "NF_Player.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Component/MotionMatchingComponent.h"
#include "MotionWarpingComponent.h"
#include "NFGA_MeleeAttack.h"
#include "NF_PlayerLevelAttribute.h"
#include "NF_PlayerStatusAttribute.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GamePlay/Combat/Armor/NF_ArmorComponent.h"
#include "GamePlay/Combat/Components/NF_PlayerLifeComponent.h"
#include "GamePlay/Combat/Weapons/NF_WeaponComponent.h"
#include "GAS/Abilities/NFGA_PlayerDeath.h"
#include "GAS/Tags/NFTag_Player.h"
#include "Net/UnrealNetwork.h"
#include "Player/Component/NF_CharacterMovementComponent.h"
#include "Player/Controller/NF_PlayerController.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/State/NF_PlayerState.h"
#include "Engine/LocalPlayer.h"
#include "GamePlay/ConsumableItems/NF_ConsumableComponent.h"
#include "Player/Data/NF_PlayerDataAsset.h"
#include "UI/SubSystem/NF_ViewModelSubsystem.h"
#include "UpperBodyBlend/UpperBodyLayer.h"


ANF_Player::ANF_Player(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UNF_CharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
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
		VisualMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
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
	MeleeAttackMeshComponent =CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MeleeAttackMeshComponent"));
	MeleeAttackMeshComponent->SetupAttachment(VisualMesh, TEXT("MeleeAttackSocket"));
	MeleeAttackMeshComponent->SetHiddenInGame(true);
	
	//Life
	NF_PlayerLifeComponent = CreateDefaultSubobject<UNF_PlayerLifeComponent>(TEXT("NF_PlayerLifeComponent"));
	
	//Consumable
	NF_ConsumableComponent = CreateDefaultSubobject<UNF_ConsumableComponent>(TEXT("NF_ConsumableComponent"));
}


void ANF_Player::BeginPlay()
{
	Super::BeginPlay();
	ConfigBindDelegate();
}


void ANF_Player::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilitySystem();
	
	ConfigData();
}


void ANF_Player::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	
	//클라이언트를 위한 ASC 초기화
	InitAbilitySystem();
	
	ConfigData();
}


void ANF_Player::ConfigBindDelegate()
{
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnPlayerAction, OnTryAction);
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnPlayerBindingSkill, OnSkillBind);
	NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(OnInteractBuildShop, OnEnablePlayerMovementFromType);
}


void ANF_Player::ConfigData()
{
	/*auto* NF_PlayerState = GetPlayerState<ANF_PlayerState>();
	if (IsValid(NF_PlayerState) == false) return;
	
	if (IsValid(NF_PlayerState->NF_PlayerDataAsset) == false) return;

	VisualMesh->SetSkeletalMeshAsset(NF_PlayerState->NF_PlayerDataAsset->VisualMesh);*/
}


#pragma region OnTryAction
void ANF_Player::OnTryAction(APlayerController* SendController, ENF_PlayerActionType ActionType)
{
	if (UNF_PlayerActionDelegateSubsystem::IsControllerMatchedToThisPawn(SendController, this) == false) return;
		
	if (ActionType == ENF_PlayerActionType::MeleeAttack)
	{
		TryMeleeAttack();
		return;
	}
	
	if (ActionType == ENF_PlayerActionType::Fire)
	{
		TryStartFire();
		return;
	}
		
	if (ActionType == ENF_PlayerActionType::StopFire)
	{
		TryStopFire();
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
	
	if (ActionType == ENF_PlayerActionType::UseItem)
	{
		TryUseItem();
		return;
	}
}	

	#pragma region TryAction Function
	void ANF_Player::TryMeleeAttack()
	{
		GetAbilitySystemComponent()->TryActivateAbilityByClass(UNFGA_MeleeAttack::StaticClass());	
	}
	void ANF_Player::TryStartFire()
	{
		UUpperBodyLayerLibrary::SetUpperBodyFireHeld(this, true);
		NF_WeaponComponent->StartFire();
	}
	void ANF_Player::TryStopFire()
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
			FString::Printf(TEXT("%d 번 스킬을 사용 합니다."), InputID + 1));
			
		GetAbilitySystemComponent()->AbilityLocalInputPressed(InputID);
	}

	void ANF_Player::TryInteraction(APlayerController* SendController)
	{
		FHitResult TraceResult = GetActorFromLineTrace();
		
		//해당 수정 사항 제대로 검증 되는지 테스트하기
		
		ServerRPC_TryInteraction(SendController, TraceResult.GetActor());
	
	}

	void ANF_Player::TryUseItem()
	{
		UKismetSystemLibrary::PrintString(this,TEXT("아이템 사용, 정준님 협의 예정"));
		
		NF_ConsumableComponent->NF_UseItem();
	}

	void ANF_Player::ServerRPC_TryInteraction_Implementation(APlayerController* SendController, AActor* TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerRPC_상호작용"));
	
		if (UNF_PlayerActionDelegateSubsystem::IsControllerMatchedToThisPawn(SendController, this) == false) return;
	
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

	NF_PlayerState->InitAbilitySystem(this);
	
	if (HasAuthority() == true)
	{
		NF_ArmorComponent->ClearArmorDefence();
		NF_ArmorComponent->RefreshArmorDefence();
	
		ConfigStatusAttribute();
		ConfigLevelAttribute();
	}
	
	if (IsLocallyControlled() == true)
	{
		auto* PlayerController = GetController<APlayerController>();
		if (IsValid(PlayerController) == false) return;
	
		auto* ViewModelSubsystem = ULocalPlayer::GetSubsystem<UNF_ViewModelSubsystem>(PlayerController->GetLocalPlayer());
		if (IsValid(ViewModelSubsystem) == false) return;

		ViewModelSubsystem->RefreshViewModelContext();
	}
}


void ANF_Player::ConfigStatusAttribute()
{
	auto* PlayerStatusAttribute = GetAbilitySystemComponent()->GetSet<UNF_PlayerStatusAttribute>(); 
	if (IsValid(PlayerStatusAttribute) == true)
	{
		PlayerStatusAttribute->OnOutOfHealth.AddUniqueDynamic(this, &ThisClass::OnOutOfHealthCpp);  
		PlayerStatusAttribute->OnOutOfShield.AddUniqueDynamic(this, &ThisClass::OnOutOfShieldCpp); 
	}
}


void ANF_Player::OnOutOfHealthCpp()
{
	if (HasAuthority() == true)
	{
		//TODO이건 일단 코어가 완료되기전까지 임시용도이다.
		//GetAbilitySystemComponent()->TryActivateAbilityByClass(UNFGA_PlayerDeath::StaticClass());
		
		NF_PlayerLifeComponent->HandleHealthReachedZero();
		GainExpToKiller();
	}
}


void ANF_Player::GainExpToKiller()
{
	auto* VictimPlayerState = GetPlayerState<ANF_PlayerState>();
	if (IsValid(VictimPlayerState) == false) return;
		
	UAbilitySystemComponent* VictimASC = VictimPlayerState->GetAbilitySystemComponent();
	if (IsValid(VictimASC) == false) return;
		
	auto* PlayerStatusAttribute = VictimASC->GetSet<UNF_PlayerStatusAttribute>();
	if (IsValid(PlayerStatusAttribute) == false) return;
		
	auto* NF_KillerPlayerState = Cast<ANF_PlayerState>(PlayerStatusAttribute->GetLethalDamageInstigatorPlayerState());
	if (IsValid(NF_KillerPlayerState) == true)
	{
		UAbilitySystemComponent* KillerASC = NF_KillerPlayerState->GetAbilitySystemComponent();
		KillerASC->BP_ApplyGameplayEffectToSelf(NF_KillerPlayerState->NF_PlayerDataAsset->GainExp_GEClass, 
			1.0f, KillerASC->MakeEffectContext());
		
		
		//TODO 명우님 요청 사항으로 주석 된 내용
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


void ANF_Player::ConfigLevelAttribute()
{
	auto* PlayerLevelAttribute = GetAbilitySystemComponent()->GetSet<UNF_PlayerLevelAttribute>(); 
	if (IsValid(PlayerLevelAttribute) == true)
	{
		PlayerLevelAttribute->OnLevelUp.AddUniqueDynamic(this, &ThisClass::OnLevelUpCpp);
	}
}


void ANF_Player::OnLevelUpCpp(int32 NewLevel)
{
	UKismetSystemLibrary::PrintString(this,
		FString::Printf(TEXT("레벨업! 현재 레벨 : %d"), NewLevel),
		true, true, FColor::Cyan, 5.0f);
}



