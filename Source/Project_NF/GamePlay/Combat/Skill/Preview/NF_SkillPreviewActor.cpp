#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewActor.h"

#include "Abilities/GameplayAbility.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "GamePlay/Combat/Skill/NFGA_SkillBase.h"

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
	bPreviewInitialized = true;
	SetActorTickEnabled(true);
	PreviewStarted();
	if (!IsValid(this))
	{
		return false;
	}
	UpdatePreview();
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
	UpdatePreview();
}

void ANF_SkillPreviewActor::UpdatePreview_Implementation()
{
	// 스킬별 자식 액터/BP가 부착한 컴포넌트에 표시 데이터를 전달한다.
}
