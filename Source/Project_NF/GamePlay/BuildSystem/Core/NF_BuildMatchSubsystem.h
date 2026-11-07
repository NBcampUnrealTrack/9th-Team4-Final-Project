#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GamePlay/BuildSystem/Core/NF_BuildTypesInterface.h"
#include "NF_BuildMatchSubsystem.generated.h"

class ANF_GameMode;
class ANF_PlayerState;
class ANF_BuildShopZone;

/** 월드별 빌드 공통 조회·검증과 상점 등록을 담당한다. Core 기본 빌드 연동은 서버에서만 실행한다. */
UCLASS()
class PROJECT_NF_API UNF_BuildMatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 월드 없이 실행할 수 있어 Rules 에디터 검증과 런타임 검증이 같은 규칙을 사용한다.
	static FNF_BuildValidation ValidateSelections(
		const UNF_BuildRules* Rules, int32 CostLimit, const TArray<FNF_BuildSelection>& Selections,
		TMap<FName, FNF_BuildItemData>* OutSelectedData = nullptr,
		const TArray<FNF_BuildSelection>* OwnedSelections = nullptr);
	static FNF_BuildValidation ValidateCatalog(
		const UNF_BuildRules* Rules, const TArray<FNF_BuildCatalogItem>& Items, int32 CostLimit,
		const TArray<FNF_BuildSelection>& Selections, TMap<FName, FNF_BuildItemData>* OutSelectedData,
		const TArray<FNF_BuildSelection>* OwnedSelections);
	static bool IsArmorDefinitionCompatible(const FNF_BuildItemData& Data);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void RegisterShop(ANF_BuildShopZone* Shop);
	void UnregisterShop(ANF_BuildShopZone* Shop);

	UNF_BuildItemDefinitionDataAsset* FindItemDefinition(const FNF_BuildQueryContext& Context, FName ItemId) const;
	bool ResolveItemData(const FNF_BuildQueryContext& Context, FName ItemId, FNF_BuildItemData& OutData) const;
	UDataAsset* GetItemSource(const FNF_BuildQueryContext& Context, FName ItemId) const;
	TArray<UNF_BuildItemDefinitionDataAsset*> GetCatalogItems(
		const FNF_BuildQueryContext& Context, ENF_ItemType ItemType, ENF_ArmorSlot ArmorPart) const;
	FNF_BuildValidation PreviewBuild(
		const FNF_BuildQueryContext& Context, const TArray<FNF_BuildSelection>& Selections) const;
	FNF_BuildShopPreview GetShopPreview(const FNF_BuildQueryContext& Context) const;
	FNF_BuildValidation ValidateBuild(
		const UNF_BuildRules* Rules, int32 CostLimit, const TArray<FNF_BuildSelection>& Selections,
		const TArray<FNF_BuildSelection>& CurrentBuild, const TArray<FName>& PaidItemIds,
		TMap<FName, FNF_BuildItemData>* OutSelectedData = nullptr) const;

	bool CanApplyFallbackBuild(ANF_BuildShopZone* Shop, AActor* Player) const;
	ENF_BuildFallbackAction PrepareFallbackBuild(
		ANF_BuildShopZone* Shop, AActor* Player, int32 CostLimit,
		const TArray<FNF_BuildSelection>& CurrentBuild, bool bReady,
		TArray<FNF_BuildSelection>& OutSelections) const;

private:
	bool HandleApplyFallbackBuild(ANF_PlayerState* PlayerState);
	TWeakObjectPtr<ANF_GameMode> RegisteredGameMode;
	TArray<TWeakObjectPtr<ANF_BuildShopZone>> Shops;
	TArray<FNF_BuildSelection> GetFreeSelections(
		const TArray<FNF_BuildSelection>& CurrentBuild, const TArray<FName>& PaidItemIds) const;
};
