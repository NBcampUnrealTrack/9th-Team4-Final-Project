#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewActor.h"
#include "NF_GrenadePreviewActor.generated.h"

class UNF_ProjectileTrajectoryComponent;
class UNF_AreaIndicatorComponent;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 부착한 포물선/장판 컴포넌트를 조합하여 수류탄 조준을 표시한다.
 */
UCLASS(Blueprintable)
class PROJECT_NF_API ANF_GrenadePreviewActor : public ANF_SkillPreviewActor
{
	GENERATED_BODY()

public:
	ANF_GrenadePreviewActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void UpdatePreview_Implementation() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Skill Preview")
	TObjectPtr<UNF_ProjectileTrajectoryComponent> Trajectory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NF|Skill Preview")
	TObjectPtr<UNF_AreaIndicatorComponent> AreaIndicator;
};
