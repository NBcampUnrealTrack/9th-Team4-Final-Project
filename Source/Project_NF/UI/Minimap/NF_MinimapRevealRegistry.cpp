#include "UI/Minimap/NF_MinimapRevealRegistry.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Player/State/NF_PlayerState.h"

// @owner: 이정준 (구현: Codex) | 2026-10-15
// 목적: 공개 사유를 서버에서 합산하고 클라이언트에는 허용된 좌표 스냅샷만 보관한다.
void UNF_MinimapRevealRegistry::Initialize(APlayerController* InRecipient) { Recipient = InRecipient; }
bool UNF_MinimapRevealRegistry::IsServer() const { return Recipient.IsValid() && Recipient->HasAuthority() && Recipient->GetWorld(); }
double UNF_MinimapRevealRegistry::Now() const { return Recipient.IsValid() && Recipient->GetWorld() ? Recipient->GetWorld()->GetTimeSeconds() : 0.0; }

bool UNF_MinimapRevealRegistry::RegisterReveal(FGuid SourceId, AActor* Target, ENF_MinimapRevealKind Kind, float Lifetime)
{
	if (!IsServer() || !SourceId.IsValid() || !IsValid(Target) || Target->IsActorBeingDestroyed()
		|| Target->GetWorld() != Recipient->GetWorld() || !FMath::IsFinite(Lifetime) || Lifetime < 0.0f) { return false; }
	FName TargetId;
	for (FReveal& Reveal : Reveals)
	{
		if (Reveal.Target.Get() != Target) { continue; }
		TargetId = Reveal.TargetId;
		if (Reveal.SourceId == SourceId)
		{
			Reveal.Kind = Kind; Reveal.ExpiresAt = Lifetime > 0.0f ? Now() + Lifetime : 0.0;
			return true;
		}
	}
	if (TargetId.IsNone()) { TargetId = FName(*FString::Printf(TEXT("Reveal_%llu"), static_cast<unsigned long long>(NextTargetId++))); }
	Reveals.Add({ SourceId, Target, TargetId, Kind, Lifetime > 0.0f ? Now() + Lifetime : 0.0 });
	return true;
}
void UNF_MinimapRevealRegistry::RemoveReveal(FGuid SourceId, AActor* Target)
{
	if (!IsServer()) { return; }
	Reveals.RemoveAll([&](const FReveal& Reveal) { return Reveal.SourceId == SourceId && Reveal.Target.Get() == Target; });
}
void UNF_MinimapRevealRegistry::RemoveRevealSource(FGuid SourceId)
{
	if (!IsServer()) { return; }
	Reveals.RemoveAll([&](const FReveal& Reveal) { return Reveal.SourceId == SourceId; });
}
void UNF_MinimapRevealRegistry::UpdateRevealSource(FGuid SourceId, const TArray<AActor*>& Targets, ENF_MinimapRevealKind Kind, float Lifetime)
{
	if (!IsServer() || !SourceId.IsValid() || !FMath::IsFinite(Lifetime) || Lifetime <= 0.0f) { return; }
	Reveals.RemoveAll([&](const FReveal& Reveal) { return Reveal.SourceId == SourceId && !Targets.Contains(Reveal.Target.Get()); });
	for (AActor* Target : Targets) { RegisterReveal(SourceId, Target, Kind, Lifetime); }
}
void UNF_MinimapRevealRegistry::BuildServerSnapshot(TArray<FNF_MinimapRevealMarker>& Out)
{
	Out.Reset();
	if (!IsServer()) { return; }
	const double Time = Now();
	const UWorld* World = Recipient->GetWorld();
	Reveals.RemoveAll([Time, World](const FReveal& Reveal)
	{
		return !Reveal.Target.IsValid() || Reveal.Target->IsActorBeingDestroyed() || Reveal.Target->GetWorld() != World
			|| (Reveal.ExpiresAt > 0.0 && Reveal.ExpiresAt <= Time);
	});
	for (const FReveal& Reveal : Reveals)
	{
		const FVector Location = Reveal.Target->GetActorLocation();
		if (Location.ContainsNaN()) { continue; }
		FNF_MinimapRevealMarker* Marker = Out.FindByPredicate([&](const FNF_MinimapRevealMarker& Existing) { return Existing.TargetId == Reveal.TargetId; });
		if (!Marker)
		{
			Marker = &Out.AddDefaulted_GetRef(); Marker->TargetId = Reveal.TargetId;
			Marker->WorldLocation = Location; Marker->Kind = Reveal.Kind;
		}
		else if (Reveal.Kind == ENF_MinimapRevealKind::EscapeKey) { Marker->Kind = Reveal.Kind; }
		const auto* Pawn = Cast<APawn>(Reveal.Target.Get());
		Marker->PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
		const auto* State = Pawn ? Pawn->GetPlayerState<ANF_PlayerState>() : nullptr;
		Marker->PlayerTeamID = State ? State->NF_TeamID : INDEX_NONE;
	}
	Out.Sort([](const FNF_MinimapRevealMarker& A, const FNF_MinimapRevealMarker& B) { return A.TargetId.LexicalLess(B.TargetId); });
}
void UNF_MinimapRevealRegistry::ReceiveSnapshot(const TArray<FNF_MinimapRevealMarker>& Markers, float ValidForSeconds)
{
	if (!Recipient.IsValid() || !Recipient->IsLocalController() || !FMath::IsFinite(ValidForSeconds) || ValidForSeconds <= 0.0f) { return; }
	ReceivedMarkers = Markers;
	ReceivedUntil = Now() + ValidForSeconds;
	OnRevealsChanged.Broadcast();
}
TArray<FNF_MinimapRevealMarker> UNF_MinimapRevealRegistry::GetVisibleMarkers() const
{
	return Now() < ReceivedUntil ? ReceivedMarkers : TArray<FNF_MinimapRevealMarker>();
}
void UNF_MinimapRevealRegistry::ExpireClientSnapshot()
{
	if (ReceivedMarkers.IsEmpty() || Now() < ReceivedUntil) { return; }
	ReceivedMarkers.Reset(); OnRevealsChanged.Broadcast();
}
void UNF_MinimapRevealRegistry::Clear()
{
	Reveals.Reset(); ReceivedMarkers.Reset(); ReceivedUntil = 0.0;
	OnRevealsChanged.Broadcast();
}
