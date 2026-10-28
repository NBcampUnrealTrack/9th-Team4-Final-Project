#include "Core/Audio/NF_AudioSubsystem.h"

#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Misc/ConfigCacheIni.h"

void UNF_AudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	float Value = 1.0f;
	if (GConfig->GetFloat(TEXT("NF.Audio"), TEXT("Master"), Value, GGameUserSettingsIni)) SetMasterVolume(Value);
	if (GConfig->GetFloat(TEXT("NF.Audio"), TEXT("SFX"), Value, GGameUserSettingsIni)) SetSFXVolume(Value);
	if (GConfig->GetFloat(TEXT("NF.Audio"), TEXT("BGM"), Value, GGameUserSettingsIni)) SetBGMVolume(Value);
}

void UNF_AudioSubsystem::SaveVolumeSettings()
{
	GConfig->SetFloat(TEXT("NF.Audio"), TEXT("Master"), MasterVolume, GGameUserSettingsIni);
	GConfig->SetFloat(TEXT("NF.Audio"), TEXT("SFX"), SFXVolume, GGameUserSettingsIni);
	GConfig->SetFloat(TEXT("NF.Audio"), TEXT("BGM"), BGMVolume, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

namespace
{
	bool CanPlayAudio(const UWorld* World)
	{
		return IsValid(World) && World->GetNetMode() != NM_DedicatedServer;
	}

	float SanitizeNonNegative(const float Value, const float Fallback)
	{
		return FMath::IsFinite(Value) ? FMath::Max(0.0f, Value) : Fallback;
	}

	float SanitizePitch(const float Pitch)
	{
		return FMath::IsFinite(Pitch) ? FMath::Max(0.01f, Pitch) : 1.0f;
	}
}

void UNF_AudioSubsystem::Deinitialize()
{
	if (IsValid(CurrentBGMComponent))
	{
		CurrentBGMComponent->Stop();
		CurrentBGMComponent = nullptr;
	}
	if (IsValid(CurrentAmbientComponent))
	{
		CurrentAmbientComponent->Stop();
		CurrentAmbientComponent = nullptr;
	}

	Super::Deinitialize();
}

UNF_AudioSubsystem* UNF_AudioSubsystem::Get(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return IsValid(GameInstance) ? GameInstance->GetSubsystem<UNF_AudioSubsystem>() : nullptr;
}

void UNF_AudioSubsystem::PlaySFX(
	USoundBase* Sound,
	const float Volume,
	const float Pitch,
	const float StartTime)
{
	UWorld* World = GetWorld();
	if (!IsValid(Sound) || !CanPlayAudio(World))
	{
		return;
	}

	UGameplayStatics::PlaySound2D(
		World,
		Sound,
		SanitizeNonNegative(Volume, 1.0f) * SFXVolume * MasterVolume,
		SanitizePitch(Pitch),
		SanitizeNonNegative(StartTime, 0.0f));
}

void UNF_AudioSubsystem::PlaySFXAtLocation(
	USoundBase* Sound,
	const FVector Location,
	const FRotator Rotation,
	const float Volume,
	const float Pitch,
	const float StartTime,
	USoundAttenuation* AttenuationSettings,
	USoundConcurrency* ConcurrencySettings)
{
	UWorld* World = GetWorld();
	if (!IsValid(Sound) || !CanPlayAudio(World))
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		World,
		Sound,
		Location,
		Rotation,
		SanitizeNonNegative(Volume, 1.0f) * SFXVolume * MasterVolume,
		SanitizePitch(Pitch),
		SanitizeNonNegative(StartTime, 0.0f),
		AttenuationSettings,
		ConcurrencySettings);
}

void UNF_AudioSubsystem::PlayBGM(USoundBase* Sound, const float FadeDuration, const float Volume)
{
	UWorld* World = GetWorld();
	if (!IsValid(Sound) || !CanPlayAudio(World))
	{
		return;
	}

	const float SafeFadeDuration = SanitizeNonNegative(FadeDuration, 1.0f);
	CurrentBGMBaseVolume = SanitizeNonNegative(Volume, 1.0f);
	const float TargetVolume = GetEffectiveBGMVolume();

	if (IsValid(CurrentBGMComponent) && CurrentBGMComponent->IsPlaying()
		&& CurrentBGMComponent->GetSound() == Sound)
	{
		CurrentBGMComponent->SetVolumeMultiplier(TargetVolume);
		return;
	}

	UAudioComponent* PreviousComponent = CurrentBGMComponent;
	UAudioComponent* NewComponent = UGameplayStatics::CreateSound2D(
		World, Sound, 0.0f, 1.0f, 0.0f, nullptr, true, true);
	if (!IsValid(NewComponent))
	{
		return;
	}

	CurrentBGMComponent = NewComponent;
	NewComponent->FadeIn(SafeFadeDuration, TargetVolume);

	if (IsValid(PreviousComponent) && PreviousComponent->IsPlaying())
	{
		PreviousComponent->FadeOut(SafeFadeDuration, 0.0f);
	}
}

void UNF_AudioSubsystem::StopBGM(const float FadeOutDuration)
{
	UAudioComponent* ComponentToStop = CurrentBGMComponent;
	CurrentBGMComponent = nullptr;
	if (!IsValid(ComponentToStop) || !ComponentToStop->IsPlaying())
	{
		return;
	}

	ComponentToStop->FadeOut(SanitizeNonNegative(FadeOutDuration, 1.0f), 0.0f);
}

void UNF_AudioSubsystem::PlayAmbient(USoundBase* Sound, const float FadeDuration, const float Volume)
{
	UWorld* World = GetWorld();
	if (!IsValid(Sound) || !CanPlayAudio(World))
	{
		return;
	}

	const float SafeFadeDuration = SanitizeNonNegative(FadeDuration, 1.0f);
	CurrentAmbientBaseVolume = SanitizeNonNegative(Volume, 1.0f);
	const float TargetVolume = GetEffectiveAmbientVolume();

	if (IsValid(CurrentAmbientComponent) && CurrentAmbientComponent->IsPlaying()
		&& CurrentAmbientComponent->GetSound() == Sound)
	{
		CurrentAmbientComponent->SetVolumeMultiplier(TargetVolume);
		return;
	}

	UAudioComponent* PreviousComponent = CurrentAmbientComponent;
	UAudioComponent* NewComponent = UGameplayStatics::CreateSound2D(
		World, Sound, 0.0f, 1.0f, 0.0f, nullptr, true, true);
	if (!IsValid(NewComponent))
	{
		return;
	}

	CurrentAmbientComponent = NewComponent;
	NewComponent->FadeIn(SafeFadeDuration, TargetVolume);

	if (IsValid(PreviousComponent) && PreviousComponent->IsPlaying())
	{
		PreviousComponent->FadeOut(SafeFadeDuration, 0.0f);
	}
}

void UNF_AudioSubsystem::StopAmbient(const float FadeOutDuration)
{
	UAudioComponent* ComponentToStop = CurrentAmbientComponent;
	CurrentAmbientComponent = nullptr;
	if (!IsValid(ComponentToStop) || !ComponentToStop->IsPlaying())
	{
		return;
	}

	ComponentToStop->FadeOut(SanitizeNonNegative(FadeOutDuration, 1.0f), 0.0f);
}

void UNF_AudioSubsystem::SetMasterVolume(const float InVolume)
{
	MasterVolume = FMath::IsFinite(InVolume) ? FMath::Clamp(InVolume, 0.0f, 1.0f) : 1.0f;
	if (IsValid(CurrentBGMComponent))
	{
		CurrentBGMComponent->SetVolumeMultiplier(GetEffectiveBGMVolume());
	}
	if (IsValid(CurrentAmbientComponent))
	{
		CurrentAmbientComponent->SetVolumeMultiplier(GetEffectiveAmbientVolume());
	}
}

void UNF_AudioSubsystem::SetSFXVolume(const float InVolume)
{
	SFXVolume = FMath::IsFinite(InVolume) ? FMath::Clamp(InVolume, 0.0f, 1.0f) : 1.0f;
}

void UNF_AudioSubsystem::SetBGMVolume(const float InVolume)
{
	BGMVolume = FMath::IsFinite(InVolume) ? FMath::Clamp(InVolume, 0.0f, 1.0f) : 1.0f;
	if (IsValid(CurrentBGMComponent))
	{
		CurrentBGMComponent->SetVolumeMultiplier(GetEffectiveBGMVolume());
	}
}

void UNF_AudioSubsystem::SetAmbientVolume(const float InVolume)
{
	AmbientVolume = FMath::IsFinite(InVolume) ? FMath::Clamp(InVolume, 0.0f, 1.0f) : 1.0f;
	if (IsValid(CurrentAmbientComponent))
	{
		CurrentAmbientComponent->SetVolumeMultiplier(GetEffectiveAmbientVolume());
	}
}

void UNF_AudioSubsystem::SetBGMDuckMultiplier(const float InMultiplier, const float FadeDuration)
{
	BGMDuckMultiplier = FMath::IsFinite(InMultiplier) ? FMath::Clamp(InMultiplier, 0.0f, 1.0f) : 1.0f;
	if (IsValid(CurrentBGMComponent))
	{
		CurrentBGMComponent->AdjustVolume(
			SanitizeNonNegative(FadeDuration, 0.25f),
			GetEffectiveBGMVolume());
	}
}

float UNF_AudioSubsystem::GetEffectiveBGMVolume() const
{
	return CurrentBGMBaseVolume * BGMVolume * MasterVolume * BGMDuckMultiplier;
}

float UNF_AudioSubsystem::GetEffectiveAmbientVolume() const
{
	return CurrentAmbientBaseVolume * AmbientVolume * MasterVolume;
}
