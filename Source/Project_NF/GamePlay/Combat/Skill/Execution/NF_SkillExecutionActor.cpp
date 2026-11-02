#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GamePlay/Combat/Components/NF_AreaDamageComponent.h"
#include "GamePlay/Combat/Components/NF_AreaIndicatorComponent.h"
#include "GamePlay/Combat/Components/NF_BeamTargetingComponent.h"
#include "GamePlay/Combat/Components/NF_GroundTargetingComponent.h"
#include "GamePlay/Combat/Components/NF_DeployableHealthComponent.h"
#include "GamePlay/Combat/Components/NF_ProjectileLaunchComponent.h"
#include "GamePlay/Combat/Components/NF_GroundTravelComponent.h"
#include "GamePlay/Combat/Components/NF_DetonationComponent.h"
#include "GamePlay/Combat/Components/NF_ApplyEffectComponent.h"
#include "GamePlay/Combat/Components/NF_TargetQueryComponent.h"
#include "GamePlay/Combat/Components/NF_HitEffectComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"
#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Project_NF.h"
#include "GamePlay/Combat/Skill/NF_SkillComposition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

ANF_SkillExecutionActor::ANF_SkillExecutionActor()
{
	bReplicates = true;
	SetReplicateMovement(true);
	PrimaryActorTick.bCanEverTick = true;
	SetNetUpdateFrequency(20.0f);
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ExecutionRoot")));
}

void ANF_SkillExecutionActor::PostInitializeComponents()
{
	if (auto* Deployment = FindComponentByClass<UNF_DeployableComponent>()) { Deployment->SetManagedStart(true); }
	if (auto* Launch = FindComponentByClass<UNF_ProjectileLaunchComponent>()) { Launch->ConfigureComponents(); }
	if (auto* Travel = FindComponentByClass<UNF_GroundTravelComponent>()) { Travel->ConfigureComponents(); }
	Super::PostInitializeComponents();
}

void ANF_SkillExecutionActor::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) { OnRep_Frame(); }
}

void ANF_SkillExecutionActor::InitializeExecution(AActor* Avatar, UNFGA_SkillBase* Ability)
{
	SourceAvatar = Avatar;
	SourceAbility = Ability;
	if (Ability)
	{
		if (const auto* ActiveSkill = Cast<UNFGA_ActiveSkillBase>(Ability)) { ActiveSkill->GetSkillPreviewData(Avatar, PreviewData); }
		const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(Ability->GetConfirmedTargetData());
		bGroundResolved = Target && Target->Kind == ENF_SkillTargetKind::GroundPoint;
		bHasLaunchDirection = Target && Target->Kind == ENF_SkillTargetKind::Direction;
		if (bHasLaunchDirection) { ConfirmedLaunchDirection = Target->AimDirection; }
	}
}

