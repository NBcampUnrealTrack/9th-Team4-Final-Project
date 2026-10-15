#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewActor.h"

#include "Abilities/GameplayAbility.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "GamePlay/Combat/Components/NF_GroundTargetingComponent.h"
#include "GamePlay/Combat/Components/NF_AreaIndicatorComponent.h"
#include "GamePlay/Combat/Projectiles/NF_ProjectileTrajectoryComponent.h"
#include "Project_NF.h"
#include "GamePlay/Combat/Skill/NF_SkillComposition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

ANF_SkillPreviewActor::ANF_SkillPreviewActor()
{
	bReplicates = false;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PreviewRoot")));
}

bool ANF_SkillPreviewActor::InitializePreview(AActor* InSourceAvatar, UGameplayAbility* InSourceAbility)
{
	const APawn* Pawn = Cast<APawn>(InSourceAvatar);
	if (!Pawn || !Pawn->IsLocallyControlled() || GetNetMode() == NM_DedicatedServer || !IsValid(InSourceAbility))
	{
		return false;
	}
	SourceAvatar = InSourceAvatar;
	SourceAbility = InSourceAbility;
	if (bAutomaticComposition && !ValidateComposition()) { return false; }
	bPreviewInitialized = true;
	SetActorTickEnabled(true);
	PreviewStarted();
	if (!IsValid(this))
	{
		return false;
	}
	if (bAutomaticComposition) { UpdateAutomaticPreview(); }
	else { UpdatePreview(); }
	return true;
}

UGameplayAbility* ANF_SkillPreviewActor::GetSourceAbility() const
{
	return SourceAbility.Get();
}

bool ANF_SkillPreviewActor::GetPreviewData(FNF_SkillPreviewData& OutData) const
{
	OutData = FNF_SkillPreviewData();
	const UNFGA_SkillBase* Skill = Cast<UNFGA_SkillBase>(SourceAbility.Get());
	return Skill && Skill->GetSkillPreviewData(SourceAvatar.Get(), OutData);
}

void ANF_SkillPreviewActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetActorTickEnabled(false);
	if (bPreviewInitialized)
	{
		TArray<UNF_ProjectileTrajectoryComponent*> Paths;
		GetComponents(Paths);
		for (auto* Path : Paths) { Path->StopPreview(); }
		TArray<UNF_AreaIndicatorComponent*> Areas;
		GetComponents(Areas);
		for (auto* Area : Areas) { Area->HideArea(); }
		PreviewStopped();
		bPreviewInitialized = false;
	}
	Super::EndPlay(EndPlayReason);
}

void ANF_SkillPreviewActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const APawn* Pawn = Cast<APawn>(SourceAvatar.Get());
	if (!Pawn || !Pawn->IsLocallyControlled() || !SourceAbility.IsValid())
	{
		Destroy();
		return;
	}
	if (bAutomaticComposition) { UpdateAutomaticPreview(); }
	else { UpdatePreview(); }
}

bool ANF_SkillPreviewActor::ValidateComposition() const
{
	TArray<UNF_GroundTargetingComponent*> Grounds;
	TArray<UNF_ProjectileTrajectoryComponent*> Paths;
	NF_GetCompositionComponents(this, Grounds);
	NF_GetCompositionComponents(this, Paths);
	if (Grounds.Num() + Paths.Num() != 1)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillPreview] %s needs exactly one GroundTargeting or ProjectileTrajectory component."), *GetName());
		return false;
	}
	return true;
}

#if WITH_EDITOR
EDataValidationResult ANF_SkillPreviewActor::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (bAutomaticComposition && !ValidateComposition())
	{
		Context.AddError(FText::FromString(TEXT("Automatic preview requires exactly one GroundTargeting or ProjectileTrajectory component.")));
		return EDataValidationResult::Invalid;
	}
	return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}
#endif

void ANF_SkillPreviewActor::UpdateAutomaticPreview()
{
	// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
	// 계산 단계가 끝난 다음 표시 단계로 전달한다. BP 컴포넌트 배치 순서에 의존하지 않는다.
	FNF_SkillPreviewData Data;
	FHitResult Hit;
	bool bHasTarget = false;
	if (GetPreviewData(Data))
	{
		if (auto* Ground = FindComponentByClass<UNF_GroundTargetingComponent>())
		{
			FVector Direction;
			bHasTarget = Ground->FindGroundTarget(GetSourceAvatar(), Data.TargetRange, Data.GroundSearchDepth, Hit, Direction);
		}
		else if (auto* Path = FindComponentByClass<UNF_ProjectileTrajectoryComponent>())
		{
			Path->StartPreview();
			if (Data.bHasProjectilePath)
			{
				Path->UpdatePreview(Data.LaunchLocation, Data.LaunchVelocity, Data.ProjectileRadius, Data.GravityZ, Data.FlightTime);
				bHasTarget = Path->GetPredictedImpact(Hit);
			}
			else { Path->StopPreview(); }
		}
	}
	else if (auto* Path = FindComponentByClass<UNF_ProjectileTrajectoryComponent>()) { Path->StopPreview(); }
	TArray<UNF_AreaIndicatorComponent*> Areas;
	GetComponents(Areas);
	for (auto* Area : Areas)
	{
		Area->AddTickPrerequisiteActor(this);
		if (bHasTarget) { Area->ShowArea(Hit.ImpactPoint, Data.DisplayRadius, bAlignAreaToSurface ? Hit.ImpactNormal : FVector::UpVector); }
		else { Area->HideArea(); }
	}
}

void ANF_SkillPreviewActor::UpdatePreview_Implementation()
{
	// 스킬별 자식 액터/BP가 부착한 컴포넌트에 표시 데이터를 전달한다.
}
