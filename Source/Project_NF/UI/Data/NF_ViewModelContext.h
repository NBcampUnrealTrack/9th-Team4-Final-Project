#pragma once

#include "CoreMinimal.h"

class ULocalPlayer;
class APlayerController;
class APlayerState;
class APawn;
class UAbilitySystemComponent;
class UNF_WeaponComponent;

struct FNF_ViewModelContext
{
	//UObject 수명이 연장되지 않게 Weak Pointer 사용
	TWeakObjectPtr<ULocalPlayer> LocalPlayer;
	TWeakObjectPtr<APlayerController> PlayerController;
	TWeakObjectPtr<APlayerState> PlayerState;
	TWeakObjectPtr<APawn> Pawn;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	// 현재 Pawn의 무기 컴포넌트
	TWeakObjectPtr<UNF_WeaponComponent> WeaponComponent;

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
		WeaponComponent.Reset();
	}
};