bool ANF_SkillExecutionActor::ValidateComposition() const
{
	TArray<UNF_BeamTargetingComponent*> Beams;
	TArray<UNF_GroundTargetingComponent*> Grounds;
	TArray<UNF_AreaDamageComponent*> Areas;
	TArray<UNF_RepeatApplicationComponent*> Repeats;
	TArray<UNF_DeployableComponent*> Deployments;
	TArray<UNF_DeployableHealthComponent*> Health;
	TArray<UNF_ProjectileLaunchComponent*> Launches;
	TArray<UNF_GroundTravelComponent*> Travels;
	TArray<UNF_DetonationComponent*> Detonations;
	TArray<UProjectileMovementComponent*> Movements;
	TArray<USphereComponent*> Spheres;
	TArray<UNF_ApplyEffectComponent*> ApplyEffects;
	TArray<UNF_TargetQueryComponent*> Queries;
	TArray<UNF_HitEffectComponent*> Hits;
	NF_GetCompositionComponents(this, ApplyEffects); NF_GetCompositionComponents(this, Queries); NF_GetCompositionComponents(this, Hits);
	NF_GetCompositionComponents(this, Launches); NF_GetCompositionComponents(this, Detonations);
	NF_GetCompositionComponents(this, Travels);
	NF_GetCompositionComponents(this, Movements); NF_GetCompositionComponents(this, Spheres);
	NF_GetCompositionComponents(this, Beams); NF_GetCompositionComponents(this, Grounds); NF_GetCompositionComponents(this, Areas);
	NF_GetCompositionComponents(this, Repeats); NF_GetCompositionComponents(this, Deployments); NF_GetCompositionComponents(this, Health);
	TArray<UAbilitySystemComponent*> AbilitySystems;
	NF_GetCompositionComponents(this, AbilitySystems);
	bool bValid = Beams.Num() + Grounds.Num() + Launches.Num() + Travels.Num() <= 1 && Areas.Num() <= 1 && Repeats.Num() <= 1
		&& Deployments.Num() <= 1 && Health.Num() <= 1 && (Repeats.IsEmpty() || !Areas.IsEmpty() || !Queries.IsEmpty()
			|| (ApplyEffects.Num() == 1 && ApplyEffects[0]->TargetMode == ENF_EffectTargetMode::Self))
		&& (Areas.IsEmpty() || !ApplyEffects.IsEmpty() || (DamageEffectClass && DamageMagnitudeTag.IsValid() && FMath::IsFinite(DamageMagnitude) && DamageMagnitude >= 0.0f))
		&& (Health.IsEmpty() || (AbilitySystems.Num() == 1 && !Deployments.IsEmpty()));
	for (auto* Beam : Beams) { bValid &= Beam->HasValidSettings(); }
	for (auto* Repeat : Repeats) { bValid &= Repeat->HasValidSettings(); }
	for (auto* Deployment : Deployments) { bValid &= Deployment->HasValidSettings(); }
	bValid &= Launches.Num() + Travels.Num() <= 1 && Detonations.Num() <= 1 && (Detonations.IsEmpty() || Launches.Num() + Travels.Num() == 1);
	if (!Travels.IsEmpty())
	{
		bValid &= Travels.Num() == 1 && Detonations.Num() == 1 && Spheres.Num() == 1 && Movements.IsEmpty()
			&& Repeats.IsEmpty() && Deployments.IsEmpty() && Hits.IsEmpty() && !bFollowSource;
		for (auto* Travel : Travels) { bValid &= Travel->HasValidSettings(); }
		for (auto* Detonation : Detonations) { bValid &= Detonation->HasValidSettings(); }
		for (auto* Sphere : Spheres) { bValid &= FMath::IsFinite(Sphere->GetScaledSphereRadius()) && Sphere->GetScaledSphereRadius() > 0; }
	}
	bValid &= ApplyEffects.Num() <= 1 && Queries.Num() <= 1 && Hits.Num() <= 1 && (Queries.IsEmpty() || Areas.IsEmpty());
	for (auto* Query : Queries) { bValid &= Query->HasValidSettings(); }
	for (auto* Apply : ApplyEffects)
	{
		bValid &= Apply->HasValidSettings();
		if (Apply->TargetMode == ENF_EffectTargetMode::Self) { bValid &= Areas.IsEmpty() && Queries.IsEmpty() && Hits.IsEmpty() && Launches.IsEmpty() && Travels.IsEmpty(); }
		else if (Apply->TargetMode == ENF_EffectTargetMode::Query) { bValid &= (!Queries.IsEmpty() || !Areas.IsEmpty()) && Hits.IsEmpty(); }
		else if (Apply->TargetMode == ENF_EffectTargetMode::Hit) { bValid &= Hits.Num() == 1 && Queries.IsEmpty() && Areas.IsEmpty() && Repeats.IsEmpty() && Beams.IsEmpty(); }
		else { bValid = false; }
	}
	if (!Hits.IsEmpty())
	{
		TArray<UPrimitiveComponent*> Primitives;
		NF_GetCompositionComponents(this, Primitives);
		bValid &= ApplyEffects.Num() == 1 && ApplyEffects[0]->TargetMode == ENF_EffectTargetMode::Hit && !Primitives.IsEmpty();
		for (auto* Hit : Hits) { bValid &= Hit->HasValidSettings(); }
	}
	if (!Launches.IsEmpty())
	{
		bValid &= Movements.Num() == 1 && Spheres.Num() == 1 && Repeats.IsEmpty() && Deployments.IsEmpty() && !bFollowSource;
		bValid &= Detonations.Num() == 1 || Hits.Num() == 1;
		for (auto* Launch : Launches) { bValid &= Launch->HasValidSettings(); }
		for (auto* Detonation : Detonations) { bValid &= Detonation->HasValidSettings(); }
		for (auto* Movement : Movements) { bValid &= FMath::IsFinite(Movement->ProjectileGravityScale) && Movement->ProjectileGravityScale >= 0.0f
			&& FMath::IsFinite(Movement->MaxSpeed) && Movement->MaxSpeed >= 0.0f; }
		for (auto* Sphere : Spheres) { bValid &= FMath::IsFinite(Sphere->GetScaledSphereRadius()) && Sphere->GetScaledSphereRadius() > 0.0f; }
	}
	if (Beams.IsEmpty()) { for (auto* Area : Areas) { bValid &= Area->HasValidSettings(); } }
	if (HasActorBegunPlay()) { for (auto* Component : Health) { bValid &= Component->GetHealth() > 0.0f; } }
	if (!bValid) { UE_LOG(LogProject_NF, Warning, TEXT("[SkillExecution] Invalid composition %s: check duplicates, effect settings and health/ASC. Projectile: Launch + (Detonation or HitEffect) + Sphere + ProjectileMovement. Ground travel: GroundTravel + Detonation + Sphere, without Launch/ProjectileMovement/Repeat/Deployable/FollowSource."), *GetName()); }
	return bValid;
}

