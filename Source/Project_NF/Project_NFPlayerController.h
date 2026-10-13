// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core/Match/Lobby/NF_MatchLobbyPlayerController.h"
#include "Project_NFPlayerController.generated.h"

/** Compatibility base for existing Blueprint classes; new lobby controllers should use ANF_MatchLobbyPlayerController. */
UCLASS(Abstract)
class PROJECT_NF_API AProject_NFPlayerController : public ANF_MatchLobbyPlayerController
{
	GENERATED_BODY()
};
