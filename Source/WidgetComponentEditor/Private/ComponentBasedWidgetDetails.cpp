// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentBasedWidgetDetails.h"

#include "ComponentBasedWidget.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
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
	
	const FName ComponentsGroupName = TEXT("Components");
	IDetailGroup& ComponentsGroup = DetailBuilder.AddGroup(ComponentsGroupName, FText::FromName(ComponentsGroupName));
	ComponentsGroup.HeaderProperty(ComponentsProperty.ToSharedRef());

	// refresh detail view when value changed, element added, deleted, inserted, etc...
	ComponentsProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateSP(this,
		&FComponentBasedWidgetDetails::ForceRefreshDetails, &DetailBuilder));
	
	uint32 ComponentNum;
	ComponentsProperty->GetNumChildren(ComponentNum);
	for (uint32 Index = 0; Index < ComponentNum; ++Index)
	{
		const TSharedPtr<IPropertyHandle> ComponentHandle = ComponentsProperty->GetChildHandle(Index);
		CheckCondition(ComponentHandle, continue;);

		UObject* Component;

		// nullptr is allowed, new element is always nullptr
		if (const FPropertyAccess::Result Result = ComponentHandle->GetValue(Component);
			Result != FPropertyAccess::Result::Success/* || !Component*/)
		{
			continue;
		}

		GenerateWidgetForComponent(ComponentsGroup, Component, Index, ComponentHandle);
	}
}

void FComponentBasedWidgetDetails::ForceRefreshDetails(IDetailChildrenBuilder* DetailBuilder)
{
	DetailBuilder->GetParentCategory().GetParentLayout().ForceRefreshDetails();
}

void FComponentBasedWidgetDetails::GenerateWidgetForComponent(IDetailGroup& ComponentsGroup, const UObject* Component,
	uint32 ComponentIndex, const TSharedPtr<IPropertyHandle> ComponentHandle)
{
	IDetailGroup* ComponentNameGroup;
	{
		const FName ComponentGroupName = *FString::Format(TEXT("Index[{0}]"),{ComponentIndex});
		IDetailGroup& ComponentGroup = ComponentsGroup.AddGroup(ComponentGroupName, FText::FromName(ComponentGroupName));
		
		ComponentGroup
		.HeaderProperty(ComponentHandle.ToSharedRef())
		.EditCondition(ComponentHandle->IsEditable(), {});

		// new element?
		if (!Component)
		{
			return;
		}
		
		const FName ComponentName = Component->GetClass()->GetFName();
		ComponentNameGroup = &ComponentGroup.AddGroup(ComponentName, FText::FromName(ComponentName));
		CheckPointer(ComponentNameGroup, return;);
	}

	const TSharedPtr<IPropertyHandle> TrueComponentHandle = ComponentHandle->GetChildHandle(0)->GetChildHandle(0);
	
	uint32 ComponentPropertyNum;
	TrueComponentHandle->GetNumChildren(ComponentPropertyNum);

	for (uint32 Index = 0; Index < ComponentPropertyNum; ++Index)
	{
		TSharedPtr<IPropertyHandle> PropertyHandle = TrueComponentHandle->GetChildHandle(Index);
		CheckCondition(PropertyHandle.IsValid(), continue;);

		IDetailPropertyRow& WidgetPropertyRow = ComponentNameGroup->AddPropertyRow(PropertyHandle.ToSharedRef());
		WidgetPropertyRow.EditCondition(PropertyHandle->IsEditable(), {});
		

		if (const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(PropertyHandle->GetProperty());
			!Property || !Property->PropertyClass->IsChildOf<UWidget>())
		{
			continue;
		}

		TSharedPtr<SComboButton> ComboButton = SNew(SComboButton)
			.ButtonStyle(FEditorStyle::Get(), "PropertyEditor.AssetComboStyle")
			.ForegroundColor(FEditorStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
			.ContentPadding(2.0f)
			.IsEnabled(!PropertyHandle->IsEditConst())
			.ButtonContent()
			[
				SNew(STextBlock)
				.Text(TAttribute<FText>::CreateLambda([=]
				{
					return GetCurrentValueText(PropertyHandle);
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
		WidgetListView->SetSelection(GetCurrentValue(ChildHandle, Result));

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

FText FComponentBasedWidgetDetails::GetCurrentValueText(const TSharedPtr<IPropertyHandle> ChildHandle)
{
	if (ChildHandle.IsValid())
	{
		int32 Result;
		const UWidget* Widget = GetCurrentValue(ChildHandle, Result);
		switch (Result)
		{
			case FPropertyAccess::MultipleValues:
				return LOCTEXT("MultipleValues", "Multiple Values");
			case FPropertyAccess::Success:
				if (Widget)
				{
					return Widget->GetLabelText();
				}
			default:
				break;
		}
	}

	return FText::GetEmpty();
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
				.Text(Widget->GetLabelText())
			];
	}

	return SNew(STableRow<TWeakObjectPtr<UWidget>>, OwnerTable);
}

UWidget* FComponentBasedWidgetDetails::GetCurrentValue(const TSharedPtr<IPropertyHandle> ChildHandle, int32& OutResult)
{
	if (ChildHandle.IsValid())
	{
		switch (TArray<FString> PerObjectValues;
			OutResult = ChildHandle->GetPerObjectValues(PerObjectValues))
		{
			case FPropertyAccess::Success:
				if (PerObjectValues.Num() > 0)
				{
					return TSoftObjectPtr<UWidget>(PerObjectValues[0]).Get();
				}
			default:
				break;
		}
	}

	return {};
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

		const FObjectPropertyBase* ObjectProperty = CastFieldChecked<FObjectPropertyBase>(ChildHandle->GetProperty());
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
