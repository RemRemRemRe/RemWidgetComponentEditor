// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetComponentEditorModule.h"

#include "ComponentBasedWidget.h"
#include "ComponentBasedWidgetDetails.h"

class FWidgetComponentEditorModule : public IWidgetComponentEditorModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FWidgetComponentEditorModule, WidgetComponentEditorModule)

void FWidgetComponentEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	IWidgetComponentEditorModule::StartupModule();

	// Register customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	PropertyModule.RegisterCustomClassLayout(UComponentBasedWidget::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FComponentBasedWidgetDetails::MakeInstance));
	
	PropertyModule.NotifyCustomizationModuleChanged();
}

void FWidgetComponentEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	IWidgetComponentEditorModule::ShutdownModule();

	// Register customizations
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	
	PropertyModule.UnregisterCustomPropertyTypeLayout(UComponentBasedWidget::StaticClass()->GetFName());
	
	PropertyModule.NotifyCustomizationModuleChanged();
}