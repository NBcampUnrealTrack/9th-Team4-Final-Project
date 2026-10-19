#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"

#include "Components/BoxComponent.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Core/NF_GameState.h"
#include "Engine/World.h"
#include "Player/State/NF_PlayerState.h"

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
	if (!bShopEnabled ||
		!IsActorInRange(PlayerActor) ||
		!PlayerActor->FindComponentByClass<UNF_BuildComponent>())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const ANF_GameState* GameState =
		World ? World->GetGameState<ANF_GameState>() : nullptr;

	// Core가 결정한 현재 페이즈의 접근 정책을 따른다.
	if (!IsValid(GameState) ||
		!GameState->IsBuildShopAllowed(ShopType))
	{
		return false;
	}

	if (ShopType == ENF_BuildShopType::TeamBase)
	{
		const APawn* Pawn = Cast<APawn>(PlayerActor);
		const ANF_PlayerState* PlayerState =
			Pawn ? Pawn->GetPlayerState<ANF_PlayerState>() : nullptr;

		// 팀이 지정되지 않았거나 다른 팀 소유의 거점이면 거절한다.
		if (!IsValid(PlayerState) ||
			OwnerTeamId == INDEX_NONE ||
			PlayerState->NF_TeamID == INDEX_NONE ||
			PlayerState->NF_TeamID != OwnerTeamId)
		{
			return false;
		}
	}

	// Blueprint에서는 추가 조건만 지정한다.
	return IsPlayerAllowed(PlayerActor);
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
