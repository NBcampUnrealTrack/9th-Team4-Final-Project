#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Subsystems/WorldSubsystem.h"
#include "NF_PlayerActionDelegateSubsystem.generated.h"

enum class ENF_PlayerActionType : uint8;
enum class ENF_BuildShopUIState : uint8;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerAction, APlayerController*, ENF_PlayerActionType);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPlayerBindingSkill, APlayerController*, ENF_PlayerActionType);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnInteractBuildShop, ENF_BuildShopUIState);

UCLASS()
class PROJECT_NF_API UNF_PlayerActionDelegateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 브로드캐스트한 컨트롤러(SendController)가 바인딩된 폰(ThisPawn)을 조종하는 컨트롤러인지 검사
	static bool IsControllerMatchedToThisPawn(const APlayerController* SendController, const APawn* ThisPawn)
	{
		if (IsValid(SendController) == false) return false;
		if (IsValid(ThisPawn) == false) return false;
		
		if (ThisPawn->GetController() == SendController) return true;

		return false;
	}

public:
	FOnPlayerAction OnPlayerAction;
	FOnPlayerBindingSkill OnPlayerBindingSkill;
	FOnInteractBuildShop OnInteractBuildShop;
};


#pragma region Broadcast & Binding Macro

#define NF_BROADCAST_TO_PLAYER_DELEGATE_TwoParams(DelegateVar, OneParams, TwoParams)\
	{\
		UNF_PlayerActionDelegateSubsystem* PADS = nullptr;\
		if (IsValid(GetWorld()) == true) { PADS = GetWorld()->GetSubsystem<UNF_PlayerActionDelegateSubsystem>(); }\
		if (IsValid(PADS) == true) { PADS->DelegateVar.Broadcast(OneParams, TwoParams); }\
	}

#define NF_BROADCAST_TO_PLAYER_DELEGATE_OneParam(DelegateVar, OneParam)\
	{\
		UNF_PlayerActionDelegateSubsystem* PADS = nullptr;\
		if (IsValid(GetWorld()) == true) { PADS = GetWorld()->GetSubsystem<UNF_PlayerActionDelegateSubsystem>(); }\
		if (IsValid(PADS) == true) { PADS->DelegateVar.Broadcast(OneParam); }\
	}

#define NF_BINDING_FUNCTION_TO_PLAYER_DELEGATE(DelegateVar, BindingFunc)\
	{\
		UNF_PlayerActionDelegateSubsystem* PADS = nullptr;\
		if (IsValid(GetWorld()) == true) { PADS = GetWorld()->GetSubsystem<UNF_PlayerActionDelegateSubsystem>(); }\
		if (IsValid(PADS) == true) { PADS->DelegateVar.AddUObject(this, &ThisClass::BindingFunc); }\
	}

#pragma endregion
