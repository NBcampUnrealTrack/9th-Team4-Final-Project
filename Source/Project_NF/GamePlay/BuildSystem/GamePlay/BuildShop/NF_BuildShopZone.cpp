#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"

#include "Components/BoxComponent.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

ANF_BuildShopZone::ANF_BuildShopZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BuildArea = CreateDefaultSubobject<UBoxComponent>(TEXT("BuildArea"));
	SetRootComponent(BuildArea);
	BuildArea->SetBoxExtent(FVector(50.f, 50.f, 100.f));
	BuildArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BuildArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	BuildArea->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BuildArea->SetGenerateOverlapEvents(false);
}

void ANF_BuildShopZone::NF_Interaction(const APlayerController* InteractedController)
{
	if (!HasAuthority() || !IsValid(InteractedController))
	{
		return;
	}
	APawn* Player = InteractedController->GetPawn();
	if (CanBuild(Player) && Player->GetController() == InteractedController)
	{
		Player->FindComponentByClass<UNF_BuildComponent>()->BeginBuildSession(this);
	}
}

bool ANF_BuildShopZone::CanBuild(AActor* PlayerActor) const
{
	return bShopEnabled && IsActorInRange(PlayerActor) && PlayerActor->FindComponentByClass<UNF_BuildComponent>() &&
		IsPlayerAllowed(PlayerActor);
}

bool ANF_BuildShopZone::IsActorInRange(AActor* PlayerActor) const
{
	return IsValid(PlayerActor) && PlayerActor->GetWorld() == GetWorld() && InteractionDistance >= 0.f &&
		FVector::DistSquared(PlayerActor->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionDistance);
}

bool ANF_BuildShopZone::IsPlayerAllowed_Implementation(AActor* PlayerActor) const
{
	return IsValid(PlayerActor) && PlayerActor->IsA<APawn>();
}

void ANF_BuildShopZone::SetShopEnabled(bool bEnabled)
{
	if (HasAuthority())
	{
		bShopEnabled = bEnabled;
		ForceNetUpdate();
	}
}

void ANF_BuildShopZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_BuildShopZone, bShopEnabled);
}