bool ANF_SkillExecutionActor::CanRunIndependently() const
{
	const auto* Repeat = FindComponentByClass<UNF_RepeatApplicationComponent>();
	return !Repeat || Repeat->HasFiniteLifetime() || FindComponentByClass<UNF_DeployableComponent>();
}

bool ANF_SkillExecutionActor::SurvivesCasterEMP() const
{
	return Frame.bActive && CanRunIndependently()
		&& (HasProjectileLaunch() || HasGroundTravel() || (FindComponentByClass<UNF_DeployableComponent>()
			&& !bFollowSource && !FindComponentByClass<UNF_BeamTargetingComponent>()));
}

bool ANF_SkillExecutionActor::PrepareExecution()
{
	if (!HasAuthority() || bStopping || !IsValid(SourceAvatar) || !SourceAbility.IsValid() || !ValidateComposition()) { return false; }
	if (auto* Launch = FindComponentByClass<UNF_ProjectileLaunchComponent>())
	{
		if (!bHasLaunchDirection || !Launch->PrepareLaunch(SourceAvatar, ConfirmedLaunchDirection)) { return false; }
	}
	if (auto* Travel = FindComponentByClass<UNF_GroundTravelComponent>())
	{
		if (!bHasLaunchDirection || !Travel->PrepareTravel(SourceAvatar, ConfirmedLaunchDirection)) { return false; }
	}
	if (!UpdateTarget()) { return false; }
	if (auto* Apply = FindComponentByClass<UNF_ApplyEffectComponent>())
	{
		if (!Apply->PrepareEffects(SourceAvatar, SourceAbility->GetAbilityLevel())) { return false; }
	}
	else if (FindComponentByClass<UNF_AreaDamageComponent>())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceAvatar);
		if (!ASC) { return false; }
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		DamageSpec = ASC->MakeOutgoingSpec(DamageEffectClass, SourceAbility->GetAbilityLevel(), Context);
		if (!DamageSpec.IsValid()) { return false; }
		DamageSpec.Data->SetSetByCallerMagnitude(DamageMagnitudeTag, DamageMagnitude);
	}
	bPrepared = true;
	return true;
}

