#include "BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"

#include "Components/BoxComponent.h"
#include "BuildSystem/Player/NF_BuildComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

ANF_BuildShopZone::ANF_BuildShopZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	BuildArea = CreateDefaultSubobject<UBoxComponent>(TEXT("BuildArea"));
	SetRootComponent(BuildArea);
	BuildArea->SetBoxExtent(FVector(400.f, 400.f, 200.f));
	BuildArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	BuildArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	BuildArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	BuildArea->SetGenerateOverlapEvents(true);
	BuildArea->OnComponentBeginOverlap.AddDynamic(this, &ANF_BuildShopZone::HandleBeginOverlap);
	BuildArea->OnComponentEndOverlap.AddDynamic(this, &ANF_BuildShopZone::HandleEndOverlap);
}

bool ANF_BuildShopZone::CanBuild(AActor* PlayerActor) const
{
	return bShopEnabled && IsValid(PlayerActor) && PlayerActor->FindComponentByClass<UNF_BuildComponent>() &&
		BuildArea->IsOverlappingActor(PlayerActor) && IsPlayerAllowed(PlayerActor);
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

void ANF_BuildShopZone::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (IsValid(OtherActor) && OtherActor->FindComponentByClass<UNF_BuildComponent>())
	{
		OnPlayerEntered(OtherActor);
	}
}

void ANF_BuildShopZone::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex)
{
	if (IsValid(OtherActor) && OtherActor->FindComponentByClass<UNF_BuildComponent>() &&
		!BuildArea->IsOverlappingActor(OtherActor))
	{
		OnPlayerExited(OtherActor);
	}
}

void ANF_BuildShopZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_BuildShopZone, bShopEnabled);
}
