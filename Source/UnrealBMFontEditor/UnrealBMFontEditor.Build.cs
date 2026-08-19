// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

using UnrealBuildTool;

public class UnrealBMFontEditor : ModuleRules
{
	public UnrealBMFontEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ImageCore",
				"InputCore",
				"AssetDefinition",
				"AssetRegistry",
				"Projects",
				"Slate",
				"SlateCore",
				"UnrealBMFont",
				"UnrealEd"
			}
		);
	}
}
