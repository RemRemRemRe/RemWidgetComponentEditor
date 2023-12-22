// Copyright RemRemRemRe, All Rights Reserved.

#pragma once

#include "RemDetailCustomizationUtilities.h"
#include "IDetailCustomization.h"

class FWidgetBlueprintEditor;
class UBaseWidgetBlueprint;
class UWidgetBlueprintGeneratedClass;
class UWidget;
class IDetailLayoutBuilder;
class IPropertyHandle;

using namespace Rem::DetailCustomizationUtilities;

/**
 * Detail customization for any user widget with URemWidgetComponentAsExtension component
 */
class REMWIDGETCOMPONENTEDITOR_API FRemComponentBasedWidgetDetails : public IDetailCustomization
{
#pragma region Data Members

	TArray<TWeakObjectPtr<UWidget>> ReferencableWidgets;

	TWeakObjectPtr<UWidgetBlueprintGeneratedClass> WidgetBlueprintGeneratedClass;

	FWidgetBlueprintEditor* WidgetBlueprintEditor{};

#pragma endregion Data Members

public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override {}
	virtual void CustomizeDetails( const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder ) override;

protected:
	virtual TSharedRef<SWidget> MakeComboButton(const TSharedPtr<IPropertyHandle>& PropertyHandle);

	virtual TSharedRef<SWidget> GetPopupContent(const TSharedPtr<IPropertyHandle> ChildHandle,
	                                            const TSharedPtr<SComboButton> WidgetListComboButton);

	virtual void OnSelectionChanged(TWeakObjectPtr<UWidget> InItem, ESelectInfo::Type SelectionInfo,
		TSharedPtr<IPropertyHandle> ChildHandle, TSharedPtr<SComboButton> WidgetListComboButton) const;

	virtual TSharedRef<ITableRow> OnGenerateListItem(TWeakObjectPtr<UWidget> InItem,
											const TSharedRef<STableViewBase>& OwnerTable) const;

	virtual void OnFilterTextChanged(const FText& InFilterText, const TSharedPtr<IPropertyHandle> ChildHandle,
	                                 const TSharedPtr<SListView<TWeakObjectPtr<UWidget>>> WidgetListView);
};
