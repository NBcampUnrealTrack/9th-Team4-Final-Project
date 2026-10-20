#include "GamePlay/Combat/Skill/Execution/NF_SonarReveal.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

ANF_SonarReveal::ANF_SonarReveal()
{
	bReplicates = true; bOnlyRelevantToOwner = true;
	SetNetUpdateFrequency(10.0f);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
}

bool ANF_SonarReveal::Initialize(APlayerController* Recipient, const TArray<TObjectPtr<ACharacter>>& Targets, float Duration,
	UMaterialInterface* Material, const FNF_SilhouetteStyle& Style)
{
	if (!HasAuthority() || Frame.SourceId.IsValid() || !IsValid(Recipient) || !Material || !Style.IsValid()
		|| !FMath::IsFinite(Duration) || Duration <= 0 || Recipient->GetWorld() != GetWorld()) { return false; }
	SetOwner(Recipient);
	Frame.SourceId = FGuid::NewGuid(); Frame.Targets = Targets; Frame.Material = Material; Frame.Style = Style;
	const AGameStateBase* State = GetWorld()->GetGameState();
	Frame.EndsAtServerTime = (State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) + Duration;
	ServerExpiresAt = GetWorld()->GetTimeSeconds() + Duration;
	Frame.bActive = true;
	SetLifeSpan(Duration + 1.0f); ForceNetUpdate(); RefreshLocalView();
	return true;
}

void ANF_SonarReveal::RefreshLocalView()
{
	auto* Recipient = Cast<APlayerController>(GetOwner());
	if (!Recipient || !Recipient->IsLocalController() || GetNetMode() == NM_DedicatedServer) { return; }
	if (!Frame.bActive)
	{
		if (LocalView.IsValid()) { LocalView->RemoveRequest(Frame.SourceId); }
		return;
	}
	const AGameStateBase* State = GetWorld()->GetGameState();
	if (!HasAuthority() && !State) { return; }
	const double Remaining = Frame.EndsAtServerTime - (State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds());
	if (Remaining <= 0)
	{
		if (LocalView.IsValid()) { LocalView->RemoveRequest(Frame.SourceId); }
		return;
	}
	if (!Frame.Material) { return; }
	if (!LocalView.IsValid()) { LocalView = ANF_SilhouetteView::FindOrCreate(Recipient); }
	if (LocalView.IsValid())
	{
		LocalView->UpdateRequest(Frame.SourceId, Frame.Targets, GetWorld()->GetTimeSeconds() + Remaining, Frame.Material, Frame.Style);
	}
}

void ANF_SonarReveal::OnRep_Frame() { RefreshLocalView(); }

void ANF_SonarReveal::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && Frame.bActive && (!IsValid(GetOwner()) || GetWorld()->GetTimeSeconds() >= ServerExpiresAt))
	{
		StopReveal(); return;
	}
	// Retry owner / target reference resolution; no permanent loss from an early RepNotify.
	RefreshLocalView();
}

void ANF_SonarReveal::StopReveal()
{
	if (!HasAuthority() || !Frame.bActive) { return; }
	Frame.bActive = false;
	RefreshLocalView(); ForceNetUpdate(); SetLifeSpan(0.25f);
}

void ANF_SonarReveal::EndPlay(const EEndPlayReason::Type Reason)
{
	if (LocalView.IsValid()) { LocalView->RemoveRequest(Frame.SourceId); }
	Super::EndPlay(Reason);
}

void ANF_SonarReveal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_SonarReveal, Frame);
}
