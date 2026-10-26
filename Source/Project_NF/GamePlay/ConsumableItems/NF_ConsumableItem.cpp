#include "GamePlay/ConsumableItems/NF_ConsumableItem.h"

ANF_ConsumableItem::ANF_ConsumableItem()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	SetActorEnableCollision(false);
}

bool ANF_ConsumableItem::TryUse_Implementation(AActor* User)
{
	return false;
}
