// Fill out your copyright notice in the Description page of Project Settings.


#include "ComponentBasedWidgetDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "FDetailCustomizationUtilities.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "WidgetComponentBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Macro/AssertionMacros.h"
#include "Misc.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "ComponentBasedWidget"

TSharedRef<IDetailCustomization> FComponentBasedWidgetDetails::MakeInstance()
{
	return MakeShared<FComponentBasedWidgetDetails>();
}

void FComponentBasedWidgetDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	// cache widget blueprint class
	WidgetBlueprintClass = Cast<UWidgetBlueprintGeneratedClass>(DetailBuilder->GetBaseClass());

	const FName ComponentsPropertyName = TEXT("Components");
	const TSharedPtr<IPropertyHandle> PropertyHandle = DetailBuilder->GetProperty(ComponentsPropertyName);
	IDetailCategoryBuilder& ComponentsCategory = DetailBuilder->EditCategory(PropertyHandle->GetDefaultCategoryName());
	
	const FProperty* ComponentsProperty = PropertyHandle->GetProperty();
	IDetailGroup& ComponentsGroup = ComponentsCategory.AddGroup(FObjectEditorUtils::GetCategoryFName(ComponentsProperty),
		FObjectEditorUtils::GetCategoryText(ComponentsProperty));
	
	ComponentsGroup.HeaderProperty(PropertyHandle.ToSharedRef());

	// refresh detail view when value changed, element added, deleted, inserted, etc...
	PropertyHandle->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda(
		[DetailBuilderWeakPtr = TWeakPtr<IDetailLayoutBuilder>(DetailBuilder)]
	{
		if (DetailBuilderWeakPtr.IsValid())
		{
			DetailBuilderWeakPtr.Pin()->ForceRefreshDetails();
		}
	}));
	
	uint32 ComponentNum;
	PropertyHandle->GetNumChildren(ComponentNum);

	// traverse components array
	for (uint32 Index = 0; Index < ComponentNum; ++Index)
	{
		const TSharedPtr<IPropertyHandle> ComponentHandle = PropertyHandle->GetChildHandle(Index);
		CheckCondition(ComponentHandle, continue;);

		// Generate widget for component
		GenerateWidgetForComponent(ComponentsGroup, ComponentHandle);
	}
}

void FComponentBasedWidgetDetails::GenerateWidgetForComponent(IDetailGroup& ComponentsGroup, const TSharedPtr<IPropertyHandle> ComponentHandle)
{
	// add index[] group
	const FName ComponentGroupName = *FString::Format(TEXT("Index[{0}]"),{ComponentHandle->GetIndexInArray()});
	IDetailGroup& ComponentGroup = ComponentsGroup.AddGroup(ComponentGroupName, FText::FromName(ComponentGroupName));
		
	ComponentGroup
	.HeaderProperty(ComponentHandle.ToSharedRef())
	.EditCondition(ComponentHandle->IsEditable(), {});

	// whether the component has valid value
	uint32 ElementExist;
	ComponentHandle->GetNumChildren(ElementExist);
	if (ElementExist == 0)
	{
		return;
	}
	
	/*// add element name group
	
	UObject* Component;

	if (const FPropertyAccess::Result Result = ComponentHandle->GetValue(Component);
		Result != FPropertyAccess::Result::Success || !Component)
	{
		return;
	}
	
	const UClass* ComponentClass = Component->GetClass();
	CheckCondition(ComponentClass, return;);
	
	IDetailGroup& ComponentNameGroup = ComponentGroup.AddGroup(ComponentClass->GetFName(),
	                                                           ComponentClass->GetDisplayNameText());*/
	
	// add element property and groups
	
	const TSharedPtr<IPropertyHandle> WidgetComponentHandle = ComponentHandle->GetChildHandle(0);
	uint32 NumChildren;
	WidgetComponentHandle->GetNumChildren(NumChildren);

	if (NumChildren <= 0)
	{
		return;
	}

	TArray<TMap<FName, IDetailGroup*>> ChildGroupLayerMapping{ { {NAME_None, &ComponentGroup} } };
	
	GenerateWidgetsForNestedElement(WidgetComponentHandle, NumChildren, ChildGroupLayerMapping, 0);
}

