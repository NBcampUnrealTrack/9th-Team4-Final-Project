// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/Data/NF_ViewModelContext.h"
#include "NF_ViewModelSubsystem.generated.h"

class APlayerController;
class APawn;
class ANF_BossBase;
class UNF_BossRegistrySubsystem;
class UNF_MonsterStatusViewModel;
class UNF_ViewModelBase;
class UWorld;


//해당 SubSystem의 관리 범위는 각 로컬 플레이어의 UI
UCLASS()
class PROJECT_NF_API UNF_ViewModelSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

	/**
	 * LocalPlayer의 PlayerController가 변경될 때
	 * 엔진에서 호출한다.
	 */
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

	//플레이어의 정보가 변경될 시 ViewModelContext를 각 viewmodel에 재전달
	void RefreshViewModelContext();

	//관리할 ViewModel을 등록
	void RegisterViewModel(UNF_ViewModelBase* ViewModel);

	//관리하는 ViewModel을 제거 및 델리게이트 해제
	void UnregisterViewModel(UNF_ViewModelBase* ViewModel);

	UFUNCTION(BlueprintPure, Category = "NF|UI|ViewModel")
	UNF_MonsterStatusViewModel* GetMonsterStatusViewModel() const { return MonsterStatusViewModel.Get(); }

	const FNF_ViewModelContext& GetCurrentViewModelContext() const
	{
		return CurrentViewModelContext;
	}

private:
	void BindPlayerController(APlayerController* NewPlayerController);

	void UnbindPlayerController();

	//void HandlePawnChanged(APawn* NewPawn);
	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
	
	void BindBossRegistry(UWorld* World);
	void UnbindBossRegistry();
	void HandleActiveBossChanged(ANF_BossBase* NewBoss);

	FNF_ViewModelContext BuildViewModelContext() const;

private:
	FNF_ViewModelContext CurrentViewModelContext;

	TWeakObjectPtr<APlayerController> BoundPlayerController;

	//FDelegateHandle PawnChangedDelegateHandle;
	
	//UPROPERTY 등록 시 viewmodel이 GC처리 되지 않음
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNF_ViewModelBase>> ManagedViewModels;

	UPROPERTY(Transient)
	TObjectPtr<UNF_MonsterStatusViewModel> MonsterStatusViewModel;

	TWeakObjectPtr<UNF_BossRegistrySubsystem> BoundBossRegistry;
	FDelegateHandle ActiveBossChangedDelegateHandle;
};
