#include "GamePlay/Combat/Skill/Active/NFGA_ActiveSkillBase.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "GAS/Tags/NFTag_Skill.h"
#include "TimerManager.h"
#include "Project_NF.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewActor.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillExecutionActor.h"
#include "GamePlay/Combat/Components/NF_GroundTargetingComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"

UNFGA_ActiveSkillBase::UNFGA_ActiveSkillBase()
{
	InitializeNativeSkillDefaults(CreateDefaultSubobject<UNF_ActiveSkillDataAsset>(TEXT("NativeActiveSkillDefaults"), true));
}

bool UNFGA_ActiveSkillBase::BeginLocalPreview()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsLocallyControlled() || !GetActiveSettings().PreviewActorClass
		|| IsActive() || !CanActivateAbility(CurrentSpecHandle, ActorInfo))
	{
		return false;
	}
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar) || !Avatar->GetWorld())
	{
		return false;
	}
	EndLocalPreview();
	FActorSpawnParameters Params;
	Params.Owner = Avatar;
	Params.Instigator = Cast<APawn>(Avatar);
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	LocalPreviewActor = Avatar->GetWorld()->SpawnActor<ANF_SkillPreviewActor>(GetActiveSettings().PreviewActorClass, FTransform::Identity, Params);
	if (!IsValid(LocalPreviewActor) || !LocalPreviewActor->InitializePreview(Avatar, this))
	{
		EndLocalPreview();
		return false;
	}
	return true;
}

void UNFGA_ActiveSkillBase::CommitExecute(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::CommitExecute(Handle, ActorInfo, ActivationInfo);
	// All native active skills commit at effect execution, after targeting/cast delay checks.
	// Do not wait for the animation or bind animation completion to EndAbility.
	if (IsActive() && GetSkillType() == ENF_SkillType::Active) { PlayCastMontage(ActorInfo); }
}

void UNFGA_ActiveSkillBase::PlayCastMontage(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!GetActiveSettings().CastMontage || bCastMontageRequested || !ActorInfo || !ActorInfo->IsNetAuthority()) { return; }
	bCastMontageRequested = true;
	AActor* Avatar = ActorInfo->AvatarActor.Get();
	// Dedicated servers need not initialize an AnimInstance to relay a cosmetic to clients.
	USkeletalMeshComponent* Mesh = ActorInfo->SkeletalMeshComponent.Get();
	if (!IsValid(Avatar) || !Avatar->GetWorld() || !IsValid(Mesh))
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillAnimation] 시전 모션 생략: ASC의 애니메이션 메시가 없습니다. GA=%s"), *GetName());
		return;
	}
	ANF_SkillCastVisual::SpawnForCast(Avatar, Mesh->GetFName(), GetActiveSettings().CastMontage, GetActiveSettings().CastMontagePlayRate,
		GetActiveSettings().CastMontageStartSection, ActorInfo->AffectedAnimInstanceTag);
}

void UNFGA_ActiveSkillBase::EndLocalPreview()
{
	if (IsValid(LocalPreviewActor))
	{
		LocalPreviewActor->Destroy();
	}
	LocalPreviewActor = nullptr;
}

bool UNFGA_ActiveSkillBase::GetSkillPreviewData_Implementation(AActor* SourceAvatar, FNF_SkillPreviewData& OutData) const
{
	if (UsesProjectileExecution()) { return GetActiveSettings().ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->GetProjectilePreviewData(SourceAvatar, OutData); }
	OutData = GetActiveSettings().PreviewData;
	return IsValid(SourceAvatar);
}

