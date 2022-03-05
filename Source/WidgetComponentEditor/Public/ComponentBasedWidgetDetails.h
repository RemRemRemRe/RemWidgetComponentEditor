// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailGroup.h"
#include "IPropertyTypeCustomization.h"

class UWidgetBlueprintGeneratedClass;
class UWidget;
class IDetailLayoutBuilder;
class IPropertyHandle;

/**
 * 
 */
class WIDGETCOMPONENTEDITOR_API FComponentBasedWidgetDetails : public IPropertyTypeCustomization
{
#pragma region Data Members

	TArray<TWeakObjectPtr<UWidget>> ReferencableWidgets;

	TWeakObjectPtr<UWidgetBlueprintGeneratedClass> WidgetBlueprintClass;

#pragma endregion Data Members

public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	
	virtual void CustomizeHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils ) override;
	virtual void CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& DetailBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils ) override;


private:
	void ForceRefreshDetails(IDetailChildrenBuilder* DetailBuilder);
	
	void GenerateWidgetForComponent(IDetailGroup& ComponentsGroup,
									const UObject* Component, uint32 ComponentIndex, const TSharedPtr<IPropertyHandle> ComponentHandle);
	
	TSharedRef<SWidget> GetPopupContent(const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SComboButton> WidgetListComboButton);

	static FText GetCurrentValueText(const TSharedPtr<IPropertyHandle> ChildHandle);
	
	void OnSelectionChanged(TWeakObjectPtr<UWidget> InItem, ESelectInfo::Type SelectionInfo, const TSharedPtr<IPropertyHandle> ChildHandle,
		const TSharedPtr<SComboButton> WidgetListComboButton) const;
	
	TSharedRef<ITableRow> OnGenerateListItem(TWeakObjectPtr<UWidget> InItem,
											const TSharedRef<STableViewBase>& OwnerTable) const;

	static UWidget* GetCurrentValue(const TSharedPtr<IPropertyHandle> ChildHandle, int32& OutResult);
	
	void OnFilterTextChanged(const FText& InFilterText, const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView);
};
