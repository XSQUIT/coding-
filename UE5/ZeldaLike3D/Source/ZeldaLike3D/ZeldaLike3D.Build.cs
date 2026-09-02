// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ZeldaLike3D : ModuleRules
{
	public ZeldaLike3D(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ZeldaLike3D",
			"ZeldaLike3D/Variant_Platforming",
			"ZeldaLike3D/Variant_Platforming/Animation",
			"ZeldaLike3D/Variant_Combat",
			"ZeldaLike3D/Variant_Combat/AI",
			"ZeldaLike3D/Variant_Combat/Animation",
			"ZeldaLike3D/Variant_Combat/Gameplay",
			"ZeldaLike3D/Variant_Combat/Interfaces",
			"ZeldaLike3D/Variant_Combat/UI",
			"ZeldaLike3D/Variant_SideScrolling",
			"ZeldaLike3D/Variant_SideScrolling/AI",
			"ZeldaLike3D/Variant_SideScrolling/Gameplay",
			"ZeldaLike3D/Variant_SideScrolling/Interfaces",
			"ZeldaLike3D/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