bool ANF_SkillExecutionActor::StartExecution()
{
	if (!HasAuthority() || bStopping || Frame.bActive || (!bPrepared && !PrepareExecution())) { return false; }
	if (auto* Deployment = FindComponentByClass<UNF_DeployableComponent>())
	{
		// BP-added components only exist after FinishSpawning. Use the actual casting ASC.
		Deployment->SetCasterAbilitySystem(SourceAbility.IsValid() ? SourceAbility->GetAbilitySystemComponentFromActorInfo() : nullptr);
		Deployment->OnDeploymentEnded.AddDynamic(this, &ANF_SkillExecutionActor::HandleDeploymentEnded);
		if (!Deployment->IsDeploymentActive() && !Deployment->StartDeployment()) { return false; }
	}
	Frame.bActive = true;
	const auto* Area = FindComponentByClass<UNF_AreaDamageComponent>();
	const auto* Query = FindComponentByClass<UNF_TargetQueryComponent>();
	Frame.DisplayRadius = Area ? Area->GetDefaultShape().Radius : (Query ? Query->Settings.Shape.Radius : PreviewData.DisplayRadius);
	UpdateTarget();
	bStartedNotified = true;
	ExecutionStarted();
	if (!Frame.bActive || IsActorBeingDestroyed()) { return false; }
	if (auto* Hit = FindComponentByClass<UNF_HitEffectComponent>())
	{
		Hit->OnHitEffectsSubmitted.AddDynamic(this, &ANF_SkillExecutionActor::HandleHitEffectsSubmitted);
		if (!Hit->StartListening(SourceAvatar)) { StopExecution(); return false; }
	}
	if (auto* Launch = FindComponentByClass<UNF_ProjectileLaunchComponent>())
	{
		auto* Detonation = FindComponentByClass<UNF_DetonationComponent>();
		if (Detonation)
		{
			Detonation->OnDetonated.AddDynamic(this, &ANF_SkillExecutionActor::HandleDetonated);
			if (!Detonation->Arm(Cast<UPrimitiveComponent>(GetRootComponent()))) { StopExecution(); return false; }
		}
		if (!Launch->Launch(SourceAvatar)) { StopExecution(); return false; }
		// 명중 전용 투사체가 빗나가도 무한히 남지 않는다.
		if (!Detonation) { SetLifeSpan(10.0f); }
	}
	else if (auto* Travel = FindComponentByClass<UNF_GroundTravelComponent>())
	{
		auto* Detonation = FindComponentByClass<UNF_DetonationComponent>();
		Travel->OnTravelEnded.AddDynamic(this, &ThisClass::HandleGroundTravelEnded);
		Detonation->OnDetonated.AddDynamic(this, &ThisClass::HandleDetonated);
		if (!Detonation->Arm(Cast<UPrimitiveComponent>(GetRootComponent()), false) || !Travel->StartTravel())
		{
			StopExecution(); return false;
		}
	}
	else if (auto* Repeat = FindComponentByClass<UNF_RepeatApplicationComponent>())
	{
		Repeat->OnApplication.AddDynamic(this, &ANF_SkillExecutionActor::ApplyAttack);
		Repeat->OnRepeatStopped.AddDynamic(this, &ANF_SkillExecutionActor::HandleRepeatStopped);
		if (!Repeat->StartRepeating()) { StopExecution(); return false; }
	}
	else if (FindComponentByClass<UNF_AreaDamageComponent>() || FindComponentByClass<UNF_TargetQueryComponent>()
		|| FindComponentByClass<UNF_ApplyEffectComponent>()) { ApplyAttack(1); }
	// 일회성 실행은 설치물/반복 수명이 없으면 짧게 표시한 뒤 제거한다.
	if (!FindComponentByClass<UNF_DeployableComponent>() && !FindComponentByClass<UNF_RepeatApplicationComponent>()
		&& !FindComponentByClass<UNF_DetonationComponent>()
		&& !FindComponentByClass<UNF_HitEffectComponent>()
		&& SourceAbility.IsValid() && SourceAbility->GetSkillExecutionMode() == ENF_SkillExecutionMode::Instant) { SetLifeSpan(0.5f); }
	ForceNetUpdate();
	UpdateVisuals();
	return true;
}

bool ANF_SkillExecutionActor::UpdateTarget()
{
	if (FindComponentByClass<UNF_ProjectileLaunchComponent>() || FindComponentByClass<UNF_GroundTravelComponent>())
	{
		Frame.Start = GetActorLocation(); Frame.End = Frame.Start; Frame.bHasTarget = true; return true;
	}
	if (!IsValid(SourceAvatar)) { Frame.bHasTarget = false; return false; }
	if (auto* Beam = FindComponentByClass<UNF_BeamTargetingComponent>())
	{
		Frame.bHasTarget = Beam->CalculateBeam(SourceAvatar, Frame.Start, Frame.End);
	}
	else
	{
		if (auto* Ground = FindComponentByClass<UNF_GroundTargetingComponent>())
		{
			if (!bGroundResolved || bFollowSource)
			{
				FHitResult Hit;
				FVector Direction;
				if (!Ground->FindGroundTarget(SourceAvatar, PreviewData.TargetRange, PreviewData.GroundSearchDepth, Hit, Direction))
				{
					Frame.bHasTarget = false; return false;
				}
				SetActorLocationAndRotation(Hit.ImpactPoint, Direction.Rotation());
				bGroundResolved = true;
			}
		}
		else if (bFollowSource) { SetActorTransform(SourceAvatar->GetActorTransform()); }
		Frame.Start = GetActorLocation();
		Frame.End = Frame.Start;
		Frame.bHasTarget = true;
	}
	return Frame.bHasTarget;
}

