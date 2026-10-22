#include "GamePlay/EscapeZone/NF_EscapeZone.h"

#include "Components/SphereComponent.h"
#include "Core/NF_GameMode.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Player/State/NF_PlayerState.h"

ANF_EscapeZone::ANF_EscapeZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;

	EscapeRange = CreateDefaultSubobject<USphereComponent>(TEXT("EscapeRange"));
	SetRootComponent(EscapeRange);
	EscapeRange->InitSphereRadius(300.0f);
	EscapeRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	EscapeRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	EscapeRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	EscapeRange->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	EscapeRange->SetGenerateOverlapEvents(true);
	EscapeRange->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleRangeEndOverlap);
}

void ANF_EscapeZone::NF_Interaction(const APlayerController* InteractedController)
{
	if (!HasAuthority() || !IsValid(InteractedController) || InteractedController->GetWorld() != GetWorld()) return;

	ANF_GameMode* GameMode = GetWorld()->GetAuthGameMode<ANF_GameMode>();
	ANF_PlayerState* Requester = InteractedController->GetPlayerState<ANF_PlayerState>();

	if (IsValid(GameMode) && IsValid(Requester))
	{
		GameMode->RequestEscape(this, Requester);
	}
}

bool ANF_EscapeZone::ContainsPawn(const APawn* Pawn) const
{
	if (!IsValid(Pawn) || Pawn->GetWorld() != GetWorld()) return false;

	const float Radius = EscapeRange->GetScaledSphereRadius();
	return FVector::DistSquared(EscapeRange->GetComponentLocation(), Pawn->GetActorLocation()) <= FMath::Square(Radius);
}

float ANF_EscapeZone::GetEscapeDuration() const
{
	if (!FMath::IsFinite(EscapeDuration)) return 5.0f;
	return FMath::Max(0.1f, EscapeDuration);
}

FNF_EscapeProgress ANF_EscapeZone::GetEscapeProgress() const
{
	return EscapeProgress;
}

void ANF_EscapeZone::AuthoritySetProgress(const FNF_EscapeProgress& NewProgress)
{
	if (!HasAuthority()) return;

	EscapeProgress = NewProgress;
	ForceNetUpdate();
}

void ANF_EscapeZone::OnRep_EscapeProgress()
{
	OnEscapeProgressChanged(EscapeProgress);
}

void ANF_EscapeZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_EscapeZone, EscapeProgress);
}

void ANF_EscapeZone::HandleRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!IsValid(Pawn) || ContainsPawn(Pawn)) return;

	if (ANF_GameMode* GameMode = GetWorld()->GetAuthGameMode<ANF_GameMode>())
	{
		GameMode->NotifyEscapeRangeExited(this, Pawn->GetPlayerState<ANF_PlayerState>());
	}
}
