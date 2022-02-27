// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

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

	TSharedPtr<SComboButton> WidgetListComboButton = nullptr;

	TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView;

	TArray<TWeakObjectPtr<UWidget>> ReferencableWidgets;

	TSharedPtr<SSearchBox> SearchBox;

	TWeakObjectPtr<UWidgetBlueprintGeneratedClass> WidgetBlueprintClass;

#pragma endregion Data Members

public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override;

private:
	void GenerateWidgetForComponent(IDetailLayoutBuilder& DetailBuilder, const UObject* Component, uint32 ComponentIndex);
	
	TSharedRef<SWidget> GetPopupContent(const TSharedPtr<IPropertyHandle> ChildHandle);
	
	FText GetCurrentValueText(const TSharedPtr<IPropertyHandle> ChildHandle) const;
	
	void OnSelectionChanged(TWeakObjectPtr<UWidget> InItem, ESelectInfo::Type SelectionInfo, const TSharedPtr<IPropertyHandle> ChildHandle) const;
	
	TSharedRef<ITableRow> OnGenerateListItem(TWeakObjectPtr<UWidget> InItem,
	                        const TSharedRef<STableViewBase>& OwnerTable) const;
	
	UWidget* GetCurrentValue(const TSharedPtr<IPropertyHandle> ChildHandle) const;
	
	
	void OnFilterTextChanged(const FText& InFilterText, const TSharedPtr<IPropertyHandle> ChildHandle);
};
