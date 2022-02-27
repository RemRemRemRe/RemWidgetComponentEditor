// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentBasedWidgetDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "WidgetComponentBase.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Macro/AssertionMacros.h"

#define LOCTEXT_NAMESPACE "ComponentBasedWidget"

TSharedRef<IDetailCustomization> FComponentBasedWidgetDetails::MakeInstance()
{
	return MakeShared<FComponentBasedWidgetDetails>();
}

const FName ComponentsName = TEXT("Components");

void FComponentBasedWidgetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray< TWeakObjectPtr<UObject> > ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	CheckCondition(ObjectsBeingCustomized.Num() > 0, return;);

	const TWeakObjectPtr<>& ComponentBasedWidgetPtr = ObjectsBeingCustomized[0];
	WidgetBlueprintClass = Cast<UWidgetBlueprintGeneratedClass>(ComponentBasedWidgetPtr.Get()->GetClass());

	CheckCondition(WidgetBlueprintClass.IsValid(), return;);

	const TSharedPtr<IPropertyHandleArray> ComponentsProperty = DetailBuilder.GetProperty(ComponentsName)->AsArray();
	CheckCondition(ComponentsProperty.IsValid(), return;);

	uint32 ComponentNum;
	ComponentsProperty->GetNumElements(ComponentNum);
	for (uint32 Index = 0; Index < ComponentNum; ++Index)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = ComponentsProperty->GetElement(Index);
		CheckCondition(ChildHandle.IsValid(), continue;);

		UObject* Component;

		if (const FPropertyAccess::Result Result = ChildHandle->GetValue(Component);
			Result != FPropertyAccess::Result::Success || !Component)
		{
			continue;
		}

		GenerateWidgetForComponent(DetailBuilder, Component, Index);
	}
}

void FComponentBasedWidgetDetails::GenerateWidgetForComponent(IDetailLayoutBuilder& DetailBuilder, const UObject* Component,
	uint32 ComponentIndex)
{
	TArray<FName> WidgetPropertyNames;
	for (TFieldIterator<FObjectProperty> It(Component->GetClass()); It; ++It)
	{
		const FObjectProperty* ObjectProperty = *It;
		CheckPointer(ObjectProperty, continue;);

		if (ObjectProperty->PropertyClass->IsChildOf(UWidget::StaticClass()))
		{
			WidgetPropertyNames.Emplace(ObjectProperty->GetName());
		}
	}

	TArray<TSharedRef<IPropertyHandle> > WidgetPropertyHandles;
	WidgetPropertyHandles.Reserve(WidgetPropertyNames.Num());

	for (const FName& WidgetPropertyName : WidgetPropertyNames)
	{
		const FName PropertyPath = *FString::Format(TEXT("{0}[{1}].{2}"),
			{ComponentsName.ToString(), ComponentIndex, WidgetPropertyName.ToString()});
		
		WidgetPropertyHandles.Emplace(DetailBuilder.GetProperty(PropertyPath));
	}
		
	for	(const TSharedRef<IPropertyHandle> WidgetPropertyHandle : WidgetPropertyHandles)
	{
		IDetailPropertyRow* WidgetRow = DetailBuilder.EditDefaultProperty(WidgetPropertyHandle);
		CheckPointer(WidgetRow, continue;);
		
		if (FDetailWidgetDecl* CustomValueWidget = WidgetRow->CustomValueWidget())
		{
			(*CustomValueWidget)
			[
				SAssignNew(WidgetListComboButton, SComboButton)
				.ButtonStyle(FEditorStyle::Get(), "PropertyEditor.AssetComboStyle")
				.ForegroundColor(FEditorStyle::GetColor("PropertyEditor.AssetName.ColorAndOpacity"))
				.OnGetMenuContent(this, &FComponentBasedWidgetDetails::GetPopupContent, TSharedPtr<IPropertyHandle>(WidgetPropertyHandle))
				.ContentPadding(2.0f)
				.IsEnabled(!WidgetPropertyHandle->IsEditConst())
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FComponentBasedWidgetDetails::GetCurrentValueText, TSharedPtr<IPropertyHandle>(WidgetPropertyHandle))
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			];
		}
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
			TArray<FString> References;
			for (int32 Index = 0; Index < ChildHandle->GetNumPerObjectValues(); ++Index)
			{
				References.Add(InItem.Get()->GetPathName());
			}
			ChildHandle->SetPerObjectValues(References);

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
