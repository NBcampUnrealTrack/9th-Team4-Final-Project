#include "UI/ViewModel/Settings/NF_OptionsViewModel.h"
#include "Core/Audio/NF_AudioSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "TimerManager.h"

namespace
{
	const TArray<FIntPoint> Resolutions = { {1280,720}, {1920,1080}, {2560,1440} };
}
UWorld* UNF_OptionsViewModel::GetWorld() const { return World.Get(); }

void UNF_OptionsViewModel::Initialize(UWorld* InWorld)
{
	if (bInitialized) return;
	World = InWorld;
	bInitialized = IsValid(InWorld);
	if (!bInitialized) return;
	const TArray<FString> Res = { TEXT("HD · 1280 × 720"), TEXT("FHD · 1920 × 1080"), TEXT("QHD · 2560 × 1440") };
	const TArray<FString> Modes = { TEXT("전체 화면"), TEXT("창 모드") };
	UE_MVVM_SET_PROPERTY_VALUE(ResolutionOptions, Res);
	UE_MVVM_SET_PROPERTY_VALUE(WindowModeOptions, Modes);
	RefreshDisplay();
	RefreshFrameRateLimit();
	if (auto* Audio = UNF_AudioSubsystem::Get(this))
	{
		UE_MVVM_SET_PROPERTY_VALUE(MasterVolume, Audio->GetMasterVolume());
		UE_MVVM_SET_PROPERTY_VALUE(SFXVolume, Audio->GetSFXVolume());
		UE_MVVM_SET_PROPERTY_VALUE(BGMVolume, Audio->GetBGMVolume());
	}
	UE_MVVM_SET_PROPERTY_VALUE(Status, FText::FromString(TEXT("해상도 적용 후 15초 이내에 변경을 유지해 주세요.")));
	UE_MVVM_SET_PROPERTY_VALUE(FrameRateStatus, FText::GetEmpty());
	UpdateAvailability();
}
void UNF_OptionsViewModel::Shutdown()
{
	if (!bInitialized) return;
	RevertDisplay();
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RevertTimer);
	if (auto* Audio = UNF_AudioSubsystem::Get(this)) Audio->SaveVolumeSettings();
	bInitialized = false;
	UpdateAvailability();
	World.Reset();
}
void UNF_OptionsViewModel::UpdateAvailability()
{
	UE_MVVM_SET_PROPERTY_VALUE(bCanApplyFrameRate, bInitialized && !bDisplayPending && FrameRateChoices.IsValidIndex(FrameRateIndex));
}
void UNF_OptionsViewModel::SelectResolution(int32 Index)
{
	if (bInitialized && Resolutions.IsValidIndex(Index)) UE_MVVM_SET_PROPERTY_VALUE(ResolutionIndex, Index);
}
void UNF_OptionsViewModel::SelectWindowMode(int32 Index)
{
	if (bInitialized && WindowModeOptions.IsValidIndex(Index)) UE_MVVM_SET_PROPERTY_VALUE(WindowModeIndex, Index);
}
void UNF_OptionsViewModel::SelectFrameRate(int32 Index)
{
	if (bInitialized && FrameRateChoices.IsValidIndex(Index)) UE_MVVM_SET_PROPERTY_VALUE(FrameRateIndex, Index);
	UpdateAvailability();
}
void UNF_OptionsViewModel::RefreshDisplay()
{
	if (const auto* Settings = UGameUserSettings::GetGameUserSettings())
	{
		UE_MVVM_SET_PROPERTY_VALUE(ResolutionIndex, Resolutions.IndexOfByKey(Settings->GetScreenResolution()));
		const auto Mode = Settings->GetFullscreenMode();
		UE_MVVM_SET_PROPERTY_VALUE(WindowModeIndex, Mode == EWindowMode::Fullscreen ? 0 : (Mode == EWindowMode::Windowed ? 1 : INDEX_NONE));
	}
}
void UNF_OptionsViewModel::RefreshFrameRateLimit()
{
	FrameRateChoices = {30,60,120,144,165,240,0};
	TArray<FString> Labels;
	for (float Value : FrameRateChoices) Labels.Add(Value == 0 ? FString(TEXT("무제한")) : FString::Printf(TEXT("%.0f FPS"), Value));
	int32 Selected = INDEX_NONE;
	if (const auto* Settings = UGameUserSettings::GetGameUserSettings())
	{
		const float Current = Settings->GetFrameRateLimit();
		Selected = FrameRateChoices.IndexOfByPredicate([Current](float Value) { return FMath::IsNearlyEqual(Value, Current); });
		if (Selected == INDEX_NONE)
		{
			Selected = FrameRateChoices.Add(Current);
			Labels.Add(FString::Printf(TEXT("현재 설정: %g FPS"), Current));
		}
	}
	UE_MVVM_SET_PROPERTY_VALUE(FrameRateOptions, Labels);
	UE_MVVM_SET_PROPERTY_VALUE(FrameRateIndex, Selected);
}
void UNF_OptionsViewModel::ApplyFrameRateLimit()
{
	if (!bInitialized) return;
	if (bDisplayPending)
	{
		UE_MVVM_SET_PROPERTY_VALUE(FrameRateStatus, FText::FromString(TEXT("해상도 변경을 먼저 유지하거나 복원해 주세요.")));
		return;
	}
	auto* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings || !FrameRateChoices.IsValidIndex(FrameRateIndex)) return;
	Settings->SetFrameRateLimit(FrameRateChoices[FrameRateIndex]);
	Settings->ApplyNonResolutionSettings();
	Settings->SaveSettings();
	UE_MVVM_SET_PROPERTY_VALUE(FrameRateStatus, FText::FromString(TEXT("FPS 제한을 적용하고 저장했습니다.")));
}
void UNF_OptionsViewModel::ApplyDisplay()
{
	auto* Settings = UGameUserSettings::GetGameUserSettings();
	if (!bInitialized || !GetWorld() || !Settings || !Resolutions.IsValidIndex(ResolutionIndex) || !WindowModeOptions.IsValidIndex(WindowModeIndex)) return;
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		UE_MVVM_SET_PROPERTY_VALUE(Status, FText::FromString(TEXT("해상도 변경은 Standalone 또는 패키징 실행에서 확인해 주세요.")));
		return;
	}
	const FIntPoint Target = Resolutions[ResolutionIndex];
	const auto Mode = WindowModeIndex == 0 ? EWindowMode::Fullscreen : EWindowMode::Windowed;
	if (bDisplayPending) RevertDisplay();
	PreviousResolution = Settings->GetScreenResolution();
	PreviousWindowMode = static_cast<int32>(Settings->GetFullscreenMode());
	Settings->SetScreenResolution(Target);
	Settings->SetFullscreenMode(Mode);
	Settings->ApplyResolutionSettings(false);
	UE_MVVM_SET_PROPERTY_VALUE(bDisplayPending, true);
	RefreshDisplay();
	UpdateAvailability();
	GetWorld()->GetTimerManager().SetTimer(RevertTimer, this, &ThisClass::RevertDisplay, 15.0f, false);
	UE_MVVM_SET_PROPERTY_VALUE(Status, FText::FromString(TEXT("15초 이내에 변경 유지를 누르세요. 미확인 시 자동 복원됩니다.")));
}
void UNF_OptionsViewModel::ConfirmDisplay()
{
	if (!bInitialized || !bDisplayPending) return;
	if (auto* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->ConfirmVideoMode();
		Settings->SaveSettings();
	}
	UE_MVVM_SET_PROPERTY_VALUE(bDisplayPending, false);
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RevertTimer);
	UpdateAvailability();
	UE_MVVM_SET_PROPERTY_VALUE(Status, FText::FromString(TEXT("화면 설정을 저장했습니다.")));
}
void UNF_OptionsViewModel::RevertDisplay()
{
	if (!bDisplayPending) return;
	UE_MVVM_SET_PROPERTY_VALUE(bDisplayPending, false);
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RevertTimer);
	if (auto* Settings = UGameUserSettings::GetGameUserSettings())
	{
		Settings->SetScreenResolution(PreviousResolution);
		Settings->SetFullscreenMode(static_cast<EWindowMode::Type>(PreviousWindowMode));
		Settings->ApplyResolutionSettings(false);
	}
	RefreshDisplay();
	UpdateAvailability();
	UE_MVVM_SET_PROPERTY_VALUE(Status, FText::FromString(TEXT("이전 화면 설정으로 복원했습니다.")));
}
void UNF_OptionsViewModel::ChangeMaster(float Value)
{
	if (!bInitialized) return;
	Value = FMath::IsFinite(Value) ? FMath::Clamp(Value,0.f,1.f) : 1.f;
	if (auto* Audio = UNF_AudioSubsystem::Get(this)) Audio->SetMasterVolume(Value);
	UE_MVVM_SET_PROPERTY_VALUE(MasterVolume, Value);
}
void UNF_OptionsViewModel::ChangeSFX(float Value)
{
	if (!bInitialized) return;
	Value = FMath::IsFinite(Value) ? FMath::Clamp(Value,0.f,1.f) : 1.f;
	if (auto* Audio = UNF_AudioSubsystem::Get(this)) Audio->SetSFXVolume(Value);
	UE_MVVM_SET_PROPERTY_VALUE(SFXVolume, Value);
}
void UNF_OptionsViewModel::ChangeBGM(float Value)
{
	if (!bInitialized) return;
	Value = FMath::IsFinite(Value) ? FMath::Clamp(Value,0.f,1.f) : 1.f;
	if (auto* Audio = UNF_AudioSubsystem::Get(this)) Audio->SetBGMVolume(Value);
	UE_MVVM_SET_PROPERTY_VALUE(BGMVolume, Value);
}
