#include "GamePlay/Combat/GameplayCue/NF_BossWarningMarker.h"

#include "Components/TextRenderComponent.h"

ANF_BossWarningMarker::ANF_BossWarningMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);
	SetActorEnableCollision(false);

	WarningText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("WarningText"));
	SetRootComponent(WarningText);
	WarningText->SetText(FText::FromString(TEXT("!")));
	WarningText->SetTextRenderColor(FColor::Red);
	WarningText->SetWorldSize(90.0f);
	WarningText->SetHorizontalAlignment(EHTA_Center);
	WarningText->SetVerticalAlignment(EVRTA_TextCenter);
	WarningText->SetUsingAbsoluteRotation(true);
}