void ANF_SkillExecutionActor::ApplyAttack(int32 Index)
{
	if (!HasAuthority() || !Frame.bActive || bStopping || !UpdateTarget()) { return; }
	auto* Apply = FindComponentByClass<UNF_ApplyEffectComponent>();
	auto* Query = FindComponentByClass<UNF_TargetQueryComponent>();
	auto* Area = FindComponentByClass<UNF_AreaDamageComponent>();
	if (Apply && Apply->TargetMode == ENF_EffectTargetMode::Hit) { return; }
	if (Apply && Apply->TargetMode == ENF_EffectTargetMode::Self) { Apply->ApplyToSelf(); return; }
	if (!Query && !Area) { return; }
	FTransform Transform = GetActorTransform();
	FNF_AreaDamageShape Shape = Query ? Query->Settings.Shape : Area->GetDefaultShape();
	if (auto* Beam = FindComponentByClass<UNF_BeamTargetingComponent>())
	{
		const FVector DamageEnd = Frame.End + (Frame.End - Frame.Start).GetSafeNormal();
		const FVector Delta = DamageEnd - Frame.Start;
		Shape.Shape = ENF_AreaDamageShape::Box;
		Shape.BoxHalfExtent = FVector(Delta.Size() * 0.5f, Beam->GetHalfWidth(), Beam->GetHalfWidth());
		Transform = FTransform(Delta.Rotation(), (Frame.Start + DamageEnd) * 0.5f);
	}
	const FNF_AreaDamageTargetFilter Filter;
	if (Apply || Query)
	{
		TArray<FNF_TargetQueryResult> Targets;
		if (Query) { Query->FindTargetsWithShape(Transform, Shape, SourceAvatar, Targets); }
		else { Area->FindAreaTargets(Transform, Shape, SourceAvatar, Filter, Targets); }
		// 탐색 이벤트에서 스킬을 종료한 경우 후속 GE를 적용하지 않는다.
		if (Frame.bActive && !bStopping && Apply) { Apply->ApplyToTargets(Targets, Transform.GetLocation()); }
	}
	else { Area->SubmitAreaDamage(Transform, Shape, DamageSpec, SourceAvatar, Filter); }
}

void ANF_SkillExecutionActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority() && Frame.bActive)
	{
		// 이미 발사된 투사체는 시전자 제거 이후에도 자신의 폭발 시점까지 유지한다.
		if (!FindComponentByClass<UNF_ProjectileLaunchComponent>() && !FindComponentByClass<UNF_GroundTravelComponent>()
			&& (!IsValid(SourceAvatar) || SourceAvatar->IsActorBeingDestroyed())) { StopExecution(); return; }
		UpdateTarget();
	}
	UpdateVisuals();
}

void ANF_SkillExecutionActor::UpdateVisuals()
{
	if (GetNetMode() == NM_DedicatedServer) { return; }
	if (auto* Beam = FindComponentByClass<UNF_BeamTargetingComponent>())
	{
		if (Frame.bActive && Frame.bHasTarget) { Beam->ShowBeam(Frame.Start, Frame.End); }
		else { Beam->HideBeam(); }
	}
	TArray<UNF_AreaIndicatorComponent*> Indicators;
	GetComponents(Indicators);
	for (auto* Area : Indicators)
	{
		if (Frame.bActive && Frame.bHasTarget) { Area->ShowArea(Frame.End, Frame.DisplayRadius, FVector::UpVector); }
		else { Area->HideArea(); }
	}
}

#if WITH_EDITOR
EDataValidationResult ANF_SkillExecutionActor::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (!ValidateComposition())
	{
		Context.AddError(FText::FromString(TEXT("Invalid skill composition: check component duplicates, ApplyEffect target/effect settings and health/ASC. Projectile: Launch + (Detonation or HitEffect) + Sphere + ProjectileMovement. Ground travel: GroundTravel + Detonation + Sphere, without Launch/ProjectileMovement/Repeat/Deployable/FollowSource.")));
		return EDataValidationResult::Invalid;
	}
	return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}
#endif

