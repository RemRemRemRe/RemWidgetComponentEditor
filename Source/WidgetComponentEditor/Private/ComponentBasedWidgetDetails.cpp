// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentBasedWidgetDetails.h"

#include "BaseWidgetBlueprint.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCustomizationUtilities.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "IPropertyUtilities.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Macro/AssertionMacros.h"
#include "ObjectEditorUtils.h"
#include "PropertyCustomizationHelpers.h"
#include "WidgetComponentAsExtension.h"

#define LOCTEXT_NAMESPACE "ComponentBasedWidget"

TSharedRef<IDetailCustomization> FComponentBasedWidgetDetails::MakeInstance()
{
	return MakeShared<FComponentBasedWidgetDetails>();
}

void FComponentBasedWidgetDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	IDetailCustomization::CustomizeDetails(DetailBuilder);

	TArray< TWeakObjectPtr<UObject> > ObjectsBeingCustomized;
	DetailBuilder->GetObjectsBeingCustomized(ObjectsBeingCustomized);
	CheckCondition(ObjectsBeingCustomized.Num() > 0, return;);

	UUserWidget* WidgetObject = Cast<UUserWidget>(ObjectsBeingCustomized[0].Get());
	CheckPointer(WidgetObject, return;);

	const UWidgetComponentAsExtension* Extension = WidgetObject->GetExtension<UWidgetComponentAsExtension>();
	if (!Extension)
	{
		return;
	}

	FArrayProperty* ComponentsProperty = Extension->GetComponentsProperty();
	CheckPointer(ComponentsProperty, return;);

	// cache widget blueprint class
	WidgetBlueprintGeneratedClass = Cast<UWidgetBlueprintGeneratedClass>(DetailBuilder->GetBaseClass());

	// generate array header widget
	const TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder->GetProperty(*GetPropertyPath(ComponentsProperty));
	CheckCondition(PropertyHandle.IsValid(), return;);
	CheckCondition(PropertyHandle->IsValidHandle(), return;);

	IDetailCategoryBuilder& ComponentsCategory = DetailBuilder->EditCategory(PropertyHandle->GetDefaultCategoryName());

	const FSimpleDelegate OnComponentsChanged = FSimpleDelegate::CreateLambda(
		[DetailBuilderWeakPtr = TWeakPtr<IDetailLayoutBuilder>(DetailBuilder)]
	{
		if (DetailBuilderWeakPtr.IsValid())
		{
			const TSharedRef< class IPropertyUtilities > PropertyUtilities = DetailBuilderWeakPtr.Pin()->GetPropertyUtilities();
			PropertyUtilities.Get().ForceRefresh();
		}
	});

	IDetailGroup& ComponentsGroup = GenerateContainerHeader(
		PropertyHandle, ComponentsCategory, OnComponentsChanged);

	const FPropertyCustomizationFunctor Predicate =
	[this] (const TSharedPtr<IPropertyHandle> Handle, FDetailWidgetRow& WidgetPropertyRow,
		const EContainerCombination MemberContainerType)
	{
		MakeCustomWidgetForProperty(Handle, WidgetPropertyRow, MemberContainerType,
		[this] (const TSharedPtr<IPropertyHandle> WidgetPropertyHandle)
		{
			return MakeComboButton(WidgetPropertyHandle);
		});
	};

	GenerateWidgetForContainerContent<FSoftObjectProperty, UWidget>
	(PropertyHandle, ComponentsGroup, Predicate, EContainerCombination::ContainerItself);
}

TSharedRef<SWidget> FComponentBasedWidgetDetails::MakeComboButton(const TSharedPtr<IPropertyHandle> PropertyHandle)
{
	const TSharedPtr<SComboButton> ComboButton = SNew(SComboButton)
		.ButtonStyle(FAppStyle::Get(), AssetComboStyleName)
		.ForegroundColor(FAppStyle::GetColor(AssetNameColorName))
		.ContentPadding(2.0f)
		.IsEnabled(!PropertyHandle->IsEditConst())
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(TAttribute<FText>::CreateLambda([=]
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
		this, &FComponentBasedWidgetDetails::GetPopupContent, PropertyHandle, ComboButton));

	return ComboButton.ToSharedRef();
}

