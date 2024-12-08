// Copyright RemRemRemRe, All Rights Reserved.

#pragma once

#include "IDetailCustomization.h"

class STableViewBase;
class ITableRow;
class SComboButton;
class FWidgetBlueprintEditor;
class UBaseWidgetBlueprint;
class UWidgetBlueprintGeneratedClass;
class UWidget;
class IDetailLayoutBuilder;
class IPropertyHandle;
namespace ESelectInfo { enum Type : int; }
template <typename ItemType>
class SListView;
class SWidget;

/**
 * Detail customization for any user widget with URemWidgetComponentAsExtension component
 */
class REMWIDGETCOMPONENTEDITOR_API FRemComponentBasedWidgetDetails : public IDetailCustomization
{
#pragma region Data Members

	TArray<TWeakObjectPtr<UWidget>> ListViewItems;

	TWeakObjectPtr<const UWidgetBlueprintGeneratedClass> WidgetBlueprintGeneratedClass;

#pragma endregion Data Members

public:
	using ThisClass = FRemComponentBasedWidgetDetails;
	using FListViewItemType = decltype(ListViewItems)::ElementType;

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) override {}
	virtual void CustomizeDetails( const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder ) override;

protected:
	virtual void OnFilterTextChanged(const FText& InFilterText, const TSharedRef<IPropertyHandle> FilterTextPropertyHandle,
		const TSharedRef<SListView<TWeakObjectPtr<UWidget>>> WidgetListView);
};