void UNFGA_ActiveSkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CanContinueSkillExecution() || ((GetActiveSettings().bRequiresConfirmedTargetData || UsesProjectileExecution())
		&& (!ActorInfo || !ActorInfo->IsNetAuthority()
			|| !UNFGA_ActiveSkillBase::ValidateConfirmedTargetData_Implementation(ConfirmedTargetData)
			|| !ValidateConfirmedTargetData(ConfirmedTargetData))))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (GetActiveSettings().ExecutionActorClass)
	{
		if (!ActorInfo || !ActorInfo->IsNetAuthority()) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
		if (GetCastTime() > 0.0f) { GetWorld()->GetTimerManager().SetTimer(AutomaticCastTimer, this, &UNFGA_ActiveSkillBase::StartAutomaticExecution, GetCastTime(), false); }
		else { StartAutomaticExecution(); }
		return;
	}
	// 실행 클래스 미지정 GA의 기존 BP 실행은 유지한다.
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UNFGA_ActiveSkillBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0)
	{
		Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
		return;
	}
	bCastMontageRequested = false;
	RuntimeExecutionDuration = -1.f;
	CancelLocalInput();
	ClearConfirmedTargetData();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutomaticCastTimer);
		GetWorld()->GetTimerManager().ClearTimer(AutomaticEndTimer);
	}
	ANF_SkillExecutionActor* Actor = ActiveExecutionActor;
	ActiveExecutionActor = nullptr;
	if (IsValid(Actor))
	{
		Actor->OnExecutionEnded.RemoveAll(this);
		// Also protect launched/deployed results from EMP when their GA was configured as Timed.
		if (!(bWasCancelled && IsBlockedByEMP() && Actor->SurvivesCasterEMP())) { Actor->StopExecution(); }
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UNFGA_ActiveSkillBase::BeginLocalInput()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Info || !Info->IsLocallyControlled() || !ASC || !IsValid(Avatar) || !Avatar->GetWorld()
		|| GetSkillType() != ENF_SkillType::Active || IsActive() || !CanContinueSkillExecution()
		|| !CanActivateAbility(CurrentSpecHandle, Info))
	{
		return false;
	}
	TArray<TWeakObjectPtr<UNFGA_SkillBase>> HoldingSkills;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (UNFGA_SkillBase* Other = Cast<UNFGA_SkillBase>(Spec.GetPrimaryInstance()))
		{
			HoldingSkills.Add(Other);
		}
	}
	// BP 취소 이벤트가 GA 목록을 수정할 수 있으므로 목록 순회가 끝난 뒤 호출한다.
	for (const TWeakObjectPtr<UNFGA_SkillBase>& Other : HoldingSkills)
	{
		if (Other.IsValid()) { Other->CancelLocalInput(); }
	}
	const FGameplayAbilitySpec* CurrentSpec = ASC->FindAbilitySpecFromHandle(CurrentSpecHandle);
	if (!CurrentSpec || CurrentSpec->GetPrimaryInstance() != this || !Info->IsLocallyControlled()
		|| GetAvatarActorFromActorInfo() != Avatar || IsActive() || !CanContinueSkillExecution()
		|| !CanActivateAbility(CurrentSpecHandle, Info)) { return false; }
	bLocalInputHolding = true;
	Avatar->GetWorld()->GetTimerManager().SetTimer(LocalInputTimer, this,
		&UNFGA_ActiveSkillBase::CheckLocalInputValidity, 0.05f, true);
	if (GetActiveSettings().PreviewActorClass)
	{
		BeginLocalPreview(); // 표시 실패는 입력 자체를 막지 않는다.
	}
	return bLocalInputHolding;
}

bool UNFGA_ActiveSkillBase::ConfirmLocalInput(FGameplayAbilityTargetDataHandle& OutTargetData)
{
	OutTargetData.Clear();
	CheckLocalInputValidity();
	if (!bLocalInputHolding)
	{
		return false;
	}
	const bool bValid = BuildConfirmedTargetData(OutTargetData);
	// 정상 확정은 취소 이벤트를 발생시키지 않는다.
	bLocalInputHolding = false;
	CancelLocalInput();
	return bValid;
}

void UNFGA_ActiveSkillBase::CancelLocalInput()
{
	const bool bWasHolding = bLocalInputHolding;
	bLocalInputHolding = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LocalInputTimer);
	}
	EndLocalPreview();
	if (bWasHolding)
	{
		LocalInputCanceled();
	}
}

void UNFGA_ActiveSkillBase::CheckLocalInputValidity()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	if (bLocalInputHolding && (!Info || !Info->IsLocallyControlled()
		|| !IsValid(GetAvatarActorFromActorInfo()) || IsActive() || !CanContinueSkillExecution()
		|| !CanActivateAbility(CurrentSpecHandle, Info)))
	{
		CancelLocalInput();
	}
}





bool UNFGA_ActiveSkillBase::BuildConfirmedTargetData_Implementation(FGameplayAbilityTargetDataHandle& OutTargetData) const
{
	OutTargetData.Clear();
	if (UsesProjectileExecution())
	{
		AActor* Avatar = GetAvatarActorFromActorInfo();
		if (!IsValid(Avatar)) { return false; }
		FVector Eyes;
		FRotator Aim;
		Avatar->GetActorEyesViewPoint(Eyes, Aim);
		OutTargetData = MakeSkillTargetData(ENF_SkillTargetKind::Direction, FVector::ZeroVector, Aim.Vector());
		return true;
	}
	if (GetActiveSettings().bRequiresConfirmedTargetData && GetActiveSettings().ExecutionActorClass)
	{
		FNF_SkillPreviewData Data;
		FHitResult Hit;
		FVector Direction;
		if (!GetSkillPreviewData(GetAvatarActorFromActorInfo(), Data)
			|| !UNF_GroundTargetingComponent::CalculateGroundTarget(GetAvatarActorFromActorInfo(), Data.TargetRange, Data.GroundSearchDepth, Hit, Direction)) { return false; }
		OutTargetData = MakeSkillTargetData(ENF_SkillTargetKind::GroundPoint, Hit.ImpactPoint, Direction);
		return true;
	}
	return !GetActiveSettings().bRequiresConfirmedTargetData;
}

