#include "UI/Sonar/NF_SilhouetteView.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

bool FNF_SilhouetteStyle::IsValid() const
{
	return FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G) && FMath::IsFinite(Color.B) && FMath::IsFinite(Color.A)
		&& FMath::IsFinite(FillOpacity) && FillOpacity >= 0 && FillOpacity <= 1
		&& FMath::IsFinite(OutlineOpacity) && OutlineOpacity >= 0 && OutlineOpacity <= 1
		&& FMath::IsFinite(OutlineWidth) && OutlineWidth >= 0 && OutlineWidth <= 4
		&& FMath::IsFinite(DepthBias) && DepthBias >= 0;
}

ANF_SilhouetteView::ANF_SilhouetteView()
{
	PrimaryActorTick.bCanEverTick = true;
	// UWorld updates cameras before this group. Cached blends are consumed by LocalPlayer afterward.
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

ANF_SilhouetteView* ANF_SilhouetteView::FindOrCreate(APlayerController* Recipient)
{
	if (!IsValid(Recipient) || !Recipient->IsLocalController() || !Recipient->GetLocalPlayer()
		|| Recipient->GetNetMode() == NM_DedicatedServer) { return nullptr; }
	for (TActorIterator<ANF_SilhouetteView> It(Recipient->GetWorld()); It; ++It)
	{
		if (!It->IsActorBeingDestroyed() && It->GetOwner() == Recipient) { return *It; }
	}
	const int32 LocalIndex = Recipient->GetGameInstance()->GetLocalPlayers().IndexOfByKey(Recipient->GetLocalPlayer());
	// Reserve 240..255 for local silhouette views; each split-screen player gets a separate mask.
	if (LocalIndex < 0 || LocalIndex >= 16) { return nullptr; }
	FActorSpawnParameters Params; Params.Owner = Recipient;
	ANF_SilhouetteView* View = Recipient->GetWorld()->SpawnActor<ANF_SilhouetteView>(Params);
	if (View) { View->StencilValue = 240 + LocalIndex; }
	return View;
}

void ANF_SilhouetteView::UpdateRequest(FGuid SourceId, const TArray<TObjectPtr<ACharacter>>& Targets, double ExpiresAt,
	UMaterialInterface* Material, const FNF_SilhouetteStyle& Style)
{
	if (!SourceId.IsValid() || !Material || !Style.IsValid()) { return; }
	FRequest* Existing = Requests.Find(SourceId);
	FRequest& Request = Existing ? *Existing : Requests.Add(SourceId);
	if (!Existing) { Request.Order = NextOrder++; }
	Request.Targets.Reset();
	for (ACharacter* Target : Targets)
	{
		if (IsValid(Target) && !Target->IsActorBeingDestroyed()) { Request.Targets.AddUnique(Target); }
	}
	Request.ExpiresAt = ExpiresAt; Request.Material = Material; Request.Style = Style;
}

void ANF_SilhouetteView::RemoveRequest(FGuid SourceId)
{
	Requests.Remove(SourceId);
	ReconcileMeshes();
}

void ANF_SilhouetteView::ReconcileMeshes()
{
	TSet<USkeletalMeshComponent*> Wanted;
	for (const auto& Pair : Requests)
	{
		for (const TWeakObjectPtr<ACharacter>& Target : Pair.Value.Targets)
		{
			if (!Target.IsValid() || Target->IsActorBeingDestroyed()) { continue; }
			TInlineComponentArray<USkeletalMeshComponent*> Components; Target->GetComponents(Components);
			for (auto* Component : Components)
			{
				if (Component->IsRegistered() && Component->GetSkeletalMeshAsset()) { Wanted.Add(Component); }
			}
		}
	}
	for (int32 Index = Meshes.Num() - 1; Index >= 0; --Index)
	{
		if (!Meshes[Index].Source.IsValid() || !Wanted.Contains(Meshes[Index].Source.Get()))
		{
			if (Meshes[Index].Proxy) { Meshes[Index].Proxy->DestroyComponent(); }
			Meshes.RemoveAtSwap(Index);
		}
	}
	for (USkeletalMeshComponent* Source : Wanted)
	{
		FNF_SilhouetteMesh* Entry = Meshes.FindByPredicate([Source](const FNF_SilhouetteMesh& Item) { return Item.Source.Get() == Source; });
		if (!Entry)
		{
			FNF_SilhouetteMesh& NewEntry = Meshes.AddDefaulted_GetRef(); NewEntry.Source = Source;
			NewEntry.Proxy = NewObject<USkeletalMeshComponent>(this);
			USkeletalMeshComponent* Proxy = NewEntry.Proxy;
			AddInstanceComponent(Proxy); Proxy->SetupAttachment(RootComponent);
			Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Proxy->SetGenerateOverlapEvents(false); Proxy->SetCastShadow(false);
			Proxy->SetRenderInMainPass(false); Proxy->SetRenderInDepthPass(false);
			Proxy->SetRenderCustomDepth(true); Proxy->SetCustomDepthStencilValue(StencilValue);
			Proxy->SetReceivesDecals(false);
			Proxy->RegisterComponent(); Entry = &NewEntry;
		}
		USkeletalMeshComponent* Proxy = Entry->Proxy;
		if (Proxy->GetSkeletalMeshAsset() != Source->GetSkeletalMeshAsset())
		{
			Proxy->SetSkeletalMeshAsset(Source->GetSkeletalMeshAsset());
			// Opaque proxy materials ensure depth writing even if the original uses translucency.
			for (int32 Slot = 0; Slot < Proxy->GetNumMaterials(); ++Slot) { Proxy->SetMaterial(Slot, UMaterial::GetDefaultMaterial(MD_Surface)); }
		}
		Proxy->SetLeaderPoseComponent(Source);
		Proxy->SetWorldTransform(Source->GetComponentTransform());
	}
}

void ANF_SilhouetteView::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	auto* Recipient = Cast<APlayerController>(GetOwner());
	if (!IsValid(Recipient) || !Recipient->IsLocalController()) { Destroy(); return; }
	const double Now = GetWorld()->GetTimeSeconds();
	for (auto It = Requests.CreateIterator(); It; ++It)
	{
		if (It.Value().ExpiresAt <= Now || !It.Value().Material.IsValid()) { It.RemoveCurrent(); }
	}
	ReconcileMeshes();
	if (Requests.IsEmpty()) { Destroy(); return; }
	const FRequest* Appearance = nullptr;
	for (const auto& Pair : Requests)
	{
		if (!Appearance || Pair.Value.Order > Appearance->Order) { Appearance = &Pair.Value; }
	}
	// Targets are a union; the latest active request controls the shared palette.
	if (!MaterialInstance || MaterialInstance->Parent != Appearance->Material.Get())
	{
		if (MaterialInstance)
		{
			MaterialInstance->SetScalarParameterValue(TEXT("FillOpacity"), 0.0f);
			MaterialInstance->SetScalarParameterValue(TEXT("OutlineOpacity"), 0.0f);
		}
		MaterialInstance = UMaterialInstanceDynamic::Create(Appearance->Material.Get(), this);
	}
	if (!MaterialInstance || !Recipient->PlayerCameraManager) { return; }
	MaterialInstance->SetVectorParameterValue(TEXT("RevealColor"), Appearance->Style.Color);
	MaterialInstance->SetScalarParameterValue(TEXT("FillOpacity"), Appearance->Style.FillOpacity);
	MaterialInstance->SetScalarParameterValue(TEXT("OutlineOpacity"), Appearance->Style.OutlineOpacity);
	MaterialInstance->SetScalarParameterValue(TEXT("OutlineWidth"), Appearance->Style.OutlineWidth);
	MaterialInstance->SetScalarParameterValue(TEXT("DepthBias"), Appearance->Style.DepthBias);
	MaterialInstance->SetScalarParameterValue(TEXT("RevealStencil"), StencilValue);
	FPostProcessSettings Settings;
	Settings.AddBlendable(MaterialInstance, 1.0f);
	const TArray<FPostProcessSettings>* CachedSettings = nullptr;
	const TArray<float>* CachedWeights = nullptr;
	const TArray<EViewTargetBlendOrder>* CachedOrders = nullptr;
	Recipient->PlayerCameraManager->GetCachedPostProcessBlends(CachedSettings, CachedWeights, CachedOrders);
	// Some camera modes skip modifier updates/cache clearing. Never append duplicate entries in those modes.
	if (CachedSettings)
	{
		for (const auto& Cached : *CachedSettings)
		{
			if (Cached.WeightedBlendables.Array.ContainsByPredicate([this](const FWeightedBlendable& Blend)
				{ return Blend.Object == MaterialInstance; })) { return; }
		}
	}
	Recipient->PlayerCameraManager->AddCachedPPBlend(Settings, 1.0f, VTBlendOrder_Override);
}

void ANF_SilhouetteView::ClearMeshes()
{
	for (auto& Mesh : Meshes) { if (Mesh.Proxy) { Mesh.Proxy->DestroyComponent(); } }
	Meshes.Reset();
}

void ANF_SilhouetteView::EndPlay(const EEndPlayReason::Type Reason)
{
	if (MaterialInstance)
	{
		MaterialInstance->SetScalarParameterValue(TEXT("FillOpacity"), 0.0f);
		MaterialInstance->SetScalarParameterValue(TEXT("OutlineOpacity"), 0.0f);
	}
	ClearMeshes(); Requests.Reset(); MaterialInstance = nullptr;
	Super::EndPlay(Reason);
}
