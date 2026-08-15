// Copyright RemRemRemRe, All Rights Reserved.

#include "RemComponentBasedWidgetDetails.h"

#include "BaseWidgetBlueprint.h"
#include "DetailLayoutBuilder.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"

TSharedRef<IDetailCustomization> FRemComponentBasedWidgetDetails::MakeInstance()
{
    return MakeShared<FRemComponentBasedWidgetDetails>();
}

void FRemComponentBasedWidgetDetails::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
    IDetailCustomization::CustomizeDetails(DetailBuilder);

    // Disabled after the instanced-struct refactor: the component array is now
    // URemComponentBasedWidget::Components (FRemComponentContainer), edited
    // natively, so the TFieldPath bridge and the tree-filter widget picker are no
    // longer needed. The previous implementation (a FSoftObjectProperty combo box
    // whose drop-down was filtered against the current widget tree via
    // URemWidgetComponentAsExtension) was removed from this function; see git
    // history before 2026-07.
    // To restore the tree-filter picker:
    //   1. locate the component array property (protected) on
    //      FRemComponentContainer via FRemComponentContainer::StaticStruct()
    //      (TArray<TInstancedStruct<FRemComponentBase>>);
    //   2. attach a combo picker to each component's TSoftObjectPtr<UWidget>;
    //   3. OnFilterTextChanged and the RemEditorUtilities template helpers
    //      (kept below) can be reused directly.
}

void FRemComponentBasedWidgetDetails::OnFilterTextChanged(const FText& InFilterText,
    const TSharedRef<IPropertyHandle> FilterTextPropertyHandle,
    const TSharedRef<SListView<FListViewItemType>> WidgetListView)
{
    if (auto* GeneratedClass = WidgetBlueprintGeneratedClass.Get())
    {
        // Only use UBaseWidgetBlueprint::WidgetTree
        // rather than UUserWidget::WidgetTree
        // or UWidgetBlueprintGeneratedClass::WidgetTree (the widget class version),
        // so the drop-down list stays up to date after a widget rename.
        TArray<UWidget*> AllWidgets;
        Cast<UBaseWidgetBlueprint>(GeneratedClass->ClassGeneratedBy)->WidgetTree->GetAllWidgets(AllWidgets);

        AllWidgets.Sort([](const UWidget& Lhs, const UWidget& Rhs)
        {
            return Lhs.GetLabelText().CompareTo(Rhs.GetLabelText()) < 0;
        });

        const auto* ObjectProperty = CastField<FObjectPropertyBase>(FilterTextPropertyHandle->GetProperty());
        if (!ObjectProperty)
        {
            return;
        }

        const UClass* FilterWidgetClass = ObjectProperty->PropertyClass;

        ListViewItems.Reset();

        const auto& CurrentFilterString = InFilterText.ToString();

        for (auto* Widget : AllWidgets)
        {
            if (!Widget || !Widget->IsA(FilterWidgetClass))
            {
                continue;
            }

            if (CurrentFilterString.IsEmpty() ||
                Widget->GetName().Contains(CurrentFilterString) ||
                Widget->GetDisplayLabel().Contains(CurrentFilterString) ||
                Widget->GetClass()->GetName().Contains(CurrentFilterString))
            {
                ListViewItems.Add(Widget);
            }
        }

        ListViewItems.Shrink();
        WidgetListView->RequestListRefresh();
    }
}
