#pragma once
#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "NF_OptionsViewModel.generated.h"

UCLASS(BlueprintType)
class PROJECT_NF_API UNF_OptionsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	void Initialize(UWorld* InWorld);
	void Shutdown();
	virtual UWorld* GetWorld() const override;
	UFUNCTION(BlueprintCallable) void ApplyDisplay();
	UFUNCTION(BlueprintCallable) void ConfirmDisplay();
	UFUNCTION(BlueprintCallable) void RevertDisplay();
	UFUNCTION(BlueprintCallable) void ApplyFrameRateLimit();
	UFUNCTION(BlueprintCallable) void SelectResolution(int32 Index);
	UFUNCTION(BlueprintCallable) void SelectWindowMode(int32 Index);
	UFUNCTION(BlueprintCallable) void SelectFrameRate(int32 Index);
	UFUNCTION(BlueprintCallable) void ChangeMaster(float Value);
	UFUNCTION(BlueprintCallable) void ChangeSFX(float Value);
	UFUNCTION(BlueprintCallable) void ChangeBGM(float Value);
	UPROPERTY(BlueprintReadOnly, FieldNotify) TArray<FString> ResolutionOptions;
	UPROPERTY(BlueprintReadOnly, FieldNotify) TArray<FString> WindowModeOptions;
	UPROPERTY(BlueprintReadOnly, FieldNotify) TArray<FString> FrameRateOptions;
	UPROPERTY(BlueprintReadOnly, FieldNotify) int32 ResolutionIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, FieldNotify) int32 WindowModeIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, FieldNotify) int32 FrameRateIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, FieldNotify) float MasterVolume = 1.0f;
	UPROPERTY(BlueprintReadOnly, FieldNotify) float SFXVolume = 1.0f;
	UPROPERTY(BlueprintReadOnly, FieldNotify) float BGMVolume = 1.0f;
	UPROPERTY(BlueprintReadOnly, FieldNotify) bool bDisplayPending = false;
	UPROPERTY(BlueprintReadOnly, FieldNotify) bool bCanApplyFrameRate = false;
	UPROPERTY(BlueprintReadOnly, FieldNotify) FText Status;
	UPROPERTY(BlueprintReadOnly, FieldNotify) FText FrameRateStatus;
private:
	void RefreshDisplay();
	void RefreshFrameRateLimit();
	void UpdateAvailability();
	TWeakObjectPtr<UWorld> World;
	TArray<float> FrameRateChoices;
	FIntPoint PreviousResolution;
	int32 PreviousWindowMode = 0;
	FTimerHandle RevertTimer;
	bool bInitialized = false;
};
