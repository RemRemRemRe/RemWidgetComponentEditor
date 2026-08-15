// Copyright RemRemRemRe, All Rights Reserved.

#include "RemWidgetComponentEditorModule.h"

class FRemWidgetComponentEditorModule : public IRemWidgetComponentEditorModule
{
    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FRemWidgetComponentEditorModule, RemWidgetComponentEditor)

void FRemWidgetComponentEditorModule::StartupModule()
{
    // This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
    IRemWidgetComponentEditorModule::StartupModule();

    // ============================================================
    // Deactivation note (kept for reference, no longer active): removed together
    // with the instanced struct refactor —
    //   1. FRemComponentBasedWidgetDetails registration (replaced by native
    //      struct array editing);
    //   2. OnObjectReplaced REINST fix (recompile-crash fallback for instanced
    //      objects, removed together with the object family, see ADR-004);
    //   3. OnObjectModified / UpdateSoftObjects soft-reference rename fix
    //      (TSoftObjectPtr paths embed member names; the link contract is
    //      unchanged).
    // The full old implementation lives in git history
    // (RemWidgetComponentEditorModule.cpp, versions before 2026-07).
    // ============================================================
}

void FRemWidgetComponentEditorModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
    IRemWidgetComponentEditorModule::ShutdownModule();
}
