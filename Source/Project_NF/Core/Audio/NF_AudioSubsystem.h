#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NF_AudioSubsystem.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;

/** Local audio playback and volume routing for one game instance. */
UCLASS(BlueprintType)
class PROJECT_NF_API UNF_AudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|Volume")
	void SaveVolumeSettings();
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "NF|Audio", meta = (WorldContext = "WorldContextObject"))
	static UNF_AudioSubsystem* Get(const UObject* WorldContextObject);

	/** Plays non-spatial audio such as UI sounds on this client. */
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|SFX")
	void PlaySFX(USoundBase* Sound, float Volume = 1.0f, float Pitch = 1.0f, float StartTime = 0.0f);

	/** Plays spatial audio locally. In multiplayer, invoke this on each client that should hear it. */
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|SFX")
	void PlaySFXAtLocation(
		USoundBase* Sound,
		FVector Location,
		FRotator Rotation = FRotator::ZeroRotator,
		float Volume = 1.0f,
		float Pitch = 1.0f,
		float StartTime = 0.0f,
		USoundAttenuation* AttenuationSettings = nullptr,
		USoundConcurrency* ConcurrencySettings = nullptr);

	UFUNCTION(BlueprintCallable, Category = "NF|Audio|BGM")
	void PlayBGM(USoundBase* Sound, float FadeDuration = 1.0f, float Volume = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "NF|Audio|BGM")
	void StopBGM(float FadeOutDuration = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "NF|Audio|Ambient")
	void PlayAmbient(USoundBase* Sound, float FadeDuration = 1.0f, float Volume = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "NF|Audio|Ambient")
	void StopAmbient(float FadeOutDuration = 1.0f);

	UFUNCTION(BlueprintPure, Category = "NF|Audio|Volume")
	float GetMasterVolume() const { return MasterVolume; }
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|Volume")
	void SetMasterVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category = "NF|Audio|Volume")
	float GetSFXVolume() const { return SFXVolume; }
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|Volume")
	void SetSFXVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category = "NF|Audio|Volume")
	float GetBGMVolume() const { return BGMVolume; }
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|Volume")
	void SetBGMVolume(float InVolume);

	UFUNCTION(BlueprintPure, Category = "NF|Audio|Volume")
	float GetAmbientVolume() const { return AmbientVolume; }
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|Volume")
	void SetAmbientVolume(float InVolume);

	/** Temporarily scales BGM without changing the user's saved BGM volume. */
	UFUNCTION(BlueprintCallable, Category = "NF|Audio|BGM")
	void SetBGMDuckMultiplier(float InMultiplier, float FadeDuration = 0.25f);

private:
	float GetEffectiveBGMVolume() const;
	float GetEffectiveAmbientVolume() const;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentBGMComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CurrentAmbientComponent;

	float MasterVolume = 1.0f;
	float SFXVolume = 1.0f;
	float BGMVolume = 1.0f;
	float AmbientVolume = 1.0f;
	float BGMDuckMultiplier = 1.0f;
	float CurrentBGMBaseVolume = 1.0f;
	float CurrentAmbientBaseVolume = 1.0f;
};
