#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NF_HealthDebugComponent.generated.h"

class UAbilitySystemComponent;

// @Owner: 이정준 (구현: Codex) | 2026-10-26
// 임시 테스트용: 기존 플레이어 ASC의 체력/쉴드를 읽어서 화면에 표시한다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_HealthDebugComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_HealthDebugComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Health Debug", meta = (ToolTip = "장착한 캐릭터의 체력과 쉴드를 화면 디버그 메시지로 표시합니다. Shipping 빌드에서는 표시하지 않습니다."))
	bool bShowOnScreen = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Health Debug", meta = (ToolTip = "화면에 표시할 체력 디버그 메시지의 색상입니다."))
	FColor TextColor = FColor::Green;

private:
	void RefreshDisplay();
	void RemoveDisplay();
	UAbilitySystemComponent* FindHealthASC() const;
	uint64 GetMessageKey() const;
};
