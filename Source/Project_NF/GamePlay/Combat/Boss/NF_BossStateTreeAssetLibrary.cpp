#include "GamePlay/Combat/Boss/NF_BossStateTreeAssetLibrary.h"

#include "GamePlay/Combat/Boss/NF_BossBase.h"
#include "GamePlay/Combat/Boss/NF_BossCombatData.h"
#include "GamePlay/Combat/Boss/NF_BossStateTreeTask.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossAreaVolley.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossCenterBlast.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossGroundDamageZone.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossLastGasp.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossRelocate.h"
#include "GamePlay/Combat/Skill/Active/Boss/NFGA_BossThreatVolley.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataTable.h"
#include "StateTree.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StateTreeComponentSchema.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "StateTreeEditorData.h"
#include "StateTreeEditorModule.h"
#include "StateTreeEditorSchema.h"
#include "StateTreeState.h"
#include "StateTreeTypes.h"
#include "UObject/SavePackage.h"
#endif

#if WITH_EDITOR
namespace
{
	UDataTable* LoadOrCreateBossCombatDataTable()
	{
		static const FString PackageName = TEXT("/Game/ProjectNF/Data/Boss/DT_BossCombat");
		UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *(PackageName + TEXT(".DT_BossCombat")));
		const bool bIsNewAsset = DataTable == nullptr;
		UPackage* Package = bIsNewAsset ? CreatePackage(*PackageName) : DataTable->GetOutermost();
		if (!Package)
		{
			return nullptr;
		}

		if (bIsNewAsset)
		{
			DataTable = NewObject<UDataTable>(Package, TEXT("DT_BossCombat"),
				RF_Public | RF_Standalone | RF_Transactional);
			if (!DataTable)
			{
				return nullptr;
			}
			DataTable->RowStruct = FNF_BossCombatDataRow::StaticStruct();
			DataTable->AddRow(TEXT("DefaultBoss"), FNF_BossCombatDataRow());
			FAssetRegistryModule::AssetCreated(DataTable);
		}
		else if (DataTable->GetRowStruct() != FNF_BossCombatDataRow::StaticStruct())
		{
			UE_LOG(LogTemp, Error, TEXT("[BossData] DT_BossCombat uses an unexpected row struct."));
			return nullptr;
		}

		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackageName, FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		if (!UPackage::SavePackage(Package, DataTable, *Filename, SaveArgs))
		{
			UE_LOG(LogTemp, Error, TEXT("[BossData] Could not save DT_BossCombat to %s"), *Filename);
			return nullptr;
		}
		return DataTable;
	}
}
#endif

bool UNF_BossStateTreeAssetLibrary::AssignBossPatternCycleToTestBoss(UStateTree* StateTree)
{
#if WITH_EDITOR
	if (!StateTree)
	{
		return false;
	}

	static const TCHAR* BossBlueprintPath = TEXT("/Game/ProjectNF/Persnal/Myungwoo/BP_TestBoss.BP_TestBoss");
	UBlueprint* BossBlueprint = LoadObject<UBlueprint>(nullptr, BossBlueprintPath);
	if (!BossBlueprint || !BossBlueprint->GeneratedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTree] Could not load BP_TestBoss"));
		return false;
	}

	ANF_BossBase* BossDefaults = Cast<ANF_BossBase>(BossBlueprint->GeneratedClass->GetDefaultObject());
	if (!BossDefaults)
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTree] BP_TestBoss does not derive from ANF_BossBase"));
		return false;
	}

	UClass* FixedVolleyClass = LoadClass<UGameplayAbility>(nullptr,
		TEXT("/Game/ProjectNF/Persnal/Myungwoo/GA_BossAreaVolley.GA_BossAreaVolley_C"));
	if (!FixedVolleyClass)
	{
		FixedVolleyClass = UNFGA_BossAreaVolley::StaticClass();
	}
	UClass* GroundDamageZoneClass = LoadClass<UGameplayAbility>(nullptr,
		TEXT("/Game/ProjectNF/Persnal/Myungwoo/GA_BossGroundDamageZone.GA_BossGroundDamageZone_C"));
	if (!GroundDamageZoneClass)
	{
		GroundDamageZoneClass = UNFGA_BossGroundDamageZone::StaticClass();
	}

	BossDefaults->Modify();
	BossDefaults->BossStateTree = StateTree;
	BossDefaults->InitialPatternAbility = FixedVolleyClass;
	if (UDataTable* BossCombatTable = LoadOrCreateBossCombatDataTable())
	{
		BossDefaults->BossCombatDataTable = BossCombatTable;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BossData] Could not assign DT_BossCombat to BP_TestBoss."));
	}
	BossDefaults->CombatPatternAbilities = {
		FixedVolleyClass,
		UNFGA_BossThreatVolley::StaticClass(),
		GroundDamageZoneClass,
		UNFGA_BossRelocate::StaticClass(),
		UNFGA_BossCenterBlast::StaticClass()
	};
	BossDefaults->LastGaspAbility = UNFGA_BossLastGasp::StaticClass();

	FBlueprintEditorUtils::MarkBlueprintAsModified(BossBlueprint);
	BossBlueprint->MarkPackageDirty();
	const FString PackageName = BossBlueprint->GetOutermost()->GetName();
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(BossBlueprint->GetOutermost(), BossBlueprint, *Filename, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTree] Could not save BP_TestBoss to %s"), *Filename);
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("[BossStateTree] Assigned StateTree and combat abilities to BP_TestBoss"));
	return true;
