// Copyright Epic Games, Inc. All Rights Reserved.


namespace UnrealBuildTool.Rules
{
	public class RemWidgetComponentEditor : ModuleRules
	{
		public RemWidgetComponentEditor(ReadOnlyTargetRules target) : base(target)
		{
			PCHUsage					= PCHUsageMode.UseExplicitOrSharedPCHs;
			bLegacyPublicIncludePaths	= false;
			ShadowVariableWarningLevel	= WarningLevel.Error;
			CppStandard 				= CppStandardVersion.Cpp20;

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
