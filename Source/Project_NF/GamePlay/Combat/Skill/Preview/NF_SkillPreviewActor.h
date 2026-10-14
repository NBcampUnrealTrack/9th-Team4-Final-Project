#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GamePlay/Combat/Skill/Preview/NF_SkillPreviewData.h"
#include "NF_SkillPreviewActor.generated.h"

class UGameplayAbility;

/** @Owner: 스킬 담당 (구현: Codex) | 2026-10-13
 * 목적: 스킬별 표시 컴포넌트를 부착하는 로컬 조준 표시 액터의 공통 베이스.
 * GA가 생성/종료하고 액터가 매 프레임 표시를 갱신한다. 서버 피해 판정은 수행하지 않는다.
 */
UCLASS(Abstract, Blueprintable)
class PROJECT_NF_API ANF_SkillPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ANF_SkillPreviewActor();
	bool InitializePreview(AActor* InSourceAvatar, UGameplayAbility* InSourceAbility);
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "NF|Skill Preview")
	AActor* GetSourceAvatar() const { return SourceAvatar.Get(); }

	UFUNCTION(BlueprintPure, Category = "NF|Skill Preview")
	UGameplayAbility* GetSourceAbility() const;

	UFUNCTION(BlueprintPure, Category = "NF|Skill Preview")
	bool GetPreviewData(FNF_SkillPreviewData& OutData) const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Source Avatar/Ability 설정 이후 호출한다. BP에서 포물선 Start Preview 등을 연결한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill Preview")
	void PreviewStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "NF|Skill Preview")
	void PreviewStopped();

	UFUNCTION(BlueprintNativeEvent, Category = "NF|Skill Preview")
	void UpdatePreview();
	virtual void UpdatePreview_Implementation();

private:
	bool bPreviewInitialized = false;
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> SourceAvatar;

	UPROPERTY(Transient)
	TWeakObjectPtr<UGameplayAbility> SourceAbility;
};
