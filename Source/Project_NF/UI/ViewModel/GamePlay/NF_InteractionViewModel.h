#pragma once

#include "CoreMinimal.h"
#include "UI/ViewModel/Base/NF_ViewModelBase.h"
#include "NF_InteractionViewModel.generated.h"

class UNF_PlayerInputDataAsset;

UENUM(BlueprintType)
enum class ENF_InteractionTargetType : uint8
{
	None,
	EscapeDoor,
	BuildShop,
	DroppedItem
};

USTRUCT(BlueprintType)
struct FNF_InteractionViewData
{
	GENERATED_BODY()

	//상호작용 대상 타겟팅 여부
	UPROPERTY(BlueprintReadWrite)
	bool bHasTarget = false;

	//상호작용 대상 종류
	UPROPERTY(BlueprintReadWrite)
	ENF_InteractionTargetType TargetType =ENF_InteractionTargetType::None;

	//상호작용 대상 이름
	UPROPERTY(BlueprintReadWrite)
	FText TargetName;

	//상호작용내용
	UPROPERTY(BlueprintReadWrite)
	FText ActionText;

	//상호작용 가능 여부
	UPROPERTY(BlueprintReadWrite)
	bool bCanInteract = false;

	//상호작용 불가능 사유
	UPROPERTY(BlueprintReadWrite)
	FText UnavailableReason;

	//홀드식 상호작용 여부
	UPROPERTY(BlueprintReadWrite)
	bool bIsInteracting = false;

	//진행도 0~1
	UPROPERTY(BlueprintReadWrite)
	float Progress = 0.0f;
};

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_InteractionViewModel : public UNF_ViewModelBase
{
	GENERATED_BODY()

public:
	//상호작용 키를 가져옴
	void ConfigureInput(const UNF_PlayerInputDataAsset* InputDataAsset);

	//상호작용 대상의 상태
	void ApplyInteractionState(const FNF_InteractionViewData& NewState);

	//상호작용 완료 시
	void ClearInteractionState();

	UFUNCTION(BlueprintPure, FieldNotify, Category = "Interaction|UI")
	FText GetInteractionPromptText() const;

private:
	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	bool bShowInteractionPrompt = false;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	ENF_InteractionTargetType TargetType =ENF_InteractionTargetType::None;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	FText TargetName;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	FText InteractionActionText;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	bool bCanInteract = false;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	FText UnavailableReason;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	bool bShowInteractionProgress = false;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	float InteractionProgress = 0.0f;

	UPROPERTY(BlueprintReadOnly,FieldNotify,Category = "Interaction|UI",meta = (AllowPrivateAccess = "true"))
	FText InteractionKeyText;
};