void ANF_SkillExecutionActor::StopExecution()
{
	if (!HasAuthority() || bStopping) { return; }
	bStopping = true;
	Frame.bActive = false;
	if (auto* Hit = FindComponentByClass<UNF_HitEffectComponent>()) { Hit->StopListening(); }
	if (auto* Apply = FindComponentByClass<UNF_ApplyEffectComponent>()) { Apply->StopEffects(); }
	if (auto* Detonation = FindComponentByClass<UNF_DetonationComponent>()) { Detonation->Disarm(); }
	if (auto* Launch = FindComponentByClass<UNF_ProjectileLaunchComponent>()) { Launch->StopProjectile(); }
	if (auto* Travel = FindComponentByClass<UNF_GroundTravelComponent>()) { Travel->StopTravel(); }
	if (auto* Repeat = FindComponentByClass<UNF_RepeatApplicationComponent>()) { Repeat->StopRepeating(); }
	if (auto* Deployment = FindComponentByClass<UNF_DeployableComponent>()) { Deployment->EndDeployment(); }
	ForceNetUpdate();
	UpdateVisuals();
	ExecutionStopped();
	if (!bEndedNotified) { bEndedNotified = true; OnExecutionEnded.Broadcast(); }
	if (!IsActorBeingDestroyed() && !FindComponentByClass<UNF_DeployableComponent>()) { SetLifeSpan(0.5f); }
}

void ANF_SkillExecutionActor::HandleRepeatStopped(ENF_RepeatStopReason Reason) { StopExecution(); }
void ANF_SkillExecutionActor::HandleHitEffectsSubmitted(AActor* Target, int32 EffectCount)
{
	if (auto* Hit = FindComponentByClass<UNF_HitEffectComponent>())
	{
		if (Hit->bEndExecutionAfterHit && EffectCount > 0) { StopExecution(); }
	}
}
void ANF_SkillExecutionActor::HandleDeploymentEnded(ENF_DeploymentEndReason Reason) { StopExecution(); }
void ANF_SkillExecutionActor::OnRep_Frame()
{
	if (!HasActorBegunPlay()) { return; }
	UpdateVisuals();
	if (Frame.bActive && !bStartedNotified) { bStartedNotified = true; ExecutionStarted(); }
	if (!Frame.bActive && bStartedNotified && !bEndedNotified) { bEndedNotified = true; ExecutionStopped(); }
}
void ANF_SkillExecutionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANF_SkillExecutionActor, SourceAvatar);
	DOREPLIFETIME(ANF_SkillExecutionActor, Frame);
}
void ANF_SkillExecutionActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (HasAuthority()) { StopExecution(); }
	else if (!bEndedNotified)
	{
		bEndedNotified = true;
		Frame.bActive = false;
		UpdateVisuals();
		ExecutionStopped();
	}
	Super::EndPlay(Reason);
}

bool ANF_SkillExecutionActor::HasProjectileLaunch() const
{
	TArray<UNF_ProjectileLaunchComponent*> Launches;
	NF_GetCompositionComponents(this, Launches);
	return !Launches.IsEmpty();
}

bool ANF_SkillExecutionActor::HasGroundTravel() const
{
	TArray<UNF_GroundTravelComponent*> Travels;
	NF_GetCompositionComponents(this, Travels);
	return !Travels.IsEmpty();
}

