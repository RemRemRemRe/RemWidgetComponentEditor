// Copyright Epic Games, Inc. All Rights Reserved.

#include "WidgetComponentEditorModule.h"

#include "BaseWidgetBlueprint.h"
#include "ComponentBasedWidgetDetails.h"
#include "WidgetComponentAsExtension.h"
#include "WidgetComponentBase.h"
#include "WidgetComponentEditorSetting.h"
#include "WidgetComponentStatics.h"
#include "Blueprint/UserWidget.h"
#include "Macro/AssertionMacros.h"
#include "Blueprint/WidgetTree.h"

class FWidgetComponentEditorModule : public IWidgetComponentEditorModule
{
	TArray<FName> RegisteredClasses;

	FDelegateHandle SettingChangedHandle;
	FDelegateHandle ObjectModifiedHandle;

	uint64 SavedFrameCounter = 0;
	TWeakObjectPtr<const UBaseWidgetBlueprint> WidgetBlueprint;
	TWeakObjectPtr<const UWidget> Widget;
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
protected:
	void RegisterCustomization();
	void UnregisterCustomization();

	void OnSettingChanged(UObject* Settings, FPropertyChangedEvent&);

	void OnObjectReplaced(const TMap<UObject*, UObject*>& ReplacementObjectMap) const;
	void OnObjectModified(UObject* Object);
	void UpdateSoftObjects(TWeakObjectPtr<const UBaseWidgetBlueprint> InWidgetBlueprint) const;

	FDelegateHandle OnObjectReplacedHandle;
};

IMPLEMENT_MODULE(FWidgetComponentEditorModule, WidgetComponentEditor)

void FWidgetComponentEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	IWidgetComponentEditorModule::StartupModule();
	
	RegisterCustomization();
	
	OnObjectReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FWidgetComponentEditorModule::OnObjectReplaced);
	ObjectModifiedHandle = FCoreUObjectDelegates::OnObjectModified.AddRaw(this, &FWidgetComponentEditorModule::OnObjectModified);
}

