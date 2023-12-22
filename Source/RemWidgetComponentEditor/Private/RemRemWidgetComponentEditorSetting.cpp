// Copyright RemRemRemRe, All Rights Reserved.


#include "RemWidgetComponentEditorSetting.h"

#include "Blueprint/UserWidget.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "RemWidgetComponentEditorSetting"

TArray<TSoftClassPtr<UUserWidget>> URemWidgetComponentEditorSetting::GetWidgetClassToCustomize() const
{
	return WidgetClassToCustomize;
}

void URemWidgetComponentEditorSetting::SetWidgetClassToCustomize(
	const TArray<TSoftClassPtr<UUserWidget>>& InWidgetClassToCustomize)
{
	WidgetClassToCustomize = InWidgetClassToCustomize;
}

#if WITH_EDITOR

void URemWidgetComponentEditorSetting::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (!RemoveBlueprintClasses())
	{
		// valid selection can fire the multicast delegate
		Super::PostEditChangeProperty(PropertyChangedEvent);
	}
	else
	{
		// emit a warning message
		FNotificationInfo Info(LOCTEXT("OnlyAllowNativeClass", "Only native class is allowed."));
		Info.bUseThrobber = true;
		Info.Image = FAppStyle::GetBrush(TEXT("MessageLog.Warning"));
		Info.FadeOutDuration = 4.0f;
		Info.ExpireDuration = Info.FadeOutDuration;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

bool URemWidgetComponentEditorSetting::RemoveBlueprintClasses()
{
	bool bFoundBlueprintClass = false;
		
	for (int32 Index = WidgetClassToCustomize.Num() - 1; Index >= 0; --Index)
	{
		TSoftClassPtr<UUserWidget>& SoftClassPtr = WidgetClassToCustomize[Index];
		if (SoftClassPtr.IsNull())
		{
			continue;
		}

		// whether it is a blueprint class
		if (const UClass* Class = SoftClassPtr.LoadSynchronous();
			Class && Class->ClassGeneratedBy)
		{
			SoftClassPtr.Reset();
			bFoundBlueprintClass = true;
		}
	}

	return bFoundBlueprintClass;
}

#endif

#undef LOCTEXT_NAMESPACE
