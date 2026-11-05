#include "GamePlay/Combat/GameplayCue/NF_BossLineTelegraph.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"

ANF_BossLineTelegraph::ANF_BossLineTelegraph()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	Decal = CreateDefaultSubobject<UDecalComponent>(TEXT("Warning"));
	SetRootComponent(Decal);
	Decal->SetHiddenInGame(true);
	Decal->FadeScreenSize = 0.f;
}

void ANF_BossLineTelegraph::Initialize(UMaterialInterface* Material, FVector Extent)
{
	WarningMaterial = Material;
	BoxExtent = Extent;
}

void ANF_BossLineTelegraph::BeginPlay()
{
	Super::BeginPlay();
	UpdateDecal();
}

void ANF_BossLineTelegraph::UpdateDecal()
{
	if (!WarningMaterial || BoxExtent.X <= 0 || GetNetMode() == NM_DedicatedServer) return;
	Decal->SetDecalMaterial(WarningMaterial);
	// With pitch -90, decal local Z lies along the strike's world-space forward axis.
	Decal->DecalSize = FVector(BoxExtent.Z, BoxExtent.Y, BoxExtent.X);
	Decal->SetHiddenInGame(false);
	Decal->MarkRenderStateDirty();
}

void ANF_BossLineTelegraph::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANF_BossLineTelegraph, WarningMaterial, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_BossLineTelegraph, BoxExtent, COND_InitialOnly);
}
