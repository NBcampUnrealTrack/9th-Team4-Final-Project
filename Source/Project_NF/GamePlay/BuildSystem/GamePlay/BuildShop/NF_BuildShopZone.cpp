#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"

#include "Components/BoxComponent.h"
#include "GamePlay/BuildSystem/Data/NF_BuildRules.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Core/NF_GameState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

void ANF_BuildShopZone::NF_Interaction(
	const APlayerController* InteractedController)
{
	UE_LOG(LogTemp, Warning,
	       TEXT("[BuildShop] Interaction: Shop=%s Authority=%d Controller=%s"),
	       *GetNameSafe(this),
	       HasAuthority(),
	       *GetNameSafe(InteractedController));

	if (!HasAuthority() || !IsValid(InteractedController))
	{
		return;
	}

	APawn* Player = InteractedController->GetPawn();
	if (!IsValid(Player))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuildShop] Pawn is invalid"));
		return;
	}

	const ANF_PlayerState* PlayerState =
		Player->GetPlayerState<ANF_PlayerState>();

	UNF_BuildComponent* Build =
		Player->FindComponentByClass<UNF_BuildComponent>();

	const ANF_GameState* GameState =
		GetWorld()->GetGameState<ANF_GameState>();

	const bool bControllerMatches =
		Player->GetController() == InteractedController;
	const bool bCanBuild = CanBuild(Player);

	UE_LOG(LogTemp, Warning,
	       TEXT("[BuildShop] Player=%s PlayerState=%s ")
	       TEXT("ShopType=%d ShopTeam=%d PlayerTeam=%d ")
	       TEXT("Enabled=%d InRange=%d Distance=%.1f Limit=%.1f ")
	       TEXT("BuildComponent=%s GameState=%s PhaseAllowed=%d ")
	       TEXT("ControllerMatches=%d CanBuild=%d"),
	       *GetNameSafe(Player),
	       *GetNameSafe(PlayerState),
	       static_cast<int32>(ShopType),
	       OwnerTeamId,
	       IsValid(PlayerState) ? PlayerState->NF_TeamID : INDEX_NONE,
	       bShopEnabled,
	       IsActorInRange(Player),
	       FVector::Dist(Player->GetActorLocation(), GetActorLocation()),
	       InteractionDistance,
	       *GetNameSafe(Build),
	       *GetNameSafe(GameState),
	       IsValid(GameState) && GameState->IsBuildShopAllowed(ShopType),
	       bControllerMatches,
	       bCanBuild);

	if (!bCanBuild || !bControllerMatches || !IsValid(Build))
	{
		return;
	}

	const bool bStarted = Build->BeginBuildSession(this);

	UE_LOG(LogTemp, Warning,
	       TEXT("[BuildShop] BeginBuildSession=%d Player=%s CurrentShop=%s"),
	       bStarted,
	       *GetNameSafe(Player),
	       *GetNameSafe(Build->GetCurrentShop()));
}

bool ANF_BuildShopZone::CanBuild(AActor* PlayerActor) const
{
	return IsActorInRange(PlayerActor) && HasBuildAccess(PlayerActor);
}

bool ANF_BuildShopZone::CanProvideFallbackBuild(AActor* PlayerActor) const
{
	return ShopType == ENF_BuildShopType::TeamBase && HasBuildAccess(PlayerActor);
}

bool ANF_BuildShopZone::HasBuildAccess(AActor* PlayerActor) const
{
	if (!bShopEnabled || !IsValid(Rules) || !IsValid(PlayerActor) || PlayerActor->GetWorld() != GetWorld() ||
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
	DOREPLIFETIME(ANF_BuildShopZone, Rules);
}

bool ANF_BuildShopZone::ConfigureRules(UNF_BuildRules* InRules)
{
	if (!HasAuthority() || IsValid(Rules) || !IsValid(InRules)) { return false; }
	Rules = InRules;
	ForceNetUpdate();
	OnRep_Rules();
	return true;
}

void ANF_BuildShopZone::OnRep_Rules()
{
	// CurrentShop이 Rules보다 먼저 도착한 클라이언트의 목록도 다시 갱신한다.
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		UNF_BuildComponent* Build = It->FindComponentByClass<UNF_BuildComponent>();
		if (Build && Build->GetCurrentShop() == this) { Build->OnDraftChanged.Broadcast(); }
	}
}