TSharedRef<SWidget> FComponentBasedWidgetDetails::GetPopupContent(const TSharedPtr<IPropertyHandle> ChildHandle,
	const TSharedPtr<SComboButton> WidgetListComboButton)
{
	constexpr bool bInShouldCloseWindowAfterMenuSelection = true;
	constexpr bool bCloseSelfOnly = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr, nullptr, bCloseSelfOnly);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView =
		SNew(SListView<TWeakObjectPtr<UWidget>>)
			.ListItemsSource(&ReferencableWidgets)
			.OnSelectionChanged(this, &FComponentBasedWidgetDetails::OnSelectionChanged, ChildHandle, WidgetListComboButton)
			.OnGenerateRow(this, &FComponentBasedWidgetDetails::OnGenerateListItem)
			.SelectionMode(ESelectionMode::Single);

		// Ensure no filter is applied at the time the menu opens
		OnFilterTextChanged(FText::GetEmpty(), ChildHandle, WidgetListView);

		int32 Result;
		WidgetListView->SetSelection(GetCurrentValue<UWidget*>(ChildHandle, Result));

		TSharedPtr<SSearchBox> SearchBox;

		const TSharedRef<SVerticalBox> MenuContent =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(SearchBox, SSearchBox)
				.OnTextChanged(this, &FComponentBasedWidgetDetails::OnFilterTextChanged, ChildHandle, WidgetListView)
			]

			+ SVerticalBox::Slot()
			.Padding(0, 2.0f, 0, 0)
			.AutoHeight()
			[
				SNew(SBox)
				.MaxDesiredHeight(300)
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

void FComponentBasedWidgetDetails::OnSelectionChanged(const TWeakObjectPtr<UWidget> InItem, const ESelectInfo::Type SelectionInfo,
	const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SComboButton> WidgetListComboButton) const
{
	if (SelectionInfo != ESelectInfo::Direct)
	{
		// value should set successfully
		CheckCondition(SetObjectValue(InItem.Get(), ChildHandle));

		WidgetListComboButton->SetIsOpen(false);
	}
}

TSharedRef<ITableRow> FComponentBasedWidgetDetails::OnGenerateListItem(const TWeakObjectPtr<UWidget> InItem,
                                                                       const TSharedRef<STableViewBase>& OwnerTable) const
{
	if (const UWidget* Widget = InItem.Get())
	{
		return
			SNew(STableRow<TWeakObjectPtr<UWidget>>, OwnerTable)
			[
				SNew(STextBlock)
				.Font(IDetailLayoutBuilder::GetDetailFont())
				.Text(GetWidgetName(TSoftObjectPtr<UWidget>(Widget)))
			];
	}

	return SNew(STableRow<TWeakObjectPtr<UWidget>>, OwnerTable);
}

void FComponentBasedWidgetDetails::OnFilterTextChanged(const FText& InFilterText,
	const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView)
{
	if (ChildHandle.IsValid() && WidgetBlueprintGeneratedClass.IsValid())
	{
		// ONLY use UBaseWidgetBlueprint::WidgetTree
		// rather than	UUserWidget::WidgetTree
		// or			UWidgetBlueprintGeneratedClass::WidgetTree (the widget class version)
		// could make the drop down list up to date, (after rename a widget)
		TArray<UWidget*> AllWidgets;
		Cast<UBaseWidgetBlueprint>(WidgetBlueprintGeneratedClass.Get()->ClassGeneratedBy)->WidgetTree->GetAllWidgets(AllWidgets);

		AllWidgets.Sort([](const UWidget& Lhs, const UWidget& Rhs) {
			return Lhs.GetLabelText().CompareTo(Rhs.GetLabelText()) < 0;
		});

		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ChildHandle->GetProperty());
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

			if (const FString& CurrentFilterText = InFilterText.ToString();
				CurrentFilterText.IsEmpty() ||
				Widget->GetName().Contains(CurrentFilterText) ||
				Widget->GetDisplayLabel().Contains(CurrentFilterText) ||
				Widget->GetClass()->GetName().Contains(CurrentFilterText)
				)
			{
				ReferencableWidgets.Add(Widget);
			}
		}

		WidgetListView->RequestListRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
