#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GamePlay/Combat/Components/NF_TargetQueryTypes.h"
#include "NF_TargetQueryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNF_TargetsFound, const TArray<FNF_TargetQueryResult>&, Targets);
// @Owner: 스킬 담당 (구현: Codex) | 2026-10-14
// 목적: 서버 대상 검색만 수행한다. GE 적용/클라이언트 표시/대상 목록 복제는 하지 않는다.
UCLASS(ClassGroup = (NF), BlueprintType, meta = (BlueprintSpawnableComponent))
class PROJECT_NF_API UNF_TargetQueryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UNF_TargetQueryComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NF|Target Query")
	FNF_TargetQuerySettings Settings;
	UPROPERTY(BlueprintAssignable, Category = "NF|Target Query")
	FNF_TargetsFound OnTargetsFound;
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "NF|Target Query")
	int32 FindTargets(FTransform AreaTransform, AActor* SourceAvatar, TArray<FNF_TargetQueryResult>& OutTargets);
	bool HasValidSettings() const;
	int32 FindTargetsWithShape(FTransform Transform, const FNF_AreaDamageShape& Shape, AActor* Source, TArray<FNF_TargetQueryResult>& OutTargets);
	static bool PassesFilter(AActor* Target, AActor* SourceAvatar, const FNF_TargetFilterSettings& Filter);
	static int32 Query(UWorld* World, AActor* AuthorityActor, AActor* SourceAvatar, FTransform AreaTransform,
		const FNF_TargetQuerySettings& QuerySettings, const FNF_AreaDamageTargetFilter& CustomFilter, TArray<FNF_TargetQueryResult>& OutTargets);
};
