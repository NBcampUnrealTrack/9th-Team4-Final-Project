#include "GamePlay/Combat/Skill/Execution/NF_SkillCastVisual.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Project_NF.h"
#include "TimerManager.h"

ANF_SkillCastVisual::ANF_SkillCastVisual()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	SetReplicateMovement(false);
	SetActorEnableCollision(false);
}

ANF_SkillCastVisual* ANF_SkillCastVisual::SpawnForCast(AActor* Avatar, FName InMeshName,
	UAnimMontage* InMontage, float InPlayRate, FName StartSection, FName InAnimInstanceTag)
{
	if (!IsValid(Avatar) || !Avatar->HasAuthority() || !Avatar->GetWorld() || !IsValid(InMontage)) { return nullptr; }
	const int32 SectionIndex = StartSection.IsNone() ? INDEX_NONE : InMontage->GetSectionIndex(StartSection);
	if (!FMath::IsFinite(InPlayRate) || InPlayRate <= 0.f || InMontage->HasRootMotion()
		|| (!StartSection.IsNone() && SectionIndex == INDEX_NONE)
		|| !FMath::IsFinite(InMontage->GetPlayLength()) || InMontage->GetPlayLength() <= 0.f)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillAnimation] 시전 모션 생략: 속도/섹션/길이 또는 루트 모션 설정 확인. 몽타주=%s"), *GetNameSafe(InMontage));
		return nullptr;
	}
	const FTransform Transform = Avatar->GetActorTransform();
	auto* Visual = Avatar->GetWorld()->SpawnActorDeferred<ANF_SkillCastVisual>(StaticClass(), Transform,
		Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Visual) { return nullptr; }
	Visual->Montage = InMontage;
	Visual->MeshName = InMeshName;
	Visual->AnimInstanceTag = InAnimInstanceTag;
	Visual->PlayRate = InPlayRate;
	if (SectionIndex != INDEX_NONE)
	{
		float SectionEndTime = 0.f;
		InMontage->GetSectionStartAndEndTime(SectionIndex, Visual->StartPosition, SectionEndTime);
	}
	const auto* GameState = Avatar->GetWorld()->GetGameState();
	Visual->bHasSynchronizedTime = IsValid(GameState);
	Visual->ServerStartTime = GameState ? GameState->GetServerWorldTimeSeconds() : Avatar->GetWorld()->GetTimeSeconds();
	Visual->SetLifeSpan(FMath::Max(2.f, (InMontage->GetPlayLength() - Visual->StartPosition) / InPlayRate + 1.f));
	Visual->FinishSpawning(Transform);
	return Visual;
}

void ANF_SkillCastVisual::BeginPlay()
{
	Super::BeginPlay();
	TryPlay();
}

void ANF_SkillCastVisual::TryPlay()
{
	if (bPlayed || GetNetMode() == NM_DedicatedServer || !GetWorld()) { return; }
	AActor* Avatar = GetOwner();
	USkeletalMeshComponent* Mesh = nullptr;
	if (IsValid(Avatar))
	{
		TInlineComponentArray<USkeletalMeshComponent*> Meshes(Avatar);
		for (auto* Candidate : Meshes)
		{
			if (IsValid(Candidate) && Candidate->GetFName() == MeshName) { Mesh = Candidate; break; }
		}
	}
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;
	if (AnimInstance && !AnimInstanceTag.IsNone())
	{
		AnimInstance = AnimInstance->GetLinkedAnimGraphInstanceByTag(AnimInstanceTag);
	}
	if (!IsValid(Avatar) || !IsValid(Montage) || !IsValid(AnimInstance))
	{
		if (++RetryCount <= 10)
		{
			GetWorld()->GetTimerManager().SetTimer(RetryTimer, this, &ThisClass::TryPlay, 0.05f, false);
		}
		else
		{
			UE_LOG(LogProject_NF, Warning, TEXT("[SkillAnimation] 시전 모션 재생 실패: 대상/메시/AnimInstance 확인. 캐릭터=%s | 메시=%s"),
				*GetNameSafe(Avatar), *MeshName.ToString());
		}
		return;
	}
	bPlayed = true;
	const auto* GameState = GetWorld()->GetGameState();
	const double Elapsed = bHasSynchronizedTime && GameState
		? FMath::Max(0.0, GameState->GetServerWorldTimeSeconds() - ServerStartTime) : 0.0;
	const float Position = StartPosition + static_cast<float>(Elapsed) * PlayRate;
	// An old cast becoming relevant must not restart from its first frame.
	if (Position >= Montage->GetPlayLength())
	{
		UE_LOG(LogProject_NF, Display, TEXT("[SkillAnimation] Skipped expired montage Avatar=%s Montage=%s Position=%.2f"),
			*GetNameSafe(Avatar), *GetNameSafe(Montage), Position);
		return;
	}
	if (AnimInstance->Montage_Play(Montage, PlayRate, EMontagePlayReturnType::MontageLength, Position) <= 0.f)
	{
		UE_LOG(LogProject_NF, Warning, TEXT("[SkillAnimation] 몽타주 재생 실패: 스켈레톤/슬롯 설정 확인. 캐릭터=%s | 몽타주=%s"),
			*GetNameSafe(Avatar), *GetNameSafe(Montage));
	}
	else
	{
		UE_LOG(LogProject_NF, Display, TEXT("[SkillAnimation] Montage_Play succeeded Avatar=%s Montage=%s Mesh=%s NetMode=%d Position=%.2f Rate=%.2f"),
			*GetNameSafe(Avatar), *GetNameSafe(Montage), *GetNameSafe(Mesh), static_cast<int32>(GetNetMode()), Position, PlayRate);
	}
}

void ANF_SkillCastVisual::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(RetryTimer); }
	// This relay never stops montages: a newer cast/weapon montage may already own the slot.
	Super::EndPlay(EndPlayReason);
}

void ANF_SkillCastVisual::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ANF_SkillCastVisual, Montage, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_SkillCastVisual, MeshName, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_SkillCastVisual, AnimInstanceTag, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_SkillCastVisual, PlayRate, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_SkillCastVisual, StartPosition, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_SkillCastVisual, ServerStartTime, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ANF_SkillCastVisual, bHasSynchronizedTime, COND_InitialOnly);
}
