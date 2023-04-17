# Features
## URemWidgetComponentEditorSetting
Directly configure whatever number of `UUserWidget` classes to turn detail customization on dynamically

## FRemWidgetComponentEditorModule
- Re-register customization when setting changed
- Fix issue with widget blueprint recompile (controlled by CVar `RemWidgetComponentEditor.FixIncorrectComponentClass`)
- Fix issue with widget renaming            (controlled by CVar `RemWidgetComponentEditor.FixSoftObjectNotUpdated`)

## FRemComponentBasedWidgetDetails
Detail customization for any user widget with `UWidgetComponentAsExtension` component