void FComponentBasedWidgetDetails::GenerateWidgetsForNestedElement(const TSharedPtr<IPropertyHandle> PropertyHandle,
	const uint32 NumChildren, TArray<TMap<FName, IDetailGroup*>>& ChildGroupLayerMapping, const uint32 Layer)
{
	for (uint32 Index = 0; Index < NumChildren; ++Index)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(Index);
		CheckCondition(ChildHandle.IsValid(), continue;);

		// if this child is a property
		if (const FProperty* Property = ChildHandle->GetProperty())
		{
			// add property group
			IDetailGroup* PropertyGroup = ChildGroupLayerMapping[Layer - 1].FindRef(NAME_None);
			
			if (FName PropertyGroupName = FObjectEditorUtils::GetCategoryFName(ChildHandle->GetProperty());
				PropertyGroupName != NAME_None)
			{
				if (PropertyGroup = ChildGroupLayerMapping[Layer].FindRef(PropertyGroupName);
					!PropertyGroup)
				{
					FText InLocalizedDisplayName = FObjectEditorUtils::GetCategoryText(ChildHandle->GetProperty());

					IDetailGroup* ParentGroup = ChildGroupLayerMapping[Layer - 1][NAME_None];
					PropertyGroup = &ParentGroup->AddGroup(PropertyGroupName, InLocalizedDisplayName);

					ChildGroupLayerMapping[Layer].Add(PropertyGroupName, PropertyGroup);
				}
			}

			// PropertyGroup need to be valid from now on
			CheckPointer(PropertyGroup, continue;);

			const UStruct* Base = UWidget::StaticClass();

			bool bNeedCustomWidget = true;
			if (const FSoftObjectProperty* ObjectPropertyBase = CastField<FSoftObjectProperty>(Property))
			{
				if (!ObjectPropertyBase->PropertyClass->IsChildOf(Base))
				{
					continue;
				}
			}
			else if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
			{
				if (!Common::IsPropertyClassChildOf<FSoftObjectProperty>(ArrayProperty->Inner, Base))
				{
					continue;
				}

				
			}
			else if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
			{
				if (!Common::IsPropertyClassChildOf<FSoftObjectProperty>(MapProperty->KeyProp, Base))
				{
					continue;
				}
			
				if (!Common::IsPropertyClassChildOf<FSoftObjectProperty>(MapProperty->ValueProp, Base))
				{
					continue;
				}
			}
			else if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
			{
				if (!Common::IsPropertyClassChildOf<FSoftObjectProperty>(SetProperty->ElementProp, Base))
				{
					continue;
				}
			}
			else
			{
				bNeedCustomWidget = false;
			}
			
			// add property row
			IDetailPropertyRow& WidgetPropertyRow = PropertyGroup->AddPropertyRow(ChildHandle.ToSharedRef());
			WidgetPropertyRow.EditCondition(ChildHandle->IsEditable(), {});

			if (bNeedCustomWidget)
			{
				MakeCustomWidget(ChildHandle, WidgetPropertyRow);
			}
		}
		// if this child is a category
		else
		{
			// prevent duplicate adding on same layer but different category
			if (ChildGroupLayerMapping.Num() <= static_cast<int32>(Layer + 1))
			{
				ChildGroupLayerMapping.AddDefaulted();
			}

			uint32 NumChildrenOfChildHandle;
			ChildHandle->GetNumChildren(NumChildrenOfChildHandle);
			if (NumChildrenOfChildHandle != 0)
			{
				/*if (FName NewGroupName = FObjectEditorUtils::GetCategoryFName(ChildHandle->GetProperty()); NewGroupName != NAME_None)
				{
					FText InLocalizedDisplayName = FObjectEditorUtils::GetCategoryText(ChildHandle->GetProperty());
						
					//ChildGroup = ParentGroup.AddGroup(NewGroupName, InLocalizedDisplayName);
	
					/*if (NewGroupName == NAME_None)
					{
						if (const FFieldClass* PropertyClass = ChildHandle->GetPropertyClass())
						{
							NewGroupName			= PropertyClass->GetFName();
							InLocalizedDisplayName	= PropertyClass->GetDisplayNameText();
						}
					}#1#
				}*/

				// and property group and generate nested properties
				GenerateWidgetsForNestedElement(ChildHandle, NumChildrenOfChildHandle, ChildGroupLayerMapping, Layer+ 1);
			}
		}
	}
}

void FComponentBasedWidgetDetails::MakeCustomWidget(const TSharedPtr<IPropertyHandle> PropertyHandle, IDetailPropertyRow& WidgetPropertyRow)
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
				.Text(FDetailCustomizationUtilities::GetWidgetName(Widget))
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
