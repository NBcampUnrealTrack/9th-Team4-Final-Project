#include "GamePlay/Combat/Boss/NF_BossRegistrySubsystem.h"

#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "Core/NF_GameMode.h"
#include "Engine/World.h"

void UNF_BossRegistrySubsystem::Deinitialize()
{
	if (ANF_GameMode* GameMode = SpawnGameMode.Get())
	{
		GameMode->UnregisterBossSpawnHandler(BossSpawnHandler.GetUObject());
	}
	SpawnGameMode.Reset();
	BossSpawnHandler.Unbind();

	Super::Deinitialize();
}

bool UNF_BossRegistrySubsystem::RegisterBossSpawnHandler(FNF_RequestBossSpawn Handler)
{
	UWorld* World = GetWorld();
	ANF_GameMode* GameMode = World ? World->GetAuthGameMode<ANF_GameMode>() : nullptr;
	if (!IsValid(GameMode) || !Handler.IsBound() || !IsValid(Handler.GetUObject()) || BossSpawnHandler.IsBound())
	{
		return false;
	}

	if (!GameMode->RegisterBossSpawnHandler(Handler))
	{
		return false;
	}
	SpawnGameMode = GameMode;
	BossSpawnHandler = MoveTemp(Handler);
	return true;
}

void UNF_BossRegistrySubsystem::UnregisterBossSpawnHandler(const UObject* HandlerOwner)
{
	if (IsValid(HandlerOwner) && BossSpawnHandler.IsBoundToObject(HandlerOwner))
	{
		if (ANF_GameMode* GameMode = SpawnGameMode.Get())
		{
			GameMode->UnregisterBossSpawnHandler(HandlerOwner);
		}
		SpawnGameMode.Reset();
		BossSpawnHandler.Unbind();
	}
}

void UNF_BossRegistrySubsystem::RegisterBoss(ANF_BossBase* Boss)
{
	if (!IsValid(Boss))
	{
		return;
	}

	RegisteredBosses.RemoveAll([](const TWeakObjectPtr<ANF_BossBase>& Entry)
	{
		return !Entry.IsValid();
	});
	RegisteredBosses.AddUnique(Boss);

	if (!ActiveBoss.IsValid())
	{
		SetActiveBoss(Boss);
	}
}

void UNF_BossRegistrySubsystem::UnregisterBoss(ANF_BossBase* Boss)
{
	if (!Boss)
	{
		return;
	}

	RegisteredBosses.RemoveAll([Boss](const TWeakObjectPtr<ANF_BossBase>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == Boss;
	});

	if (ActiveBoss.Get() != Boss)
	{
		return;
	}

	ANF_BossBase* NextBoss = nullptr;
	for (int32 Index = RegisteredBosses.Num() - 1; Index >= 0; --Index)
	{
		if (ANF_BossBase* Candidate = RegisteredBosses[Index].Get())
		{
			NextBoss = Candidate;
			break;
		}
	}

	SetActiveBoss(NextBoss);
}

void UNF_BossRegistrySubsystem::SetActiveBoss(ANF_BossBase* Boss)
{
	if (ActiveBoss.Get() == Boss)
	{
		return;
	}

	ActiveBoss = Boss;
	OnActiveBossChanged.Broadcast(Boss);
}
