// Copyright Epic Games, Inc. All Rights Reserved.


namespace UnrealBuildTool.Rules
{
	public class RemWidgetComponentEditor : ModuleRules
	{
		public RemWidgetComponentEditor(ReadOnlyTargetRules target) : base(target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
			ShadowVariableWarningLevel = WarningLevel.Error;
			IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
			DefaultBuildSettings = BuildSettingsVersion.Latest;
			CppStandard = CppStandardVersion.Cpp20;

			bEnableNonInlinedGenCppWarnings = true;

			PrivateDependencyModuleNames.AddRange(
				new[]
				{
					"Core",
					"CoreUObject",
					"Engine",

					"Slate",
					"SlateCore",
					"UMG",
					"UMGEditor",
					"InputCore",
					"UnrealEd",
					"PropertyEditor",
					"EditorStyle",
					"Kismet",

					"DeveloperSettings",
					
					"RemWidgetComponent",
					"RemCommon",
					"RemEditorUtilities",
				}
			);
		}
	}
}
