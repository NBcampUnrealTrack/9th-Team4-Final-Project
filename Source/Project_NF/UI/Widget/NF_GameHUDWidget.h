#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NF_GameHUDWidget.generated.h"

class UMVVMViewModelBase;
class UNF_ViewModelBase;
class UNF_ViewModelSubsystem;

// 인게임 HUD 구성과 자식 위젯의 ViewModel 연결을 담당
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API UNF_GameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// WBP_NF_GameHUD의 자식 위젯 이름과 일치해야 한다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> PlayerStatusWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> ActiveSkillSlotOne;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> ActiveSkillSlotTwo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> PassiveSkillSlot;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> MatchStatusWidget;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUserWidget> MinimapWidget;

private:
	void InitializeHUDViewModels();
	void ReleaseHUDViewModels();
	void RegisterOwnedViewModel(UNF_ViewModelBase* ViewModel);
	bool AssignViewModel(UUserWidget* TargetWidget, UMVVMViewModelBase* ViewModel);
	void ClearChildViewModels();

	// HUD가 생성한 VM만 등록 해제
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNF_ViewModelBase>> OwnedViewModels;

	TWeakObjectPtr<UNF_ViewModelSubsystem> ViewModelSubsystem;
	
	bool bViewModelsInitialized = false;
};
