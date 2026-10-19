// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project_NF : ModuleRules
{
	public Project_NF(ReadOnlyTargetRules Target) : base(Target)
	{
		//PublicDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "AnimGraphRuntime", "Chooser", "PoseSearch", "MotionWarping", "AnimationWarpingRuntime" });
		PublicIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Player/MotionMatching"));

		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"NetCore", // GAS TargetData의 FVector_NetQuantize 직렬화에 필요.
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"ModelViewViewModel",
			"FieldNotification",
			"ContextualAnimation",
			"Chooser",
			"PoseSearch",
			"MotionWarping",
			"AnimGraphRuntime",
			"AnimGraphRuntime",
			"AnimationModifierLibrary",
			"AnimationWarpingRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {"CoreOnline", "OnlineSubsystem", "OnlineSubsystemUtils" });
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "StateTreeEditorModule", "PropertyBindingUtils", "UnrealEd", "AssetRegistry" });
		}
		AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
		
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

		PublicIncludePaths.AddRange(new string[] {
			"Project_NF"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
