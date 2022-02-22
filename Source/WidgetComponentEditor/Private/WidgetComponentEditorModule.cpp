// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetComponentEditorModule.h"

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
}

void FWidgetComponentEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	IWidgetComponentEditorModule::ShutdownModule();
}