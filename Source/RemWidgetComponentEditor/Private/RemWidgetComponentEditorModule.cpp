// Copyright RemRemRemRe, All Rights Reserved.

#include "RemWidgetComponentEditorModule.h"

#include "BaseWidgetBlueprint.h"
#include "PropertyEditorModule.h"
#include "RemComponentBasedWidgetDetails.h"
#include "RemWidgetComponentAsExtension.h"
#include "RemWidgetComponentBase.h"
#include "RemWidgetComponentEditorSetting.h"
#include "RemWidgetComponentStatics.h"
#include "Blueprint/UserWidget.h"
#include "Macro/RemAssertionMacros.h"
#include "Blueprint/WidgetTree.h"
#include "Object/RemObjectStatics.h"
#include "Templates/RemIteratePropertiesOfType.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "UObject/Package.h"

class FRemWidgetComponentEditorModule : public IRemWidgetComponentEditorModule
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

IMPLEMENT_MODULE(FRemWidgetComponentEditorModule, RemWidgetComponentEditor)

void FRemWidgetComponentEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
	IRemWidgetComponentEditorModule::StartupModule();

	RegisterCustomization();

	OnObjectReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(this, &FRemWidgetComponentEditorModule::OnObjectReplaced);
	ObjectModifiedHandle = FCoreUObjectDelegates::OnObjectModified.AddRaw(this, &FRemWidgetComponentEditorModule::OnObjectModified);
}

void FRemWidgetComponentEditorModule::ShutdownModule()
{
	FCoreUObjectDelegates::OnObjectModified.Remove(ObjectModifiedHandle);
	FCoreUObjectDelegates::OnObjectsReplaced.Remove(OnObjectReplacedHandle);

	UnregisterCustomization();

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	IRemWidgetComponentEditorModule::ShutdownModule();
}

