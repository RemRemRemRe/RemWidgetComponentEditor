// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "IDetailGroup.h"

class UWidgetBlueprintGeneratedClass;
class UWidget;
class IDetailLayoutBuilder;
class IPropertyHandle;

/**
 * 
 */
class WIDGETCOMPONENTEDITOR_API FComponentBasedWidgetDetails : public IDetailCustomization
{
#pragma region Data Members

	TArray<TWeakObjectPtr<UWidget>> ReferencableWidgets;

	TWeakObjectPtr<UWidgetBlueprintGeneratedClass> WidgetBlueprintClass;

#pragma endregion Data Members

public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails( const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder ) override;

private:
	void GenerateWidgetForComponent(IDetailGroup& ComponentsGroup, const TSharedPtr<IPropertyHandle> ComponentHandle);

	void GenerateWidgetsForNestedElement(const TSharedPtr<IPropertyHandle> PropertyHandle, uint32 NumChildren,
		TArray<TMap<FName, IDetailGroup*>>& ChildGroupLayerMapping, uint32 Layer);
	
	void MakeCustomWidget(TSharedPtr<IPropertyHandle> PropertyHandle, IDetailPropertyRow& WidgetPropertyRow);
	
	TSharedRef<SWidget> GetPopupContent(const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SComboButton> WidgetListComboButton);

	static FText GetCurrentValueText(const TSharedPtr<IPropertyHandle> ChildHandle);
	
	void OnSelectionChanged(TWeakObjectPtr<UWidget> InItem, ESelectInfo::Type SelectionInfo, const TSharedPtr<IPropertyHandle> ChildHandle,
		const TSharedPtr<SComboButton> WidgetListComboButton) const;
	
	TSharedRef<ITableRow> OnGenerateListItem(TWeakObjectPtr<UWidget> InItem,
											const TSharedRef<STableViewBase>& OwnerTable) const;

	static UWidget* GetCurrentValue(const TSharedPtr<IPropertyHandle> ChildHandle, int32& OutResult);
	
	void OnFilterTextChanged(const FText& InFilterText, const TSharedPtr<IPropertyHandle> ChildHandle, const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView);
};
