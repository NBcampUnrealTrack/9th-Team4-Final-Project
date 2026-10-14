
#include "NF_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Data/HelperFunction/NF_GASFunctionLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/NF_Player.h"
#include "Player/Character/TestShop.h"
#include "Player/Data/NF_PlayerInputDataAsset.h"


void ANF_PlayerController::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, CurrentPlayerInteractType);
}

//On_Rep
void ANF_PlayerController::OnRep_OnChangedPlayerStateType()
{
	OnChangedPlayerStateType(CurrentPlayerInteractType);
}


void ANF_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	OnChangedPlayerType.AddUObject(this, &ThisClass::OnChangedPlayerStateType);
	
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


void ANF_PlayerController::OnChangedPlayerStateType(ENF_PlayerInteractTypeEnum NewPlayerType)
{
	CurrentPlayerInteractType = NewPlayerType;
	
	auto* NF_Player = Cast<ANF_Player>(this->GetPawn());
	if (IsValid(NF_Player) == false) return;
	
	if (CurrentPlayerInteractType == ENF_PlayerInteractTypeEnum::InteractingBuild)
	{
		NF_Player->GetCharacterMovement()->SetMovementMode(MOVE_None);
	}
	
	else
	{
		NF_Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}


void ANF_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UMotionMatchingInputLibrary::BindMotionMatchingInput(this, MotionMatchingInputDataAsset);
	
	BindActiveSkill();
}


void ANF_PlayerController::BindActiveSkill()
{
	if (NF_PlayerInputDataAsset == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
		return;
	}
	
	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;
	
	//Skill
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_A, ETriggerEvent::Started, this, &ThisClass::TrySkillAction_A);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButton_B, ETriggerEvent::Started, this, &ThisClass::TrySkillAction_B);
	
	//상호작용
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->InteractButton, ETriggerEvent::Started, this, &ThisClass::TryInteraction);
}


/*void ANF_PlayerController::ServerRPC_OnChangedPlayerStateType_Implementation(ENF_PlayerInteractTypeEnum NewPlayerType)
{
	OnChangedPlayerType.Broadcast(NewPlayerType);
}*/


void ANF_PlayerController::TrySkillAction_A()
{
	//ServerRPC_OnChangedPlayerStateType(ENF_PlayerInteractTypeEnum::InteractingBuild);
	
	if (CurrentPlayerInteractType == ENF_PlayerInteractTypeEnum::InteractingBuild)
	{
		if (SelectedActiveSkill_Tag.IsValid() == true)
		{
			BindActiveSkillButton_A();
			return;
		}
	}
	
	if (SkillButtonA_Tag.IsValid() == false) return;
	
	UE_LOG(LogTemp, Warning, TEXT("SkillA 발동함"));
	UNF_GASFunctionLibrary::NF_TryAbility(this, SkillButtonA_Tag);
	
}


void ANF_PlayerController::BindActiveSkillButton_A()
{
	SkillButtonA_Tag = SelectedActiveSkill_Tag;
	SelectedActiveSkill_Tag = FGameplayTag();
}


void ANF_PlayerController::TrySkillAction_B()
{
	//ServerRPC_OnChangedPlayerStateType(ENF_PlayerInteractTypeEnum::None);
	
	if (CurrentPlayerInteractType == ENF_PlayerInteractTypeEnum::InteractingBuild)
	{
		if (SelectedActiveSkill_Tag.IsValid() == true)
		{
			BindActiveSkillButton_B();
			return;
		}
	}
	
	if (SkillButtonB_Tag.IsValid() == false) return;
	
	UE_LOG(LogTemp, Warning, TEXT("SkillB 발동함"));
	UNF_GASFunctionLibrary::NF_TryAbility(this, SkillButtonB_Tag);
}


void ANF_PlayerController::BindActiveSkillButton_B()
{
	SkillButtonB_Tag = SelectedActiveSkill_Tag;
	SelectedActiveSkill_Tag = FGameplayTag();
}


void ANF_PlayerController::TryInteraction()
{
	ServerRPC_TryInteraction();
}


void ANF_PlayerController::ServerRPC_TryInteraction_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("상호작용 함"));
	
	FHitResult TraceResult = NF_Linetrace();
	
	auto* NF_InteractionInterface = Cast<INF_InteractionInterface>(TraceResult.GetActor());
	if (NF_InteractionInterface == nullptr) return;
	
	NF_InteractionInterface->NF_Interaction(this);
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







