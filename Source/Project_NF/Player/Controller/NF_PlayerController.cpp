
#include "NF_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Data/Interface/NF_InteractionInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"
#include "Player/Data/NF_PlayerActionDelegateSubsystem.h"
#include "Player/Data/NF_PlayerInputDataAsset.h"


void ANF_PlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, CurrentPlayerInteractType);
}


void ANF_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	OnPlayerOpenShop.AddUObject(this, &ThisClass::EnablePlayerMovementFromType);
	
	if (IsLocalController() == true)
	{
		auto* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer());
		if (IsValid(EILPS) == false) return;

		if (NF_PlayerInputDataAsset == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
			return;
		}
		
		EILPS->AddMappingContext(NF_PlayerInputDataAsset->NF_PlayerIMC, 0);
	}
}


void ANF_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UMotionMatchingInputLibrary::BindMotionMatchingInput(this, MotionMatchingInputDataAsset);
	
	BindInputButton();
}


void ANF_PlayerController::BindInputButton()
{
	if (NF_PlayerInputDataAsset == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
		return;
	}
	
	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;
	
	//Combat
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Fire);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->FireButton, ETriggerEvent::Completed, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::StopFire);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->ReloadButton, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::Reload);
	
	//Skill
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_A, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillA);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_B, ETriggerEvent::Started, this, 
		&ThisClass::TryAction, ENF_PlayerActionType::SkillB);
	
	//상호작용
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->InteractButton, ETriggerEvent::Started, this,
		&ThisClass::TryAction, ENF_PlayerActionType::Interaction);
}


void ANF_PlayerController::TryAction(ENF_PlayerActionType PlayerActionType)
{
	if (IsInBuildShop() == true) return;
	
	if (PlayerActionType == ENF_PlayerActionType::Fire)
	{
		NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
		return;
	}
	
	if (PlayerActionType == ENF_PlayerActionType::StopFire)
	{
		NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
		return;
	}
	
	if (PlayerActionType == ENF_PlayerActionType::Reload)
	{
		NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
		return;
		
	}
	
	if (PlayerActionType == ENF_PlayerActionType::SkillA)
	{
		NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
		return;
	}
	
	if (PlayerActionType == ENF_PlayerActionType::SkillB)
	{
		NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(OnPlayerAction, this, PlayerActionType);
		return;
		
	}
	
	if (PlayerActionType == ENF_PlayerActionType::Interaction)
	{
		TryInteraction();
		return;
	}
}


#pragma region Interaction
void ANF_PlayerController::ServerRPC_TryInteraction_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("상호작용 함"));
	
	FHitResult TraceResult = NF_Linetrace();
	
	auto* NF_InteractionInterface = Cast<INF_InteractionInterface>(TraceResult.GetActor());
	if (NF_InteractionInterface == nullptr) return;
	
	NF_InteractionInterface->NF_Interaction(this);
}


void ANF_PlayerController::TryInteraction()
{
	ServerRPC_TryInteraction();
}


FHitResult ANF_PlayerController::NF_Linetrace()
{
	FHitResult TraceResult;
	
	APawn* ControlledPawn = GetPawn();   // [추가] InSourceActor → 이 PC가 조종하는 폰
	if (IsValid(ControlledPawn) == false) return TraceResult;
	
	/*액터 기준 start로 하려면 이걸 쓰기
	 *const FVector Start = ControlledPawn->GetActorLocation();
	const FVector End = (Start + (ControlledPawn->GetActorForwardVector() * InteractionRange));*/
	
	FVector ViewLocation; 
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	
	// [추가] 3인칭이면 카메라가 캐릭터 뒤에 있으니 그만큼 사거리 보정
	const float CameraToPawnDistance = FVector::Dist(ViewLocation, ControlledPawn->GetActorLocation());   
	
	const FVector Start = ViewLocation;
	const FVector End = (Start + (ViewRotation.Vector() * (InteractionRange + CameraToPawnDistance))); 
	
	TArray<AActor*> ActorsToIgnore;   // [추가] 자기 폰이 맞지 않도록
	ActorsToIgnore.Add(ControlledPawn);
	
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


#pragma region BuildShop
//On_Rep
void ANF_PlayerController::OnRep_OnChangedPlayerStateType()
{
	EnablePlayerMovementFromType(CurrentPlayerInteractType);
}


//TODO Test용도임, 나중에 지워야함
/*void ANF_PlayerController::ServerRPC_OnChangedPlayerStateType_Implementation(ENF_PlayerInteractTypeEnum NewPlayerType)
{
	OnPlayerOpenShop.Broadcast(NewPlayerType);
}*/


void ANF_PlayerController::EnablePlayerMovementFromType(ENF_PlayerInteractTypeEnum NewPlayerType)
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
}


bool ANF_PlayerController::IsInBuildShop()
{
	// TODO 상점 이용 중에는 스킬 사용을 막음 (이는 준현님의 브로드캐스트를 따를 예정)
	if (CurrentPlayerInteractType == ENF_PlayerInteractTypeEnum::InteractingBuild) return true;
	
	return false;
}
#pragma endregion