bool ANF_SkillExecutionActor::GetProjectilePreviewData(AActor* Avatar, FNF_SkillPreviewData& Data) const
{
	if (HasGroundTravel())
	{
		// Direction input is shared with thrown skills; a ballistic trajectory is not appropriate here.
		Data = PreviewData;
		Data.bHasProjectilePath = false;
		return IsValid(Avatar);
	}
	TArray<UNF_ProjectileLaunchComponent*> Launches;
	TArray<UNF_DetonationComponent*> Detonations;
	TArray<UProjectileMovementComponent*> Movements;
	TArray<USphereComponent*> Spheres;
	TArray<UNF_AreaDamageComponent*> Areas;
	NF_GetCompositionComponents(this, Launches); NF_GetCompositionComponents(this, Detonations);
	NF_GetCompositionComponents(this, Movements); NF_GetCompositionComponents(this, Spheres); NF_GetCompositionComponents(this, Areas);
	TArray<UNF_TargetQueryComponent*> Queries;
	NF_GetCompositionComponents(this, Queries);
	TArray<UNF_HitEffectComponent*> Hits;
	NF_GetCompositionComponents(this, Hits);
	if (!IsValid(Avatar) || !Avatar->GetWorld() || Launches.Num() != 1 || Detonations.Num() > 1
		|| (Detonations.IsEmpty() && Hits.Num() != 1) || Movements.Num() != 1 || Spheres.Num() != 1) { return false; }
	FVector Eyes;
	FRotator Rotation;
	Avatar->GetActorEyesViewPoint(Eyes, Rotation);
	Data = FNF_SkillPreviewData();
	Data.ProjectileRadius = HasAnyFlags(RF_ClassDefaultObject)
		? Spheres[0]->GetUnscaledSphereRadius() * Spheres[0]->GetRelativeScale3D().GetAbsMin()
		: Spheres[0]->GetScaledSphereRadius();
	if ((!Detonations.IsEmpty() && !Detonations[0]->HasValidSettings()) || !Launches[0]->CalculateLaunch(Avatar, Rotation.Vector(), Data.ProjectileRadius, Movements[0]->MaxSpeed,
		Data.LaunchLocation, Data.LaunchVelocity)) { return false; }
	Data.bHasProjectilePath = true;
	Data.FlightTime = Detonations.IsEmpty() ? 10.0f : Detonations[0]->GetPreviewTime();
	Data.GravityZ = Avatar->GetWorld()->GetGravityZ() * Movements[0]->ProjectileGravityScale;
	Data.DisplayRadius = Areas.Num() == 1 ? Areas[0]->GetDefaultShape().Radius : (Queries.Num() == 1 ? Queries[0]->Settings.Shape.Radius : 0.0f);
	return true;
}

bool ANF_SkillExecutionActor::ValidateProjectileDirection(AActor* Avatar, FVector Direction) const
{
	if (HasGroundTravel())
	{
		TArray<UNF_GroundTravelComponent*> Travels; TArray<USphereComponent*> CollisionSpheres;
		NF_GetCompositionComponents(this, Travels); NF_GetCompositionComponents(this, CollisionSpheres);
		if (Travels.Num() != 1 || CollisionSpheres.Num() != 1) { return false; }
		const float Radius = HasAnyFlags(RF_ClassDefaultObject)
			? CollisionSpheres[0]->GetUnscaledSphereRadius() * CollisionSpheres[0]->GetRelativeScale3D().GetAbsMin()
			: CollisionSpheres[0]->GetScaledSphereRadius();
		FVector Location, Heading;
		return Travels[0]->CalculateStart(Avatar, Direction, Radius, Location, Heading);
	}
	TArray<UNF_ProjectileLaunchComponent*> Launches;
	TArray<UProjectileMovementComponent*> Movements;
	TArray<USphereComponent*> Spheres;
	NF_GetCompositionComponents(this, Launches); NF_GetCompositionComponents(this, Movements); NF_GetCompositionComponents(this, Spheres);
	if (Launches.Num() != 1 || Movements.Num() != 1 || Spheres.Num() != 1) { return false; }
	FVector Location;
	FVector Velocity;
	const float Radius = HasAnyFlags(RF_ClassDefaultObject)
		? Spheres[0]->GetUnscaledSphereRadius() * Spheres[0]->GetRelativeScale3D().GetAbsMin()
		: Spheres[0]->GetScaledSphereRadius();
	return Launches[0]->CalculateLaunch(Avatar, Direction, Radius, Movements[0]->MaxSpeed, Location, Velocity);
}

void ANF_SkillExecutionActor::HandleDetonated()
{
	if (!HasAuthority() || !Frame.bActive || bStopping) { return; }
	const FVector Location = GetActorLocation();
	ApplyAttack(1);
	MulticastDetonation(Location, Frame.DisplayRadius);
	StopExecution();
}

void ANF_SkillExecutionActor::HandleGroundTravelEnded()
{
	if (auto* Detonation = FindComponentByClass<UNF_DetonationComponent>()) { Detonation->Detonate(); }
}

void ANF_SkillExecutionActor::MulticastDetonation_Implementation(FVector Location, float Radius)
{
	TArray<UStaticMeshComponent*> Meshes;
	GetComponents(Meshes);
	for (auto* Mesh : Meshes) { Mesh->SetVisibility(false, true); }
	if (auto* Detonation = FindComponentByClass<UNF_DetonationComponent>()) { Detonation->PlayCosmetic(Location, Radius); }
	if (GetNetMode() != NM_DedicatedServer) { DetonationCosmetic(Location, Radius); }
}
