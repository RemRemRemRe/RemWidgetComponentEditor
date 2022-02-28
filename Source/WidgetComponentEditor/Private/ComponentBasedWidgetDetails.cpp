// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentBasedWidgetDetails.h"

#include "ComponentBasedWidget.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Macro/AssertionMacros.h"

#define LOCTEXT_NAMESPACE "ComponentBasedWidget"

TSharedRef<IPropertyTypeCustomization> FComponentBasedWidgetDetails::MakeInstance()
{
	return MakeShared<FComponentBasedWidgetDetails>();
}

void FComponentBasedWidgetDetails::CustomizeHeader(const TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
}

void FComponentBasedWidgetDetails::CustomizeChildren(const TSharedRef<IPropertyHandle> StructPropertyHandle,
    IDetailChildrenBuilder& DetailBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	WidgetBlueprintClass = Cast<UWidgetBlueprintGeneratedClass>(const_cast<UClass*>(StructPropertyHandle->GetOuterBaseClass()));

	CheckCondition(WidgetBlueprintClass.IsValid(), return;);

	const TSharedPtr<IPropertyHandle> ComponentsProperty = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FWidgetComponentContainer, Components));
	
	DetailBuilder.AddProperty(ComponentsProperty.ToSharedRef());

	uint32 ComponentNum;
	ComponentsProperty->GetNumChildren(ComponentNum);
	for (uint32 Index = 0; Index < ComponentNum; ++Index)
	{
		const TSharedPtr<IPropertyHandle> ComponentHandle = ComponentsProperty->GetChildHandle(Index);
		CheckCondition(ComponentHandle, continue;);

		UObject* Component;

		if (const FPropertyAccess::Result Result = ComponentHandle->GetValue(Component);
			Result != FPropertyAccess::Result::Success || !Component)
		{
			continue;
		}

		GenerateWidgetForComponent(DetailBuilder, Component, Index, ComponentHandle->GetChildHandle(0)->GetChildHandle(0));
	}
}

void FComponentBasedWidgetDetails::GenerateWidgetForComponent(IDetailChildrenBuilder& DetailBuilder, const UObject* Component,
                                                              uint32 ComponentIndex, const TSharedPtr<IPropertyHandle> ComponentHandle)
{
	uint32 ComponentPropertyNum;
	ComponentHandle->GetNumChildren(ComponentPropertyNum);
	for (uint32 Index = 0; Index < ComponentPropertyNum; ++Index)
	{
		TSharedPtr<IPropertyHandle> WidgetPropertyHandle = ComponentHandle->GetChildHandle(Index);
		CheckCondition(WidgetPropertyHandle.IsValid(), continue;);

		const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(WidgetPropertyHandle->GetProperty());
		if (!Property)
		{
			continue;
		}

		if (!Property->PropertyClass->IsChildOf<UWidget>())
		{
			continue;
		}

		WidgetPropertyHandle->MarkHiddenByCustomization();

		IDetailPropertyRow& WidgetRow = DetailBuilder.AddProperty(WidgetPropertyHandle.ToSharedRef());
		WidgetRow.CustomWidget()
		.NameContent()
		[
			WidgetPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SAssignNew(WidgetListComboButton, SComboButton)
			.ButtonStyle(FEditorStyle::Get(), "PropertyEditor.AssetComboStyle")
			.ForegroundColor(FEditorStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
			.OnGetMenuContent(this, &FComponentBasedWidgetDetails::GetPopupContent, WidgetPropertyHandle)
			.ContentPadding(2.0f)
			.IsEnabled(!WidgetPropertyHandle->IsEditConst())
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &FComponentBasedWidgetDetails::GetCurrentValueText, WidgetPropertyHandle)
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		];
	}
}

