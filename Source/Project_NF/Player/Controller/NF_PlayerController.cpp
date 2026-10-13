
#include "NF_PlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Input/MotionMatchingInputLibrary.h"
#include "Player/Data/NF_PlayerInputDataAsset.h"


void ANF_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	if (IsLocalController() == true)
	{
		
		auto* EILPS = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer());
		if (IsValid(EILPS) == false) return;

		if (NF_PlayerInputDataAsset == nullptr) return;
		
		EILPS->AddMappingContext(NF_PlayerInputDataAsset->NF_PlayerIMC, 0);
	}
}

void ANF_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UMotionMatchingInputLibrary::BindMotionMatchingInput(this, MotionMatchingInputDataAsset);
	
	auto* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (IsValid(EnhancedInputComponent) == false) return;
	
	BindInputActions(EnhancedInputComponent);
}


void ANF_PlayerController::BindInputActions(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (IsValid(EnhancedInputComponent) == false) return;
	
	if (NF_PlayerInputDataAsset == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("[NF_PlayerInputDataAsset]이 유효하지않아, Key바인딩이 무시됩니다."));
		return;
	}
		
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButtonA, ETriggerEvent::Started, this, &ThisClass::TrySkillActionA);
	EnhancedInputComponent->BindAction(NF_PlayerInputDataAsset->SkillButtonB, ETriggerEvent::Started, this, &ThisClass::TrySkillActionB);
	
}


void ANF_PlayerController::TrySkillActionA()
{
	/*FGameplayTag() 이 부분을 UI가 브로드캐스트로 전달한 Tag값을 바인딩하는 함수에서
	  tag를 전달받아서 그걸 여기에 작성하면 된다. 그럼 나는 미리 UI의 브로드캐스트를 바인딩 해놔야겠지?*/
	//UNF_GASFunctionLibrary::NF_TryAbility(this, FGameplayTag());
	
	UE_LOG(LogTemp, Warning, TEXT("SkillA 발동함"));
	
}

void ANF_PlayerController::TrySkillActionB()
{
	UE_LOG(LogTemp, Warning, TEXT("SkillB 발동함"));
}


