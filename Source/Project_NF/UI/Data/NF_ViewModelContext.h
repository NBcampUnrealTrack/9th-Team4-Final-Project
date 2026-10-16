#pragma once

#include "CoreMinimal.h"

class ULocalPlayer;
class APlayerController;
class APlayerState;
class APawn;
class UAbilitySystemComponent;

struct FNF_ViewModelContext
{
	//UObject 수명이 연장되지 않게 Weak Pointer 사용
	TWeakObjectPtr<ULocalPlayer> LocalPlayer;
	TWeakObjectPtr<APlayerController> PlayerController;
	TWeakObjectPtr<APlayerState> PlayerState;
	TWeakObjectPtr<APawn> Pawn;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	bool HasValidLocalPlayer() const
	{
		return LocalPlayer.IsValid();
	}

	void Reset()
	{
		LocalPlayer.Reset();
		PlayerController.Reset();
		PlayerState.Reset();
		Pawn.Reset();
		AbilitySystemComponent.Reset();
	}
};