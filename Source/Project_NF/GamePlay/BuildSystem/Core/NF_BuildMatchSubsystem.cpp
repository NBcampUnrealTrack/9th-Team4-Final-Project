#include "GamePlay/BuildSystem/Core/NF_BuildMatchSubsystem.h"

#include "Core/NF_GameMode.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GamePlay/BuildSystem/GamePlay/BuildShop/NF_BuildShopZone.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/BuildSystem/Player/NF_BuildComponent.h"
#include "Player/State/NF_PlayerState.h"

bool UNF_BuildMatchSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return Super::ShouldCreateSubsystem(Outer) && World &&
		(World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE) &&
		World->GetNetMode() != NM_Client;
}

void UNF_BuildMatchSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (InWorld.GetNetMode() == NM_Client || RegisteredGameMode.IsValid())
	{
		return;
	}
	ANF_GameMode* GameMode = InWorld.GetAuthGameMode<ANF_GameMode>();
	if (!IsValid(GameMode))
	{
		return; // 로비 등 다른 GameMode에서는 기본 빌드 연동을 사용하지 않는다.
	}
	if (!GameMode->RegisterFallbackBuildHandler(
		FNF_ApplyFallbackBuild::CreateUObject(this, &ThisClass::HandleApplyFallbackBuild)))
	{
		UE_LOG(LogTemp, Error, TEXT("기본 빌드 콜백 등록 실패: Core API 또는 기존 등록을 확인하세요."));
		return;
	}
	RegisteredGameMode = GameMode;
}

bool UNF_BuildMatchSubsystem::HandleApplyFallbackBuild(ANF_PlayerState* PlayerState)
{
	if (!RegisteredGameMode.IsValid() || !RegisteredGameMode->HasAuthority() ||
		!IsValid(PlayerState) || !PlayerState->HasAuthority() || PlayerState->GetWorld() != GetWorld())
	{
		return false;
	}
	APawn* Pawn = PlayerState->GetPawn();
	if (!IsValid(Pawn) || Pawn->GetPlayerState() != PlayerState)
	{
		return false;
	}
	UNF_BuildComponent* Build = Pawn->FindComponentByClass<UNF_BuildComponent>();
	// 기본 적용은 세션·거리와 무관하게 해당 플레이어가 사용할 수 있는 팀 거점에서 규칙을 받는다.
	ANF_BuildShopZone* FallbackShop = nullptr;
	for (TActorIterator<ANF_BuildShopZone> It(GetWorld()); It; ++It)
	{
		if (!It->CanProvideFallbackBuild(Pawn)) { continue; }
		if (FallbackShop && FallbackShop->GetRules() != It->GetRules())
		{
			UE_LOG(LogTemp, Error, TEXT("기본 빌드 규칙 충돌: 같은 팀 거점의 Rules를 통일하세요."));
			return false;
		}
		FallbackShop = *It;
	}
	const bool bSucceeded = IsValid(Build) && IsValid(FallbackShop) &&
		Build->InitializeAbilitySystem(PlayerState->GetAbilitySystemComponent()) &&
		Build->ApplyFallbackBuildForMatchStart(FallbackShop);
	if (!bSucceeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("기본 빌드 적용 실패: %s. Pawn, ASC와 Rules를 확인하세요."),
		       *GetNameSafe(PlayerState));
	}
	return bSucceeded;
}

void UNF_BuildMatchSubsystem::Deinitialize()
{
	if (RegisteredGameMode.IsValid())
	{
		RegisteredGameMode->UnregisterFallbackBuildHandler(this);
	}
	RegisteredGameMode.Reset();
	Super::Deinitialize();
}
