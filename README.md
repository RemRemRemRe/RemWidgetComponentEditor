# Features
## UWidgetComponentEditorSetting
Directly configure whatever number of `UUserWidget` classes to turn detail customization on dynamically

## FWidgetComponentEditorModule
- Re-register customization when setting changed
- Fix issue with widget blueprint recompile (controlled by CVar `WidgetComponentEditor.FixIncorrectComponentClass`)
- Fix issue with widget renaming            (controlled by CVar `WidgetComponentEditor.FixSoftObjectNotUpdated`)

## FComponentBasedWidgetDetails
Detail customization for any user widget with `UWidgetComponentAsExtension` component