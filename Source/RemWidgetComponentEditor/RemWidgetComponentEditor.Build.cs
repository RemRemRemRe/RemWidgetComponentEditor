// Copyright RemRemRemRe. All Rights Reserved.

using UnrealBuildTool;

public class RemWidgetComponentEditor : ModuleRules
{
	public RemWidgetComponentEditor(ReadOnlyTargetRules target) : base(target)
	{
		ShadowVariableWarningLevel = WarningLevel.Error;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		CppStandard = CppStandardVersion.EngineDefault;
		UnsafeTypeCastWarningLevel = WarningLevel.Warning;
		
		bEnableNonInlinedGenCppWarnings = true;
		bUseUnity = false;
		
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