void FRemWidgetComponentEditorModule::RegisterCustomization()
{
	// Register customizations
	auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	bool bSignificantlyChanged = false;
	auto* RemWidgetComponentEditorSetting = GetMutableDefault<URemWidgetComponentEditorSetting>();
	for (const auto& Class : RemWidgetComponentEditorSetting->GetWidgetClassToCustomize())
	{
		if (Class.IsNull())
		{
			continue;
		}

		const auto& AssetName = RegisteredClasses.Add_GetRef(*Class.GetAssetName());

		PropertyModule.RegisterCustomClassLayout(AssetName,
			FOnGetDetailCustomizationInstance::CreateStatic(&FRemComponentBasedWidgetDetails::MakeInstance));

		bSignificantlyChanged = true;
	}

	if (bSignificantlyChanged)
	{
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	if (!SettingChangedHandle.IsValid())
	{
		SettingChangedHandle = RemWidgetComponentEditorSetting->OnSettingChanged().AddRaw(
			this, &FRemWidgetComponentEditorModule::OnSettingChanged);
	}
}

void FRemWidgetComponentEditorModule::UnregisterCustomization()
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

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void FRemWidgetComponentEditorModule::OnSettingChanged(UObject* Settings, FPropertyChangedEvent&)
{
	if (Settings)
	{
		UnregisterCustomization();

		RegisterCustomization();
	}
}

static FAutoConsoleVariable CVarRemWidgetComponentEditorFixIncorrectComponentClass(
	TEXT("RemWidgetComponentEditor.FixIncorrectComponentClass"), true,
		TEXT("Trying to fix incorrect component class when component blueprint recompiled."));

// ReSharper disable once CppMemberFunctionMayBeStatic
void FRemWidgetComponentEditorModule::OnObjectReplaced(const TMap<UObject*, UObject*>& ReplacementObjectMap) const
{
	if (!CVarRemWidgetComponentEditorFixIncorrectComponentClass->GetBool())
	{
		return;
	}

	TArray<UObject*> Components;
	GetObjectsOfClass(URemWidgetComponentBase::StaticClass(), Components,
		true);

	for (const auto* Object : Components)
	{
		if (!IsValid(Object->GetOuter()))
		{
			continue;
		}

		const auto* Blueprint = Cast<UBaseWidgetBlueprint>(Object->GetOuter()->GetClass()->ClassGeneratedBy);
		if (!Blueprint)
		{
			continue;
		}

		const auto& GeneratedClass = Blueprint->GeneratedClass;
		if (!GeneratedClass)
		{
			continue;
		}

		auto* OwnerWidgetCDO = Cast<UUserWidget>(GeneratedClass->GetDefaultObject(false));
		if (!OwnerWidgetCDO)
		{
			continue;
		}

		// Remove all nullptr from UUserWidget::Extensions
		OwnerWidgetCDO->RemoveExtension(nullptr);

		const auto* Extension = OwnerWidgetCDO->GetExtension<URemWidgetComponentAsExtension>();
		if (!Extension)
		{
			continue;
		}

		Rem::WidgetComponent::ForeachUserWidgetComponent(Extension,
[&](URemWidgetComponentBase** MemberPtr, int32)
		{
			RemCheckVariable(MemberPtr, return);
			auto* OldObject = *MemberPtr;

			RemCheckVariable(OldObject, return);

			if (const auto* const* ReplacementClassPtr =  ReplacementObjectMap.Find(OldObject->GetClass()))
			{
				if (const auto* ReplacementClass = Cast<UClass>(*ReplacementClassPtr))
				{
					const auto SavedName = OldObject->GetFName();

					OldObject->Rename(nullptr, GetTransientPackage(),
				REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);

					auto* NewComponent = NewObject<URemWidgetComponentBase>(OwnerWidgetCDO,
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

static FAutoConsoleVariable CVarRemWidgetComponentEditorFixSoftObjectNotUpdated(
	TEXT("RemWidgetComponentEditor.FixSoftObjectNotUpdated"), true,
		TEXT("Trying to fix soft object reference did not get updated after the widget get renamed."));

void FRemWidgetComponentEditorModule::OnObjectModified(UObject* Object)
{
	if (!CVarRemWidgetComponentEditorFixSoftObjectNotUpdated->GetBool())
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
		if (auto* Blueprint = Cast<UBaseWidgetBlueprint>(Object))
		{
			SavedFrameCounter = GFrameCounter;
			WidgetBlueprint = Blueprint;
		}
	}
	else if (!Object->IsA<UWidget>())
	{
		// skip modified call back of root widget etc...
	}
	else if (!Widget.IsValid())
	{
		if (const auto* WidgetObject = Cast<UWidget>(Object);
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
		if (const auto* PreviewWidget = Cast<UWidget>(Object);
			SavedFrameCounter == GFrameCounter
			&& PreviewWidget && PreviewWidget->IsDesignTime())
		{
			// make sure it is a component based widget
			if (const auto* OuterWidget = Cast<UUserWidget>(PreviewWidget->GetOuter()->GetOuter());
				OuterWidget && OuterWidget->GetExtension<URemWidgetComponentAsExtension>())
			{
				Rem::Object::SetTimerForThisTick(*PreviewWidget, FTimerDelegate::CreateWeakLambda(WidgetBlueprint.Get(), [this]
				{
					UpdateSoftObjects(WidgetBlueprint);
				}));
			}
		}

		WidgetBlueprint.Reset();
		Widget.Reset();
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void FRemWidgetComponentEditorModule::UpdateSoftObjects(const TWeakObjectPtr<const UBaseWidgetBlueprint> InWidgetBlueprint) const
{
	RemCheckCondition(InWidgetBlueprint.IsValid(), return;);

	const auto* DefaultObject = Cast<UUserWidget>(InWidgetBlueprint->GeneratedClass->GetDefaultObject(false));

	Rem::WidgetComponent::ForeachUserWidgetComponent(DefaultObject,
	[&](URemWidgetComponentBase** ObjectMemberPtr, int32)
	{
		auto* ComponentBase = *ObjectMemberPtr;
		RemCheckVariable(ComponentBase, return);

		Rem::Property::IteratePropertiesOfType<FSoftObjectProperty>(ComponentBase->GetClass(), ComponentBase,
		[&] (const FProperty* InProperty, const void* InContainer)
		{
			auto* SoftObjectProperty = CastField<const FSoftObjectProperty>(InProperty);
			RemCheckVariable(SoftObjectProperty, return);

			auto* SoftObjectPtr = SoftObjectProperty->GetPropertyValuePtr_InContainer(const_cast<void*>(InContainer));
			RemCheckVariable(SoftObjectPtr, return);

			if (SoftObjectPtr->IsNull())
			{
				return;
			}

			// refresh soft object reference if name is outdated
			if (const auto* SoftObject = SoftObjectPtr->Get();
				SoftObject && SoftObject->GetFName() != FName(*Rem::GetObjectNameFromSoftObjectPath(SoftObjectPtr->ToSoftObjectPath())))
			{
				ComponentBase->Modify();
				*SoftObjectPtr = SoftObject;
			}
		});
	});
}
