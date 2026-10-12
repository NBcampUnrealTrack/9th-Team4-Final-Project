// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project_NF : ModuleRules
{
	public Project_NF(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"ModelViewViewModel",
			"FieldNotification",
			"ContextualAnimation",
			"PoseSearch",
			"AnimationModifierLibrary"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {"OnlineSubsystem", "OnlineSubsystemUtils" });
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