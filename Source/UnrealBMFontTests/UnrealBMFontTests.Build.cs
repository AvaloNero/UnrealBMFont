// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

using UnrealBuildTool;

public class UnrealBMFontTests : ModuleRules
{
	public UnrealBMFontTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
				"RHI",
				"Slate",
				"SlateCore",
				"UMG",
				"UnrealBMFont"
			}
		);
	}
}
