﻿// Copyright RemRemRemRe, All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "RemWidgetComponentEditorSetting.generated.h"

class UUserWidget;

/**
 * Directly configure whatever number of `UUserWidget` classes to turn detail customization on dynamically
 */
UCLASS(config = Editor, defaultconfig, notplaceable, BlueprintType)
class REMWIDGETCOMPONENTEDITOR_API URemWidgetComponentEditorSetting : public UDeveloperSettings
{
	GENERATED_BODY()

#pragma region Data Members

	UPROPERTY(Config, EditAnywhere, Category = Component)
	TArray<TSoftClassPtr<UUserWidget>> WidgetClassToCustomize;

#pragma endregion Data Members

public:
	TArray<TSoftClassPtr<UUserWidget>> GetWidgetClassToCustomize() const;

	void SetWidgetClassToCustomize(const TArray<TSoftClassPtr<UUserWidget>>& InWidgetClassToCustomize);

#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	bool RemoveBlueprintClasses();

#endif

};