void FWidgetComponentEditorModule::ShutdownModule()
{
	FCoreUObjectDelegates::OnObjectModified.Remove(ObjectModifiedHandle);
	FCoreUObjectDelegates::OnObjectsReplaced.Remove(OnObjectReplacedHandle);
	
	UnregisterCustomization();
	
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	IWidgetComponentEditorModule::ShutdownModule();
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

void FWidgetComponentEditorModule::OnSettingChanged(UObject* Settings, FPropertyChangedEvent&)
{
	if (Settings)
	{
		UnregisterCustomization();
		
		RegisterCustomization();
	}
}

static FAutoConsoleVariable CVarWidgetComponentEditorFixIncorrectComponentClass(
	TEXT("WidgetComponentEditor.FixIncorrectComponentClass"), true,
		TEXT("Trying to fix incorrect component class when component blueprint recompiled."));

void FWidgetComponentEditorModule::OnObjectReplaced(const TMap<UObject*, UObject*>& ReplacementObjectMap) const
{
	if (!CVarWidgetComponentEditorFixIncorrectComponentClass->GetBool())
	{
		return;
	}
	
	TArray<UObject*> Components;
	GetObjectsOfClass(UWidgetComponentBase::StaticClass(), Components,
		true);

	for (const UObject* Object : Components)
	{
		if (!IsValid(Object->GetOuter()))
		{
			continue;
		}

		const UBaseWidgetBlueprint* Blueprint = Cast<UBaseWidgetBlueprint>(Object->GetOuter()->GetClass()->ClassGeneratedBy);
		if (!Blueprint)
		{
			continue;
		}

		const UClass* GeneratedClass = Blueprint->GeneratedClass;
		if (!GeneratedClass)
		{
			continue;
		}

		UUserWidget* OwnerWidgetCDO = Cast<UUserWidget>(GeneratedClass->GetDefaultObject(false));
		if (!OwnerWidgetCDO)
		{
			continue;
		}
		
		// Remove all nullptr from UUserWidget::Extensions
		OwnerWidgetCDO->RemoveExtension(nullptr);
		
		const UWidgetComponentAsExtension* Extension = OwnerWidgetCDO->GetExtension<UWidgetComponentAsExtension>();
		if (!Extension)
		{
			continue;
		}
			
		WidgetComponentStatics::ForeachUserWidgetComponent(Extension,
[&](UWidgetComponentBase** MemberPtr, int32)
		{
			CheckPointer(MemberPtr, return);
			UWidgetComponentBase* OldObject = *MemberPtr;
		
			CheckPointer(OldObject, return);

			if (const UObject* const* ReplacementClassPtr =  ReplacementObjectMap.Find(OldObject->GetClass()))
			{
				if (const UClass* ReplacementClass = Cast<UClass>(*ReplacementClassPtr))
				{
					const FName SavedName = OldObject->GetFName();
						
					OldObject->Rename(nullptr, GetTransientPackage(),
				REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
					
					UWidgetComponentBase* NewComponent = NewObject<UWidgetComponentBase>(OwnerWidgetCDO,
						ReplacementClass, SavedName);
						
					OwnerWidgetCDO->Modify();

					UEngine::FCopyPropertiesForUnrelatedObjectsParams Params;
					// During a blueprint re-parent, delta serialization must be enabled to correctly copy all properties
					Params.bDoDelta = true;
					Params.bCopyDeprecatedProperties = true;
					Params.bSkipCompilerGeneratedDefaults = true;
					Params.bClearReferences = false;
					Params.bNotifyObjectReplacement = false;
					UEngine::CopyPropertiesForUnrelatedObjects(OldObject, NewComponent, Params);
					
					OldObject->MarkAsGarbage();
					*MemberPtr = NewComponent;
				}
			}
		});
	}
}

static FAutoConsoleVariable CVarWidgetComponentEditorFixSoftObjectNotUpdated(
	TEXT("WidgetComponentEditor.FixSoftObjectNotUpdated"), true,
		TEXT("Trying to fix soft object reference did not get updated after the widget get renamed."));

void FWidgetComponentEditorModule::OnObjectModified(UObject* Object)
{
	if (!CVarWidgetComponentEditorFixSoftObjectNotUpdated->GetBool())
	{
		return;
	}

	if (!IsValid(Object))
	{
		return;
	}

	// The implementation is determined by FWidgetBlueprintEditorUtils::RenameWidget
	
	if (!WidgetBlueprint.IsValid())
	{
		if (UBaseWidgetBlueprint* Blueprint = Cast<UBaseWidgetBlueprint>(Object))
		{
			SavedFrameCounter = GFrameCounter;
			WidgetBlueprint = Blueprint;
		}
	}
	else if (!Object->IsA<UWidget>())
	{
		// skip modified call back of root widget and etc...
	}
	else if (!Widget.IsValid())
	{
		if (const UWidget* WidgetObject = Cast<UWidget>(Object);
			SavedFrameCounter == GFrameCounter
			&& WidgetObject && !WidgetObject->IsDesignTime()
			&& WidgetBlueprint.Get()->WidgetTree->FindWidget(WidgetObject->GetFName()) == WidgetObject)
		{
			Widget = WidgetObject;
		}
		else
		{
			WidgetBlueprint.Reset();
		}
	}
	else
	{
		if (const UWidget* PreviewWidget = Cast<UWidget>(Object);
			SavedFrameCounter == GFrameCounter
			&& PreviewWidget && PreviewWidget->IsDesignTime())
		{
			// make sure it is a component based widget
			if (const UUserWidget* OuterWidget = Cast<UUserWidget>(PreviewWidget->GetOuter()->GetOuter());
				OuterWidget && OuterWidget->GetExtension<UWidgetComponentAsExtension>())
			{
				PreviewWidget->GetWorld()->GetTimerManager().SetTimerForNextTick(
					FTimerDelegate::CreateRaw(
						this, &FWidgetComponentEditorModule::UpdateSoftObjects, WidgetBlueprint));
			}
		}
		
		WidgetBlueprint.Reset();
		Widget.Reset();
	}
}

void FWidgetComponentEditorModule::UpdateSoftObjects(const TWeakObjectPtr<const UBaseWidgetBlueprint> InWidgetBlueprint) const
{
	CheckCondition(InWidgetBlueprint.IsValid(), return;);
	
	const UUserWidget* DefaultObject = Cast<UUserWidget>(InWidgetBlueprint->GeneratedClass->GetDefaultObject(false));
			
	WidgetComponentStatics::ForeachUserWidgetComponent(DefaultObject,
	[&](UWidgetComponentBase** ObjectMemberPtr, int32)
	{
		UWidgetComponentBase* ComponentBase = *ObjectMemberPtr;
		CheckPointer(ComponentBase, return);

		Common::PropertyHelper::IteratePropertiesOfType<FSoftObjectProperty>(ComponentBase->GetClass(), ComponentBase,
		[&] (const FProperty* InProperty, const void* InContainer, int32,
		const FString&, const FString&, const FString&, int32, int32)
		{
			const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(const_cast<FProperty*>(InProperty));
			CheckPointer(SoftObjectProperty, return);
					
			FSoftObjectPtr* SoftObjectPtr = SoftObjectProperty->GetPropertyValuePtr_InContainer(const_cast<void*>(InContainer));
			CheckPointer(SoftObjectPtr, return);

			if (SoftObjectPtr->IsNull())
			{
				return;
			}

			// refresh soft object reference if name is out dated
			if (const UObject* SoftObject = SoftObjectPtr->Get();
				SoftObject && SoftObject->GetFName() != FName(*Common::GetObjectNameFromSoftObjectPath(SoftObjectPtr->ToSoftObjectPath())))
			{
				ComponentBase->Modify();
				*SoftObjectPtr = SoftObject;
			}
		});
	});
}