bool UNFGA_ActiveSkillBase::ValidateConfirmedTargetData_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) const
{
	if (TargetData.Num() == 0) { return !GetActiveSettings().bRequiresConfirmedTargetData && !UsesProjectileExecution(); }
	const FNF_SkillTargetData* Data = FNF_SkillTargetData::Get(TargetData);
	if (!Data || Data->Location.ContainsNaN() || Data->AimDirection.ContainsNaN()
		|| !FMath::IsNearlyEqual(Data->AimDirection.SizeSquared(), 1.0, 0.02)) { return false; }
	if (UsesProjectileExecution())
	{
		return Data->Kind == ENF_SkillTargetKind::Direction
			&& GetActiveSettings().ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->ValidateProjectileDirection(GetAvatarActorFromActorInfo(), Data->AimDirection);
	}
	if (GetActiveSettings().ExecutionActorClass && GetActiveSettings().bRequiresConfirmedTargetData)
	{
		FNF_SkillPreviewData Preview;
		if (!Data || Data->Kind != ENF_SkillTargetKind::GroundPoint || !GetSkillPreviewData(GetAvatarActorFromActorInfo(), Preview)
			|| !UNF_GroundTargetingComponent::ValidateGroundTarget(GetAvatarActorFromActorInfo(), Data->Location, Preview.TargetRange, Preview.GroundSearchDepth)) { return false; }
	}
	return Data && (Data->Kind == ENF_SkillTargetKind::Direction || Data->Kind == ENF_SkillTargetKind::GroundPoint)
		&& !Data->Location.ContainsNaN() && !Data->AimDirection.ContainsNaN()
		&& FMath::IsNearlyEqual(Data->AimDirection.SizeSquared(), 1.0, 0.02);
}

bool UNFGA_ActiveSkillBase::ShouldEndOnInputRelease() const
{
	return GetActiveSettings().ExecutionActorClass && GetSkillExecutionMode() == ENF_SkillExecutionMode::Continuous && !ShouldActivateOnInputRelease();
}

void UNFGA_ActiveSkillBase::ReleaseSkillInput()
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	if (Info && Info->IsNetAuthority() && ShouldEndOnInputRelease() && IsActive())
	{
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, false);
	}
}

void UNFGA_ActiveSkillBase::StartAutomaticExecution()
{
	if (!IsActive()) { return; }
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Info || !Info->IsNetAuthority() || !IsValid(Avatar) || !GetActiveSettings().ExecutionActorClass || !CanContinueSkillExecution()
		|| !FMath::IsFinite(GetExecutionDuration()) || (GetSkillExecutionMode() == ENF_SkillExecutionMode::Timed && GetExecutionDuration() <= 0.0f)
		|| ((GetActiveSettings().bRequiresConfirmedTargetData || UsesProjectileExecution()) && !ValidateConfirmedTargetData(ConfirmedTargetData)))
	{
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); return;
	}
	FTransform Transform = Avatar->GetActorTransform();
	if (const FNF_SkillTargetData* Target = FNF_SkillTargetData::Get(ConfirmedTargetData))
	{
		if (Target->Kind == ENF_SkillTargetKind::GroundPoint) { Transform = FTransform(FVector(Target->AimDirection).Rotation(), Target->Location); }
	}
	ANF_SkillExecutionActor* Actor = GetWorld()->SpawnActorDeferred<ANF_SkillExecutionActor>(GetActiveSettings().ExecutionActorClass,
		Transform, Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Actor) { EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); return; }
	Actor->InitializeExecution(Avatar, this);
	Actor->FinishSpawning(Transform);
	// 조합 검증까지 마친 뒤 Commit한다. 설치/공격 시작은 그 이후다.
	if (!IsValid(Actor) || !Actor->PrepareExecution()
		|| (GetSkillExecutionMode() == ENF_SkillExecutionMode::Instant && !Actor->CanRunIndependently())
		|| !CommitAbility(CurrentSpecHandle, Info, CurrentActivationInfo))
	{
		if (IsValid(Actor)) { Actor->Destroy(); }
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); return;
	}
	if (!IsActive() || GetAvatarActorFromActorInfo() != Avatar || !CanContinueSkillExecution())
	{
		Actor->Destroy();
		if (IsActive()) { EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); }
		return;
	}
	ActiveExecutionActor = Actor;
	Actor->OnExecutionEnded.AddUObject(this, &UNFGA_ActiveSkillBase::FinishAutomaticExecution);
	if (!Actor->StartExecution())
	{
		if (IsActive()) { EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, true); }
		return;
	}
	if (!IsActive()) { return; }
	if (GetSkillExecutionMode() == ENF_SkillExecutionMode::Instant)
	{
		// 설치 액터는 생성 후 자신의 수명으로 유지된다.
		Actor->OnExecutionEnded.RemoveAll(this);
		ActiveExecutionActor = nullptr;
		EndAbility(CurrentSpecHandle, Info, CurrentActivationInfo, true, false);
	}
	else if (GetSkillExecutionMode() == ENF_SkillExecutionMode::Timed)
	{
		GetWorld()->GetTimerManager().SetTimer(AutomaticEndTimer, this, &UNFGA_ActiveSkillBase::FinishAutomaticExecution, GetExecutionDuration(), false);
	}
}

