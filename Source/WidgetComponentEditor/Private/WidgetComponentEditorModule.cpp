// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetComponentEditorModule.h"

#include "ComponentBasedWidgetDetails.h"
#include "WidgetComponentEditorSetting.h"

class FWidgetComponentEditorModule : public IWidgetComponentEditorModule
{
	TArray<FName> RegisteredClasses;

	FDelegateHandle SettingChangedHandle;
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
protected:
	void RegisterCustomization();
	void UnregisterCustomization();

	void OnSettingChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent);
};

IMPLEMENT_MODULE(FWidgetComponentEditorModule, WidgetComponentEditorModule)

void FWidgetComponentEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	IWidgetComponentEditorModule::StartupModule();
	
	RegisterCustomization();
}

void FWidgetComponentEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	IWidgetComponentEditorModule::ShutdownModule();

	UnregisterCustomization();
}

void FWidgetComponentEditorModule::RegisterCustomization()
{
	// Register customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	bool bSignificantlyChanged = false;
	UWidgetComponentEditorSetting* WidgetComponentEditorSetting = GetMutableDefault<UWidgetComponentEditorSetting>();
	for (const TSoftClassPtr<UUserWidget> Class : WidgetComponentEditorSetting->GetWidgetClassToCustomize())
	{
		if (Class.IsNull())
		{
			continue;
		}
		
		const FName& AssetName = RegisteredClasses.Add_GetRef(*Class.GetAssetName());
		
		PropertyModule.RegisterCustomClassLayout(AssetName,
			FOnGetDetailCustomizationInstance::CreateStatic(&FComponentBasedWidgetDetails::MakeInstance));

		bSignificantlyChanged = true;
	}

	if (bSignificantlyChanged)
	{
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	if (!SettingChangedHandle.IsValid())
	{
		SettingChangedHandle = WidgetComponentEditorSetting->OnSettingChanged().AddRaw(
			this, &FWidgetComponentEditorModule::OnSettingChanged);
	}
}

void FWidgetComponentEditorModule::UnregisterCustomization()
{
	// Unregister customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	const bool bSignificantlyChanged = RegisteredClasses.Num() > 0;
	
	for (const FName ClassName : RegisteredClasses)
	{
		PropertyModule.UnregisterCustomClassLayout(ClassName);
	}

	if (bSignificantlyChanged)
	{
		RegisteredClasses.Reset();
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

void FWidgetComponentEditorModule::OnSettingChanged(UObject* Settings, FPropertyChangedEvent& PropertyChangedEvent)
{
	if (Settings)
	{
		UnregisterCustomization();
		
		RegisterCustomization();
	}
}
