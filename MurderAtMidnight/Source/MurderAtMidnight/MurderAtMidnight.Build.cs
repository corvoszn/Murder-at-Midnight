// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MurderAtMidnight : ModuleRules
{
	public MurderAtMidnight(ReadOnlyTargetRules Target) : base(Target)
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
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"OnlineSubsystemEOS",
			"EOSShared",
			"EOSSDK",
			"VoiceChat"
		});

		PublicIncludePaths.AddRange(new string[] {
			"MurderAtMidnight",
		});
	}
}
