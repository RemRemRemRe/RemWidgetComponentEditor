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
					"InputCore",
					"UnrealEd",
					"PropertyEditor",
					"EditorStyle",

					"DeveloperSettings",
					
					"RemWidgetComponent",
					"RemCommon",
					"RemEditorUtilities",
				}
			);
		}
	}
}
