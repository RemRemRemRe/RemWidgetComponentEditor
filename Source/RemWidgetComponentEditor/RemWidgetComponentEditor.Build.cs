// Copyright RemRemRemRe. All Rights Reserved.

using UnrealBuildTool;

public class RemWidgetComponentEditor : ModuleRules
{
	public RemWidgetComponentEditor(ReadOnlyTargetRules target) : base(target)
	{
		CppCompileWarningSettings.ShadowVariableWarningLevel = WarningLevel.Error;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		CppStandard = CppStandardVersion.EngineDefault;
		CppCompileWarningSettings.UnsafeTypeCastWarningLevel = WarningLevel.Warning;
		
		CppCompileWarningSettings.NonInlinedGenCppWarningLevel = WarningLevel.Warning;
		bUseUnity = false;
		
		PrivateDependencyModuleNames.AddRange(
			[
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
			]
		);
	}
}