void UNFGA_ActiveSkillBase::FinishAutomaticExecution()
{
	if (IsActive()) { EndAbility(CurrentSpecHandle, GetCurrentActorInfo(), CurrentActivationInfo, true, false); }
}

bool UNFGA_ActiveSkillBase::UsesProjectileExecution() const
{
	return GetActiveSettings().ExecutionActorClass && (GetActiveSettings().ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->HasProjectileLaunch()
		|| GetActiveSettings().ExecutionActorClass->GetDefaultObject<ANF_SkillExecutionActor>()->HasGroundTravel());
}

bool UNFGA_ActiveSkillBase::PrepareConfirmedTargetData(const FGameplayAbilityTargetDataHandle& TargetData)
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	ClearConfirmedTargetData();
	if (!Info || !Info->IsNetAuthority() || !CanContinueSkillExecution()
		|| !UNFGA_ActiveSkillBase::ValidateConfirmedTargetData_Implementation(TargetData)
		|| !ValidateConfirmedTargetData(TargetData))
	{
		return false;
	}
	ConfirmedTargetData = TargetData;
	return true;
}

void UNFGA_ActiveSkillBase::ClearConfirmedTargetData()
{
	ConfirmedTargetData.Clear();
}


const UNF_ActiveSkillDataAsset* UNFGA_ActiveSkillBase::GetActiveData() const
{
	return Cast<UNF_ActiveSkillDataAsset>(GetSkillData());
}
const UNF_ActiveSkillDataAsset& UNFGA_ActiveSkillBase::GetActiveSettings() const
{
	const auto* Data = GetActiveData();
	return Data ? *Data : *GetDefault<UNF_ActiveSkillDataAsset>();
}
UNF_ActiveSkillDataAsset& UNFGA_ActiveSkillBase::GetMutableActiveDefaults()
{
	return *CastChecked<UNF_ActiveSkillDataAsset>(&GetMutableSkillDefaults());
}
void UNFGA_ActiveSkillBase::SetRuntimeExecutionDuration(float Duration)
{
	RuntimeExecutionDuration = FMath::IsFinite(Duration) && Duration >= 0.f ? Duration : -1.f;
}
bool UNFGA_ActiveSkillBase::CanContinueSkillExecution() const
{
	return GetActiveData() && Super::CanContinueSkillExecution();
}
bool UNFGA_ActiveSkillBase::IsBlockedByEMP() const
{
	const auto* ASC = GetAbilitySystemComponentFromActorInfo();
	return IsAffectedByEMP() && ASC && ASC->HasMatchingGameplayTag(NFTag_Skill::State_Debuff_EMP);
}
bool UNFGA_ActiveSkillBase::IsAffectedByEMP() const
{
	if (GetSkillType() != ENF_SkillType::Active) { return false; }
	const auto* Data = GetActiveData();
	if (!Data) { return false; }
	for (const auto& Rule : Data->StateRules)
	{
		if (Rule.StateTag.MatchesTag(NFTag_Skill::State_Debuff_EMP) && (Rule.bBlockActivation || Rule.bCancelRunning)) { return true; }
	}
	return false;
}
bool UNFGA_ActiveSkillBase::CanActivateAbility(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const auto* Data = Cast<UNF_ActiveSkillDataAsset>(ResolveSkillData(Handle, ActorInfo));
	if (!Data || !Data->HasValidSettings() || (Data->ExecutionActorClass
		&& NetExecutionPolicy != EGameplayAbilityNetExecutionPolicy::ServerOnly)) { return false; }
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}
void UNFGA_ActiveSkillBase::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	CancelLocalInput(); ClearConfirmedTargetData();
	Super::OnRemoveAbility(ActorInfo, Spec);
}