#else
	return false;
#endif
}

UStateTree* UNF_BossStateTreeAssetLibrary::CreateBossPatternCycleStateTree()
{
#if WITH_EDITOR
	static const FString PackageName = TEXT("/Game/ProjectNF/Gameplay/Boss/ST_BossPatternCycle");
	UStateTree* StateTree = LoadObject<UStateTree>(nullptr, *(PackageName + TEXT(".ST_BossPatternCycle")));
	const bool bIsNewAsset = StateTree == nullptr;
	UPackage* Package = bIsNewAsset ? CreatePackage(*PackageName) : StateTree->GetOutermost();
	if (!Package)
	{
		return nullptr;
	}
	if (bIsNewAsset)
	{
		StateTree = NewObject<UStateTree>(Package, TEXT("ST_BossPatternCycle"), RF_Public | RF_Standalone | RF_Transactional);
	}
	else
	{
		StateTree->Modify();
	}
	if (!StateTree)
	{
		return nullptr;
	}

	const FStateTreeEditorModule& StateTreeEditorModule = FStateTreeEditorModule::GetModule();
	const TNonNullSubclassOf<UStateTreeEditorData> EditorDataClass =
		StateTreeEditorModule.GetEditorDataClass(UStateTreeComponentSchema::StaticClass());
	UStateTreeEditorData* EditorData = NewObject<UStateTreeEditorData>(StateTree, EditorDataClass, TEXT("EditorData"), RF_Transactional);
	UStateTreeComponentSchema* Schema = NewObject<UStateTreeComponentSchema>(EditorData, TEXT("Schema"), RF_Transactional);
	EditorData->Schema = Schema;
	EditorData->EditorSchema = NewObject<UStateTreeEditorSchema>(EditorData, TEXT("EditorSchema"), RF_Transactional);
	StateTree->EditorData = EditorData;

	UStateTreeState& Root = EditorData->AddRootState();
	Root.Name = TEXT("BossEncounter");
	Root.Description = TEXT("Waits in Dormant until the server wakes the boss, then runs combat patterns and enters LastGasp when defeated.");
	Root.SelectionBehavior = EStateTreeStateSelectionBehavior::TrySelectChildrenInOrder;

	UClass* FixedVolleyClass = LoadClass<UGameplayAbility>(nullptr,
		TEXT("/Game/ProjectNF/Persnal/Myungwoo/GA_BossAreaVolley.GA_BossAreaVolley_C"));
	if (!FixedVolleyClass)
	{
		FixedVolleyClass = UNFGA_BossAreaVolley::StaticClass();
	}
	UClass* GroundDamageZoneClass = LoadClass<UGameplayAbility>(nullptr,
		TEXT("/Game/ProjectNF/Persnal/Myungwoo/GA_BossGroundDamageZone.GA_BossGroundDamageZone_C"));
	if (!GroundDamageZoneClass)
	{
		GroundDamageZoneClass = UNFGA_BossGroundDamageZone::StaticClass();
	}

	UStateTreeState& Dormant = Root.AddChildState(TEXT("Dormant"));
	UStateTreeState& FixedVolley = Root.AddChildState(TEXT("FixedPointVolley"));
	UStateTreeState& ThreatVolley = Root.AddChildState(TEXT("RandomTargetVolley"));
	UStateTreeState& GroundDamageZone = Root.AddChildState(TEXT("GroundDamageZones"));
	UStateTreeState& Relocate = Root.AddChildState(TEXT("RelocateAndEmerge"));
	UStateTreeState& CenterBlast = Root.AddChildState(TEXT("CenterBlast"));
	UStateTreeState& LastGasp = Root.AddChildState(TEXT("LastGasp"));
	Dormant.Description = TEXT("The boss remains idle until WakeBoss is called on the server.");
	Dormant.AddTask<FNF_BossDormantTask>();

	auto AddAbilityTask = [](UStateTreeState& State, const TSubclassOf<UGameplayAbility> AbilityClass,
		const bool bAllowWhenDefeated)
	{
		auto& TaskNode = State.AddTask<FNF_BossAbilityTask>();
		FNF_BossAbilityTaskInstanceData& InstanceData = TaskNode.GetInstanceData();
		InstanceData.AbilityClass = AbilityClass;
		InstanceData.bAllowWhenBossDefeated = bAllowWhenDefeated;
	};

	auto AddDefeatedTransition = [&LastGasp](UStateTreeState& State)
	{
		FStateTreeTransition& Transition = State.AddTransition(
			EStateTreeTransitionTrigger::OnTick, EStateTreeTransitionType::GotoState, &LastGasp);
		Transition.Priority = EStateTreeTransitionPriority::Critical;
		Transition.AddCondition<FNF_BossDefeatedCondition>();
	};

	AddAbilityTask(FixedVolley, FixedVolleyClass, false);
	AddAbilityTask(ThreatVolley, UNFGA_BossThreatVolley::StaticClass(), false);
	AddAbilityTask(GroundDamageZone, GroundDamageZoneClass, false);
	AddAbilityTask(Relocate, UNFGA_BossRelocate::StaticClass(), false);
	AddAbilityTask(CenterBlast, UNFGA_BossCenterBlast::StaticClass(), false);
	AddAbilityTask(LastGasp, UNFGA_BossLastGasp::StaticClass(), true);

	FixedVolley.Description = TEXT("Fire four fixed-point volleys, then finish after the last warning resolves.");
	ThreatVolley.Description = TEXT("Select a random player in range and fire a rotating aimed volley.");
	GroundDamageZone.Description = TEXT("Mark up to three players, then create two-second zones that pulse damage every 0.25 seconds.");
	Relocate.Description = TEXT("Take off, telegraph the landing area, then dive and launch nearby players.");
	CenterBlast.Description = TEXT("Telegraph the boss-centered blast, then damage, knock back, and stun nearby players.");
	LastGasp.Description = TEXT("On boss defeat, mark each player's location and detonate after four seconds.");

	FixedVolley.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextState);
	Dormant.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextState);
	ThreatVolley.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextState);
	GroundDamageZone.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextState);
	Relocate.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::NextState);
	CenterBlast.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::GotoState, &FixedVolley);
	LastGasp.AddTransition(EStateTreeTransitionTrigger::OnStateCompleted, EStateTreeTransitionType::Succeeded);

	AddDefeatedTransition(FixedVolley);
	AddDefeatedTransition(Dormant);
	AddDefeatedTransition(ThreatVolley);
	AddDefeatedTransition(GroundDamageZone);
	AddDefeatedTransition(Relocate);
	AddDefeatedTransition(CenterBlast);

	FStateTreeCompilerLog CompilerLog;
	FStateTreeCompiler Compiler(CompilerLog);
	if (!Compiler.Compile(*StateTree))
	{
		CompilerLog.DumpToLog(LogTemp);
		return nullptr;
	}
	if (!StateTree->GetSchema() || !StateTree->IsReadyToRun())
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTree] Compile completed without producing a runnable tree or schema"));
		return nullptr;
	}

	if (bIsNewAsset)
	{
		FAssetRegistryModule::AssetCreated(StateTree);
	}
	Package->MarkPackageDirty();
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	if (!UPackage::SavePackage(Package, StateTree, *Filename, SaveArgs))
	{
		UE_LOG(LogTemp, Error, TEXT("[BossStateTree] Could not save %s"), *Filename);
		return nullptr;
	}
	if (!AssignBossPatternCycleToTestBoss(StateTree))
	{
		return nullptr;
	}
	return StateTree;
#else
	return nullptr;
#endif
}
