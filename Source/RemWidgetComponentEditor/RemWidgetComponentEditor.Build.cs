// Copyright RemRemRemRe. All Rights Reserved.

using UnrealBuildTool;
using Rem.BuildRule;

public class RemWidgetComponentEditor : ModuleRules
{
	public RemWidgetComponentEditor(ReadOnlyTargetRules target) : base(target)
	{
        RemSharedModuleRules.Apply(this);
		
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
