// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NF_MatchLobbyPlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;
class UNF_MatchmakingWidget;

/** Player controller behavior used by the matchmaking lobby and local player UI. */
UCLASS()
class PROJECT_NF_API ANF_MatchLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY()
	TObjectPtr<UNF_MatchmakingWidget> MatchmakingWidget;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	bool ShouldUseTouchControls() const;
};