TSharedRef<SWidget> FComponentBasedWidgetDetails::GetPopupContent(const TSharedPtr<IPropertyHandle> ChildHandle)
{
	constexpr bool bInShouldCloseWindowAfterMenuSelection = true;
	constexpr bool bCloseSelfOnly = true;
	FMenuBuilder MenuBuilder(bInShouldCloseWindowAfterMenuSelection, nullptr, nullptr, bCloseSelfOnly);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("BrowseHeader", "Browse"));
	{
		SAssignNew(WidgetListView, SListView<TWeakObjectPtr<UWidget>>)
			.ListItemsSource(&ReferencableWidgets)
			.OnSelectionChanged(this, &FComponentBasedWidgetDetails::OnSelectionChanged, ChildHandle)
			.OnGenerateRow(this, &FComponentBasedWidgetDetails::OnGenerateListItem)
			.SelectionMode(ESelectionMode::Single);

		// Ensure no filter is applied at the time the menu opens
		OnFilterTextChanged(FText::GetEmpty(), ChildHandle);

		WidgetListView->SetSelection(GetCurrentValue(ChildHandle));

		const TSharedRef<SVerticalBox> MenuContent =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew(SearchBox, SSearchBox)
				.OnTextChanged(this, &FComponentBasedWidgetDetails::OnFilterTextChanged, ChildHandle)
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

FText FComponentBasedWidgetDetails::GetCurrentValueText(const TSharedPtr<IPropertyHandle> ChildHandle) const
{
	if (ChildHandle.IsValid())
	{
		switch (UObject* Object;
			ChildHandle->GetValue(Object))
		{
			case FPropertyAccess::MultipleValues:
				return LOCTEXT("MultipleValues", "Multiple Values");
			case FPropertyAccess::Success:
				if (const UWidget* Widget = Cast<UWidget>(Object))
				{
					return Widget->GetLabelText();
				}
			break;
			default:
				return FText::GetEmpty();
		}
	}

	return FText::GetEmpty();
}

void FComponentBasedWidgetDetails::OnSelectionChanged(const TWeakObjectPtr<UWidget> InItem, const ESelectInfo::Type SelectionInfo,
                                                      const TSharedPtr<IPropertyHandle> ChildHandle) const
{
	if (SelectionInfo != ESelectInfo::Direct)
	{
		if (ChildHandle.IsValid())
		{
			const FString ObjectPathName = InItem.IsValid() ? InItem->GetPathName() : TEXT("None");
			CheckCondition(ChildHandle->SetValueFromFormattedString(ObjectPathName) == FPropertyAccess::Result::Success);

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
				.Text(Widget->GetLabelText())
			];
	}

	return SNew(STableRow<TWeakObjectPtr<UWidget>>, OwnerTable);
}

UWidget* FComponentBasedWidgetDetails::GetCurrentValue(const TSharedPtr<IPropertyHandle> ChildHandle) const
{
	if (ChildHandle.IsValid())
	{
		switch (UObject* Object;
			ChildHandle->GetValue(Object))
		{
			case FPropertyAccess::Success:
				return Cast<UWidget>(Object);
			default:
				break;
		}
	}

	return {};
}

void FComponentBasedWidgetDetails::OnFilterTextChanged(const FText& InFilterText, const TSharedPtr<IPropertyHandle> ChildHandle)
{
	if (ChildHandle.IsValid() && WidgetBlueprintClass.IsValid())
	{
		TArray<UWidget*> AllWidgets;
		WidgetBlueprintClass.Get()->GetWidgetTreeArchetype()->GetAllWidgets(AllWidgets);

		AllWidgets.Sort([](const UWidget& Lhs, const UWidget& Rhs) {
			return Lhs.GetLabelText().CompareTo(Rhs.GetLabelText()) < 0;
		});

		const FObjectPropertyBase* ObjectProperty = CastFieldChecked<FObjectPropertyBase>(ChildHandle->GetProperty());
		const UClass* FilterWidgetClass = ObjectProperty->PropertyClass;
			
		ReferencableWidgets.Reset();
			
		for (UWidget* Widget : AllWidgets)
		{
			if (Widget->bIsVariable == false && Widget->GetDisplayLabel().IsEmpty())
			{
				continue;
			}

			if (!Widget->IsA(FilterWidgetClass))
			{
				continue;
			}

			if (const FString& CurrentFilterText = InFilterText.ToString();
				CurrentFilterText.IsEmpty() ||
				Widget->GetClass()->GetName().Contains(CurrentFilterText) ||
				Widget->GetDisplayLabel().Contains(CurrentFilterText))
			{
				ReferencableWidgets.Add(Widget);
			}
		}
			
		WidgetListView->RequestListRefresh();
	}
}

#undef LOCTEXT_NAMESPACE
