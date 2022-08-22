// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WidgetComponentEditorSetting.generated.h"

/**
 * Directly configure whatever number of `UUserWidget` classes to turn detail customization on dynamically
 */
UCLASS(config = Editor, defaultconfig, notplaceable, BlueprintType)
class WIDGETCOMPONENTEDITOR_API UWidgetComponentEditorSetting : public UDeveloperSettings
{
	GENERATED_BODY()

#pragma region Data Members

	UPROPERTY(Config, EditAnywhere, Category = Component)
	TArray<TSoftClassPtr<UUserWidget>> WidgetClassToCustomize;

#pragma endregion Data Members

public:
	TArray<TSoftClassPtr<UUserWidget>> GetWidgetClassToCustomize() const;

	void SetWidgetClassToCustomize(TArray<TSoftClassPtr<UUserWidget>> InWidgetClassToCustomize);

#if WITH_EDITOR

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	bool RemoveBlueprintClasses();

#endif

};
