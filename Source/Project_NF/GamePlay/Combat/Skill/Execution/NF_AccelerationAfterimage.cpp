#include "GamePlay/Combat/Skill/Execution/NF_AccelerationAfterimage.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

bool FNF_AccelerationAfterimageSettings::IsValid() const
{
	return ::IsValid(Material) && FMath::IsFinite(SpawnInterval) && SpawnInterval >= 0.02f
		&& FMath::IsFinite(Lifetime) && Lifetime > 0.0f && MaxAfterimages > 0 && MaxAfterimages <= 32
		&& FMath::IsFinite(MinTravelDistance) && MinTravelDistance >= 0.0f
		&& !OpacityParameter.IsNone() && !ColorParameter.IsNone()
		&& FMath::IsFinite(Color.R) && FMath::IsFinite(Color.G) && FMath::IsFinite(Color.B) && FMath::IsFinite(Color.A);
}

ANF_AccelerationAfterimage::ANF_AccelerationAfterimage()
{
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ANF_AccelerationAfterimage::Initialize(ACharacter* Character, const FNF_AccelerationAfterimageSettings& InSettings)
{
	if (!HasAuthority() || bActive || !IsValid(Character) || !InSettings.IsValid()) { return; }
	Target = Character;
	Settings = InSettings;
	bActive = true;
	ForceNetUpdate();
}

void ANF_AccelerationAfterimage::StopAfterimages()
{
	if (!HasAuthority() || !bActive) { return; }
	bActive = false;
	ForceNetUpdate();
	// 소멸 상태가 전달되고 마지막 잔상이 사라질 시간을 확보한다.
	SetLifeSpan(Settings.Lifetime + 1.0f);
}

USkeletalMeshComponent* ANF_AccelerationAfterimage::ResolveSourceMesh()
{
	if (SourceMesh.IsValid()) { return SourceMesh.Get(); }
	if (!IsValid(Target) || Target->IsActorBeingDestroyed()) { return nullptr; }
	TArray<USkeletalMeshComponent*> Meshes;
	Target->GetComponents(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetFName() == Settings.SourceMeshName) { SourceMesh = Mesh; break; }
	}
	if (!SourceMesh.IsValid()) { SourceMesh = Target->GetMesh(); }
	if (SourceMesh.IsValid()) { AddTickPrerequisiteComponent(SourceMesh.Get()); }
	return SourceMesh.Get();
}

void ANF_AccelerationAfterimage::SpawnSnapshot(USkeletalMeshComponent* Source, double Now)
{
	if (Slots.Num() < Settings.MaxAfterimages)
	{
		FNF_AccelerationAfterimageSlot& Slot = Slots.AddDefaulted_GetRef();
		Slot.Mesh = NewObject<UPoseableMeshComponent>(this);
		Slot.Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Slot.Mesh->SetGenerateOverlapEvents(false);
		Slot.Mesh->SetCastShadow(false);
		Slot.Mesh->SetReceivesDecals(false);
		Slot.Mesh->SetCanEverAffectNavigation(false);
		Slot.Mesh->SetSkinnedAssetAndUpdate(Source->GetSkinnedAsset());
		Slot.Mesh->SetVisibility(false);
		Slot.Mesh->SetComponentTickEnabled(false);
		Slot.Mesh->RegisterComponent();
		Slot.Material = UMaterialInstanceDynamic::Create(Settings.Material, Slot.Mesh);
	}
	FNF_AccelerationAfterimageSlot& Slot = Slots[NextSlot];
	NextSlot = (NextSlot + 1) % Settings.MaxAfterimages;
	if (Slot.Mesh->GetSkinnedAsset() != Source->GetSkinnedAsset())
	{
		Slot.Mesh->SetSkinnedAssetAndUpdate(Source->GetSkinnedAsset());
	}
	if (!IsValid(Slot.Material)) { return; }
	// 캐릭터에 부착하지 않고 생성 순간의 월드 위치와 포즈를 유지한다.
	Slot.Mesh->SetWorldTransform(Source->GetComponentTransform());
	USkeletalMeshComponent* PoseSource = Source;
	// 의상이 다른 메시의 본 포즈를 공유하는 구성도 처리한다.
	if (USkeletalMeshComponent* Leader = Cast<USkeletalMeshComponent>(Source->LeaderPoseComponent.Get())) { PoseSource = Leader; }
	if (PoseSource->GetBoneSpaceTransforms().IsEmpty()) { return; }
	Slot.Mesh->CopyPoseFromSkeletalComponent(PoseSource);
	Slot.Mesh->RefreshBoneTransforms();
	for (int32 Index = 0; Index < Slot.Mesh->GetNumMaterials(); ++Index) { Slot.Mesh->SetMaterial(Index, Slot.Material); }
	Slot.Material->SetVectorParameterValue(Settings.ColorParameter, Settings.Color);
	Slot.Material->SetScalarParameterValue(Settings.OpacityParameter, 1.0f);
	Slot.Mesh->SetVisibility(true);
	Slot.Mesh->SetComponentTickEnabled(false);
	Slot.BornAt = Now;
	LastSpawnTime = Now;
	LastSpawnLocation = Source->GetComponentLocation();
}

void ANF_AccelerationAfterimage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && bActive && (!IsValid(Target) || Target->IsActorBeingDestroyed())) { StopAfterimages(); }
	if (GetNetMode() == NM_DedicatedServer || !GetWorld() || !Settings.IsValid()) { return; }
	const double Now = GetWorld()->GetTimeSeconds();
	for (FNF_AccelerationAfterimageSlot& Slot : Slots)
	{
		if (Slot.BornAt < 0.0) { continue; }
		const float Opacity = FMath::Clamp(1.0f - static_cast<float>((Now - Slot.BornAt) / Settings.Lifetime), 0.0f, 1.0f);
		Slot.Material->SetScalarParameterValue(Settings.OpacityParameter, Opacity);
		if (Opacity <= 0.0f) { Slot.Mesh->SetVisibility(false); Slot.BornAt = -1.0; }
	}
	if (!bActive || !IsValid(Target) || Target->IsActorBeingDestroyed()) { return; }
	USkeletalMeshComponent* Source = ResolveSourceMesh();
	if (!IsValid(Source) || !Source->GetSkinnedAsset() || !Source->IsVisible()) { return; }
	if (LastSpawnTime >= 0.0 && (Now - LastSpawnTime < Settings.SpawnInterval
		|| FVector::DistSquared(Source->GetComponentLocation(), LastSpawnLocation) < FMath::Square(Settings.MinTravelDistance))) { return; }
	SpawnSnapshot(Source, Now);
}

void ANF_AccelerationAfterimage::EndPlay(const EEndPlayReason::Type Reason)
{
	for (FNF_AccelerationAfterimageSlot& Slot : Slots)
	{
		if (IsValid(Slot.Mesh)) { Slot.Mesh->DestroyComponent(); }
	}
	Slots.Empty();
	Super::EndPlay(Reason);
}

void ANF_AccelerationAfterimage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_AccelerationAfterimage, Target);
	DOREPLIFETIME(ANF_AccelerationAfterimage, Settings);
	DOREPLIFETIME(ANF_AccelerationAfterimage, bActive);
}
