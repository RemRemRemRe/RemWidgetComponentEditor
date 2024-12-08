// Copyright RemRemRemRe, All Rights Reserved.


#include "RemComponentBasedWidgetDetails.h"

#include "BaseWidgetBlueprint.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "RemEditorUtilitiesComboButton.inl"
#include "RemEditorUtilitiesStatics.inl"
#include "RemWidgetComponentAsExtension.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Macro/RemAssertionMacros.h"
#include "Object/RemObjectStatics.h"
#include "WidgetBlueprintEditor.h"

TSharedRef<IDetailCustomization> FRemComponentBasedWidgetDetails::MakeInstance()
{
	return MakeShared<FRemComponentBasedWidgetDetails>();
}

void FRemComponentBasedWidgetDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	using namespace Rem::Editor;

	IDetailCustomization::CustomizeDetails(DetailBuilder);

	TArray< TWeakObjectPtr<UObject> > ObjectsBeingCustomized;
	DetailBuilder->GetObjectsBeingCustomized(ObjectsBeingCustomized);
	RemCheckCondition(ObjectsBeingCustomized.Num() > 0, return;);

	auto* WidgetObject = Cast<UUserWidget>(ObjectsBeingCustomized[0].Get());
	RemCheckVariable(WidgetObject, return;);

	const auto* Extension = WidgetObject->GetExtension<URemWidgetComponentAsExtension>();
	if (!Extension)
	{
		return;
	}

	auto* ComponentsProperty = Extension->GetComponentsProperty();
	RemCheckVariable(ComponentsProperty, return;);

	// cache widget blueprint class
	WidgetBlueprintGeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(DetailBuilder->GetBaseClass());

	// generate array header widget
	const auto PropertyHandle = DetailBuilder->GetProperty(*GetPropertyPath(ComponentsProperty));
	RemCheckCondition(PropertyHandle->IsValidHandle(), return;);

	auto& ComponentsCategory = DetailBuilder->EditCategory(PropertyHandle->GetDefaultCategoryName());

	const auto OnComponentsChanged = FSimpleDelegate::CreateLambda(
		[WidgetObject, this]
	{
		Rem::Object::SetTimerForThisTick(*WidgetObject, FTimerDelegate::CreateWeakLambda(WidgetObject,
			[this, WidgetObject]
		{
			auto* WidgetBlueprintEditor = GetAssetEditorInstance<FWidgetBlueprintEditor>(WidgetObject->GetClass());
			RemCheckVariable(WidgetBlueprintEditor, return;);

			WidgetBlueprintEditor->RefreshPreview();
			WidgetBlueprintEditor->SelectObjects({WidgetBlueprintEditor->GetPreview()});
		}));
	});

	auto& ComponentsGroup = GenerateContainerHeader(
		PropertyHandle, ComponentsCategory, OnComponentsChanged);

	const auto Predicate = [this] (const TSharedRef<IPropertyHandle>& Handle, FDetailWidgetRow& WidgetPropertyRow,
		const Rem::Enum::EContainerCombination MemberContainerType)
	{
		MakeCustomWidgetForProperty(Handle, WidgetPropertyRow, MemberContainerType,
		[this] (const TSharedRef<IPropertyHandle>& WidgetPropertyHandle)
		{
			return MakeComboButton(WidgetPropertyHandle,
				[this, WidgetPropertyHandle] (TSharedRef<SComboButton>& ComboButton)
				{
					return FOnGetContent::CreateStatic(&GetPopupContent<FListViewItemType>,
						ComboButton,
						&ListViewItems,
						SListView<FListViewItemType>::FOnSelectionChanged::CreateLambda(
						[WidgetPropertyHandle, ComboButton](const FListViewItemType InItem, const ESelectInfo::Type SelectionInfo)
						{
							using namespace Rem::Editor;

							if (SelectionInfo != ESelectInfo::Direct)
							{
								// value should set successfully
								RemCheckCondition(SetObjectValue(InItem.Get(), WidgetPropertyHandle));

								ComboButton->SetIsOpen(false);
							}
						}),
						SListView<FListViewItemType>::FOnGenerateRow::CreateStatic(&OnGenerateListItem<FListViewItemType>,
						[] (const FListViewItemType& Item) -> FText
						{
							using namespace Rem::Editor;
							return GetWidgetName(Item.Get());
						}),
						[WidgetPropertyHandle]() -> FListViewItemType
						{
							return FListViewItemType{GetCurrentValue<TSoftObjectPtr<UWidget>>(WidgetPropertyHandle).Get()};
						},
						[this, WidgetPropertyHandle](TSharedRef<SListView<FListViewItemType>> ListView)
						{
							return FOnTextChanged::CreateRaw(this, &ThisClass::OnFilterTextChanged, WidgetPropertyHandle, ListView);
						}
					);
				},
				TAttribute<FText>::CreateLambda([WidgetPropertyHandle]() -> FText
				{
					FPropertyAccess::Result Result;
					auto Value = GetCurrentValue<TSoftObjectPtr<UWidget>>(WidgetPropertyHandle, Result);

					return TryGetText(Result,
					[Value]
					{
						return GetWidgetName(Value);
					});
				}));
		});
	};

	GenerateWidgetForContainerContent<FSoftObjectProperty, UWidget>
	(PropertyHandle, ComponentsGroup, Predicate, Rem::Enum::EContainerCombination::ContainerItself);
}

void FRemComponentBasedWidgetDetails::OnFilterTextChanged(const FText& InFilterText,
	const TSharedRef<IPropertyHandle> FilterTextPropertyHandle, const TSharedRef<SListView<FListViewItemType>> WidgetListView)
{
	if (auto* GeneratedClass = WidgetBlueprintGeneratedClass.Get())
	{
		// ONLY use UBaseWidgetBlueprint::WidgetTree
		// rather than	UUserWidget::WidgetTree
		// or			UWidgetBlueprintGeneratedClass::WidgetTree (the widget class version)
		// could make the drop-down list up to date, (after rename a widget)
		TArray<UWidget*> AllWidgets;
		Cast<UBaseWidgetBlueprint>(GeneratedClass->ClassGeneratedBy)->WidgetTree->GetAllWidgets(AllWidgets);

		AllWidgets.Sort([](const UWidget& Lhs, const UWidget& Rhs) {
			return Lhs.GetLabelText().CompareTo(Rhs.GetLabelText()) < 0;
		});

		const auto* ObjectProperty = CastField<FObjectPropertyBase>(FilterTextPropertyHandle->GetProperty());
		if (!ObjectProperty)
		{
			return;
		}

		const UClass* FilterWidgetClass = ObjectProperty->PropertyClass;

		ListViewItems.Reset();

		const auto& CurrentFilterString = InFilterText.ToString();

		for (auto* Widget : AllWidgets)
		{
			if (!Widget || !Widget->IsA(FilterWidgetClass))
			{
				continue;
			}

			if (CurrentFilterString.IsEmpty() ||
				Widget->GetName().Contains(CurrentFilterString) ||
				Widget->GetDisplayLabel().Contains(CurrentFilterString) ||
				Widget->GetClass()->GetName().Contains(CurrentFilterString)
				)
			{
				ListViewItems.Add(Widget);
			}
		}

		ListViewItems.Shrink();
		WidgetListView->RequestListRefresh();
	}
}
