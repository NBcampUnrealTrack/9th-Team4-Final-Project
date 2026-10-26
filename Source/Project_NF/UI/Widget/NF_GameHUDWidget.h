#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_GameHUDWidget.generated.h"

class UMVVMViewModelBase;
class UNF_ViewModelBase;
class UNF_ViewModelSubsystem;

class UNF_PlayerStatusWidget;
class UNF_SkillSlotWidget;
class UNF_MinimapWidget;
class UNF_TeamPanelWidget;
class UNF_WeaponStatusWidget;

class UNF_TeamPanelViewModel;
class UNF_WeaponStatusViewModel;

// 인게임 HUD 구성과 자식 위젯의 ViewModel 연결을 담당
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_GameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "NF|UI|HUD")
	UNF_TeamPanelViewModel* GetTeamPanelViewModel() const;

	UFUNCTION(BlueprintPure, Category = "NF|UI|HUD")
	UNF_WeaponStatusViewModel* GetWeaponStatusViewModel() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// WBP_NF_GameHUD의 자식 위젯 이름과 일치해야 한다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNF_PlayerStatusWidget> PlayerStatusWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNF_SkillSlotWidget> ActiveSkillSlotOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNF_SkillSlotWidget> ActiveSkillSlotTwo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNF_SkillSlotWidget> PassiveSkillSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> MatchStatusWidget;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNF_MinimapWidget> MinimapWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNF_TeamPanelWidget> TeamPanelWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UNF_WeaponStatusWidget> WeaponStatusWidget;

private:
	void InitializeHUDViewModels();
	void ReleaseHUDViewModels();
	void RegisterOwnedViewModel(UNF_ViewModelBase* ViewModel);
	bool AssignViewModel(UUserWidget* TargetWidget, UMVVMViewModelBase* ViewModel);
	void ClearChildViewModels();

	UPROPERTY(Transient)
	TObjectPtr<UNF_TeamPanelViewModel> TeamPanelVM;

	UPROPERTY(Transient)
	TObjectPtr<UNF_WeaponStatusViewModel> WeaponStatusVM;

	// HUD가 생성한 VM만 등록 해제
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNF_ViewModelBase>> OwnedViewModels;

	TWeakObjectPtr<UNF_ViewModelSubsystem> ViewModelSubsystem;
	
	bool bViewModelsInitialized = false;
};
