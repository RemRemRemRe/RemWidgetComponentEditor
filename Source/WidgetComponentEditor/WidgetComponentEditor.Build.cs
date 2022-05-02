// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class WidgetComponentEditor : ModuleRules
	{
		public WidgetComponentEditor(ReadOnlyTargetRules target) : base(target)
		{
			PCHUsage					= PCHUsageMode.UseExplicitOrSharedPCHs;
			bLegacyPublicIncludePaths	= false;
			ShadowVariableWarningLevel	= WarningLevel.Error;

			PrivateDependencyModuleNames.AddRange(
				new[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"Slate",
					"SlateCore",
					"UMG",
					"InputCore",
					"UnrealEd",
					"PropertyEditor",
					"EditorStyle",
					"DeveloperSettings",
					"WidgetComponent",
					"Common",
					"DetailCustomizationUtilities",
				}
			);
		}
	}
}
