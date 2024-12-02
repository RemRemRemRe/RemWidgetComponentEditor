// Copyright RemRemRemRe, All Rights Reserved.


#include "RemComponentBasedWidgetDetails.h"

#include "BaseWidgetBlueprint.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "RemEditorUtilitiesStatics.h"
#include "RemEditorUtilitiesStatics.inl"
#include "RemWidgetComponentAsExtension.h"
#include "WidgetBlueprintEditor.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Macro/RemAssertionMacros.h"
#include "Object/RemObjectStatics.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "ComponentBasedWidget"

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

	const FPropertyCustomizationFunctor Predicate =
	[this] (const TSharedPtr<IPropertyHandle>& Handle, FDetailWidgetRow& WidgetPropertyRow,
		const Rem::Enum::EContainerCombination MemberContainerType)
	{
		MakeCustomWidgetForProperty(Handle, WidgetPropertyRow, MemberContainerType,
		[this] (const TSharedPtr<IPropertyHandle>& WidgetPropertyHandle)
		{
			return MakeComboButton(WidgetPropertyHandle);
		});
	};

	GenerateWidgetForContainerContent<FSoftObjectProperty, UWidget>
	(PropertyHandle, ComponentsGroup, Predicate, Rem::Enum::EContainerCombination::ContainerItself);
}

TSharedRef<SWidget> FRemComponentBasedWidgetDetails::MakeComboButton(const TSharedPtr<IPropertyHandle>& PropertyHandle)
{
	using namespace Rem::Editor;

	const TSharedPtr<SComboButton> ComboButton = SNew(SComboButton)
		.ButtonStyle(FAppStyle::Get(), AssetComboStyleName)
		.ForegroundColor(FAppStyle::GetColor(AssetNameColorName))
		.ContentPadding(2.0f)
		.IsEnabled(!PropertyHandle->IsEditConst())
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateLambda([PropertyHandle]
			{
				return GetCurrentValueText<TSoftObjectPtr<UWidget>>(PropertyHandle,
				[](const TSoftObjectPtr<UWidget>& Widget)
				{
					return GetWidgetName(Widget);
				});
			}))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];

	ComboButton->SetOnGetMenuContent(FOnGetContent::CreateSP(
		this, &FRemComponentBasedWidgetDetails::GetPopupContent, PropertyHandle, ComboButton));

	return ComboButton.ToSharedRef();
}

// ReSharper disable CppPassValueParameterByConstReference
TSharedRef<SWidget> FRemComponentBasedWidgetDetails::GetPopupContent(const TSharedPtr<IPropertyHandle> ChildHandle,
	const TSharedPtr<SComboButton> WidgetListComboButton)
// ReSharper restore CppPassValueParameterByConstReference
{
	using namespace Rem::Editor;

	constexpr bool bInShouldCloseWindowAfterMenuSelection = true;
	constexpr bool bCloseSelfOnly = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr, nullptr, bCloseSelfOnly);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView =
		SNew(SListView<TWeakObjectPtr<UWidget>>)
			.ListItemsSource(&ReferencableWidgets)
			.OnSelectionChanged(this, &FRemComponentBasedWidgetDetails::OnSelectionChanged, ChildHandle, WidgetListComboButton)
			.OnGenerateRow(this, &FRemComponentBasedWidgetDetails::OnGenerateListItem)
			.SelectionMode(ESelectionMode::Single);

		// Ensure no filter is applied at the time the menu opens
		OnFilterTextChanged(FText::GetEmpty(), ChildHandle, WidgetListView);

		int32 Result;
		WidgetListView->SetSelection(GetCurrentValue<UWidget*>(ChildHandle, Result));

		TSharedPtr<SSearchBox> SearchBox;

		const auto MenuContent =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(SearchBox, SSearchBox)
				.OnTextChanged(this, &FRemComponentBasedWidgetDetails::OnFilterTextChanged, ChildHandle, WidgetListView)
			]

			+ SVerticalBox::Slot()
			.Padding(0, 2.0f, 0, 0)
			.AutoHeight()
			[
				SNew(SBox)
				.MaxDesiredHeight(300.0f)
				[
					WidgetListView.ToSharedRef()
				]
			];

		MenuBuilder.AddWidget(MenuContent, FText::GetEmpty(), true);

		WidgetListComboButton->SetMenuContentWidgetToFocus(SearchBox);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

// ReSharper disable CppPassValueParameterByConstReference
void FRemComponentBasedWidgetDetails::OnSelectionChanged(const TWeakObjectPtr<UWidget> InItem, const ESelectInfo::Type SelectionInfo,
	const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SComboButton> WidgetListComboButton) const
// ReSharper restore CppPassValueParameterByConstReference
{
	using namespace Rem::Editor;

	if (SelectionInfo != ESelectInfo::Direct)
	{
		// value should set successfully
		RemCheckCondition(SetObjectValue(InItem.Get(), ChildHandle));

		WidgetListComboButton->SetIsOpen(false);
	}
}

TSharedRef<ITableRow> FRemComponentBasedWidgetDetails::OnGenerateListItem(const TWeakObjectPtr<UWidget> InItem,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	using namespace Rem::Editor;

	if (const auto* Widget = InItem.Get())
	{
		return
			SNew(STableRow<TWeakObjectPtr<UWidget>>, OwnerTable)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(GetWidgetName(TSoftObjectPtr<const UWidget>(Widget)))
			];
	}

	return SNew(STableRow<TWeakObjectPtr<UWidget>>, OwnerTable);
}

// ReSharper disable CppPassValueParameterByConstReference
void FRemComponentBasedWidgetDetails::OnFilterTextChanged(const FText& InFilterText,
	const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView)
// ReSharper restore CppPassValueParameterByConstReference
{
	if (ChildHandle.IsValid() && WidgetBlueprintGeneratedClass.IsValid())
	{
		// ONLY use UBaseWidgetBlueprint::WidgetTree
		// rather than	UUserWidget::WidgetTree
		// or			UWidgetBlueprintGeneratedClass::WidgetTree (the widget class version)
		// could make the drop-down list up to date, (after rename a widget)
		TArray<UWidget*> AllWidgets;
		Cast<UBaseWidgetBlueprint>(WidgetBlueprintGeneratedClass.Get()->ClassGeneratedBy)->WidgetTree->GetAllWidgets(AllWidgets);

		AllWidgets.Sort([](const UWidget& Lhs, const UWidget& Rhs) {
			return Lhs.GetLabelText().CompareTo(Rhs.GetLabelText()) < 0;
		});

		const auto* ObjectProperty = CastField<FObjectPropertyBase>(ChildHandle->GetProperty());
		if (!ObjectProperty)
		{
			return;
		}

		const UClass* FilterWidgetClass = ObjectProperty->PropertyClass;

		ReferencableWidgets.Reset();

		for (UWidget* Widget : AllWidgets)
		{
			if (!Widget->IsA(FilterWidgetClass))
			{
				continue;
			}

			if (const auto& CurrentFilterString = InFilterText.ToString();
				CurrentFilterString.IsEmpty() ||
				Widget->GetName().Contains(CurrentFilterString) ||
				Widget->GetDisplayLabel().Contains(CurrentFilterString) ||
				Widget->GetClass()->GetName().Contains(CurrentFilterString)
				)
			{
				ReferencableWidgets.Add(Widget);
			}
		}

		WidgetListView->RequestListRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
