// Copyright (c) 2026 AvaloNero. Licensed under the MIT License.

using UnrealBuildTool;

public class UnrealBMFont : ModuleRules
{
	public UnrealBMFont(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"UMG"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new[]
			{
				"XmlParser"
			}
		);
	}
}
