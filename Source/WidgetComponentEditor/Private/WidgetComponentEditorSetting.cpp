// Fill out your copyright notice in the Description page of Project Settings.


#include "WidgetComponentEditorSetting.h"

#include "Blueprint/UserWidget.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "WidgetComponentEditorSetting"

TArray<TSoftClassPtr<UUserWidget>> UWidgetComponentEditorSetting::GetWidgetClassToCustomize() const
{
	return WidgetClassToCustomize;
}

void UWidgetComponentEditorSetting::SetWidgetClassToCustomize(
	const TArray<TSoftClassPtr<UUserWidget>> InWidgetClassToCustomize)
{
	WidgetClassToCustomize = InWidgetClassToCustomize;
}

#if WITH_EDITOR

void UWidgetComponentEditorSetting::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
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
		Info.Image = FEditorStyle::GetBrush(TEXT("MessageLog.Warning"));
		Info.FadeOutDuration = 4.0f;
		Info.ExpireDuration = Info.FadeOutDuration;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

bool UWidgetComponentEditorSetting::RemoveBlueprintClasses()
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