// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentBasedWidgetDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "DetailCustomizationUtilities.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "WidgetComponentBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Macro/AssertionMacros.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "ComponentBasedWidget"

TSharedRef<IDetailCustomization> FComponentBasedWidgetDetails::MakeInstance()
{
	return MakeShared<FComponentBasedWidgetDetails>();
}

void FComponentBasedWidgetDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	IDetailCustomization::CustomizeDetails(DetailBuilder);
	
	// cache widget blueprint class
	WidgetBlueprintClass = Cast<UWidgetBlueprintGeneratedClass>(DetailBuilder->GetBaseClass());

	// generate array header widget
	const FName ComponentsPropertyName = TEXT("Components");
	const TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder->GetProperty(ComponentsPropertyName);
	IDetailCategoryBuilder& ComponentsCategory = DetailBuilder->EditCategory(PropertyHandle->GetDefaultCategoryName());

	const FSimpleDelegate OnComponentsChanged = FSimpleDelegate::CreateLambda(
		[DetailBuilderWeakPtr = TWeakPtr<IDetailLayoutBuilder>(DetailBuilder)]
	{
		if (DetailBuilderWeakPtr.IsValid())
		{
			DetailBuilderWeakPtr.Pin()->ForceRefreshDetails();
		}
	});
	
	IDetailGroup& ComponentsGroup = GenerateContainerHeader(
		PropertyHandle, ComponentsCategory, OnComponentsChanged);

	auto Predicate = [this]
	(const TSharedPtr<IPropertyHandle> Handle, IDetailPropertyRow& WidgetPropertyRow,
		const EMemberContainerType MemberContainerType)
	{
		MakeCustomWidget(Handle, WidgetPropertyRow, MemberContainerType);
	};
	
	GenerateWidgetForContainerContent<FSoftObjectProperty>
	(PropertyHandle, ComponentsGroup, Predicate, EMemberContainerType::NotAContainer);
}

void FComponentBasedWidgetDetails::MakeCustomWidget(const TSharedPtr<IPropertyHandle> PropertyHandle,
	IDetailPropertyRow& WidgetPropertyRow, const EMemberContainerType MemberContainerType)
{
	const TSharedPtr<SComboButton> ComboButton = SNew(SComboButton)
	.ButtonStyle(FEditorStyle::Get(), "PropertyEditor.AssetComboStyle")
	.ForegroundColor(FEditorStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
	.ContentPadding(2.0f)
	.IsEnabled(!PropertyHandle->IsEditConst())
	.ButtonContent()
	[
		SNew(STextBlock)
		.Text(TAttribute<FText>::CreateLambda([=]
		{
			return GetCurrentValueText<UWidget>(PropertyHandle,
			[](const UWidget* Widget)
			{
				return GetWidgetName(Widget);
			});
		}))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	];

	ComboButton->SetOnGetMenuContent(FOnGetContent::CreateSP(
		this, &FComponentBasedWidgetDetails::GetPopupContent, PropertyHandle, ComboButton));
		
	WidgetPropertyRow.CustomWidget()
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		ComboButton.ToSharedRef()
	];
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
		WidgetListView->SetSelection(GetCurrentValue<UWidget>(ChildHandle, Result));

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
		if (ChildHandle.IsValid())
		{
			TArray<FString> References;
			for (int32 Index = 0; Index < ChildHandle->GetNumPerObjectValues(); Index++)
			{
				References.Add(InItem.Get()->GetPathName());
			}

			// PPF_ParsingDefaultProperties is needed to set value from CDO, but that is hard coded
			// @see FPropertyValueImpl::ImportText @line 402 FPropertyTextUtilities::PropertyToTextHelper
			CheckCondition(ChildHandle->SetPerObjectValues(References) == FPropertyAccess::Result::Success);

			WidgetListComboButton->SetIsOpen(false);
		}
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
				.Text(GetWidgetName(Widget))
			];
	}

	return SNew(STableRow<TWeakObjectPtr<UWidget>>, OwnerTable);
}

void FComponentBasedWidgetDetails::OnFilterTextChanged(const FText& InFilterText,
	const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView)
{
	if (ChildHandle.IsValid() && WidgetBlueprintClass.IsValid())
	{
		TArray<UWidget*> AllWidgets;
		WidgetBlueprintClass.Get()->GetWidgetTreeArchetype()->GetAllWidgets(AllWidgets);

		AllWidgets.Sort([](const UWidget& Lhs, const UWidget& Rhs) {
			return Lhs.GetLabelText().CompareTo(Rhs.GetLabelText()) < 0;
		});

		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(ChildHandle->GetProperty());
		//CheckPointer(ObjectProperty, return;);
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
