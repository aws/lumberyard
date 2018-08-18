/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "stdafx.h"

#include "EditorCommon.h"
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include "SliceMenuHelpers.h"

// Define for enabling/disabling the UI Slice system
#define ENABLE_UI_SLICE_MENU_ITEMS 1

HierarchyMenu::HierarchyMenu(HierarchyWidget* hierarchy,
    size_t showMask,
    bool addMenuForNewElement,
    AZ::Component* componentToRemove,
    const QPoint* optionalPos)
    : QMenu()
{
    setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    QTreeWidgetItemRawPtrQList selectedItems = hierarchy->selectedItems();


    if (showMask & (Show::kNew_EmptyElement | Show::kNew_ElementFromPrefabs |
                    Show::kNew_EmptyElementAtRoot | Show::kNew_ElementFromPrefabsAtRoot))
    {
        QMenu* menu = (addMenuForNewElement ? addMenu("&New...") : this);

        if (showMask & (Show::kNew_EmptyElement | Show::kNew_EmptyElementAtRoot))
        {
            New_EmptyElement(hierarchy, selectedItems, menu, (showMask & Show::kNew_EmptyElementAtRoot), optionalPos);
        }

        if (showMask & Show::kNew_InstantiateSlice | Show::kNew_InstantiateSliceAtRoot)
        {
            New_ElementFromSlice(hierarchy, selectedItems, menu, (showMask & Show::kNew_InstantiateSliceAtRoot), optionalPos);
        }

        if (showMask & (Show::kNew_ElementFromPrefabs | Show::kNew_ElementFromPrefabsAtRoot))
        {
            New_ElementFromPrefabs(hierarchy, selectedItems, menu, (showMask & Show::kNew_ElementFromPrefabsAtRoot), optionalPos);
        }
    }

    if (showMask & (Show::kNewSlice | Show::kPushToSlice))
    {
        SliceMenuItems(hierarchy, selectedItems, showMask);
    }

    if (showMask & Show::kSavePrefab)
    {
        SavePrefab(hierarchy, selectedItems);
    }

    addSeparator();

    if (showMask & Show::kCutCopyPaste)
    {
        CutCopyPaste(hierarchy, selectedItems);
    }

    if (showMask & Show::kDeleteElement)
    {
        DeleteElement(hierarchy, selectedItems);
    }

    addSeparator();

    if (showMask & Show::kAddComponents)
    {
        AddComponents(hierarchy, selectedItems);
    }

    addSeparator();

    if (showMask & Show::kRemoveComponents)
    {
        RemoveComponents(hierarchy, selectedItems, componentToRemove);
    }
}

void HierarchyMenu::CutCopyPaste(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    bool itemsAreSelected = (!selectedItems.isEmpty());

    // Cut element.
    {
        action = new QAction("Cut", this);
        action->setShortcut(QKeySequence::Cut);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ](bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "Cut", Qt::QueuedConnection);
            });
        addAction(action);

        if (!itemsAreSelected)
        {
            // Nothing has been selected.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }
    }

    // Copy element.
    {
        action = new QAction("Copy", this);
        action->setShortcut(QKeySequence::Copy);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ](bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "Copy", Qt::QueuedConnection);
            });
        addAction(action);

        if (!itemsAreSelected)
        {
            // Nothing has been selected.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }
    }

    bool thereIsContentInTheClipboard = ClipboardContainsOurDataType();

    // Paste element.
    {
        action = new QAction(QIcon(":/Icons/Eye_Open.png"), (itemsAreSelected ? "Paste as sibling" : "Paste"), this);
        action->setShortcut(QKeySequence::Paste);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ](bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "PasteAsSibling", Qt::QueuedConnection);
            });
        addAction(action);

        if (!thereIsContentInTheClipboard)
        {
            // Nothing in the clipboard.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }

        if (itemsAreSelected)
        {
            action = new QAction(QIcon(":/Icons/Eye_Open.png"), ("Paste as child"), this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V),
                                                         QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_V)});
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
            QObject::connect(action,
                &QAction::triggered,
                hierarchy,
                [hierarchy](bool checked)
                {
                    QMetaObject::invokeMethod(hierarchy, "PasteAsChild", Qt::QueuedConnection);
                });
            addAction(action);

            if (!thereIsContentInTheClipboard)
            {
                // Nothing in the clipboard.
                // We want the menu to be visible, but disabled.
                action->setEnabled(false);
            }
        }
    }
}

void HierarchyMenu::SavePrefab(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action = PrefabHelpers::CreateSavePrefabAction(hierarchy);

    // Only enable "save as prefab" option if exactly one element is selected
    // in the hierarchy pane
    if (selectedItems.size() != 1)
    {
        action->setEnabled(false);
    }

    addAction(action);
}

void HierarchyMenu::SliceMenuItems(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    size_t showMask)
{
#if ENABLE_UI_SLICE_MENU_ITEMS
    // Get the EntityId's of the selected elements
    auto selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selectedItems, false);

    // Determine if any of the selected entities are in a slice
    AZ::SliceComponent::EntityAncestorList referenceAncestors;

    AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
    for (const AZ::EntityId& entityId : selectedEntities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress(nullptr, nullptr);
        EBUS_EVENT_ID_RESULT(sliceAddress, entityId, AzFramework::EntityIdContextQueryBus, GetOwningSlice);

        if (sliceAddress.first)
        {
            if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
            {
                if (sliceInstances.empty())
                {
                    sliceAddress.first->GetInstanceEntityAncestry(entityId, referenceAncestors);
                }

                sliceInstances.push_back(sliceAddress);
            }
        }
    }

    bool sliceSelected = sliceInstances.size() > 0;

    if (sliceSelected)
    {
        if (showMask & Show::kPushToSlice)
        {
            QAction* action = addAction(QObject::tr("&Push to Slice..."));
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedEntities]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->PushEntitiesModal(selectedEntities, nullptr);
                }
            );
        }

        if (showMask & Show::kNewSlice)
        {
            QAction* action = addAction("Make Cascaded Slice from Selected Slices && Entities...");
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedEntities]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->MakeSliceFromSelectedItems(hierarchy, true);
                }
            );

            action = addAction(QObject::tr("Make Detached Slice from Selected Entities..."));
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedEntities]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->MakeSliceFromSelectedItems(hierarchy, false);
                }
            );
        }

        if (showMask & Show::kPushToSlice)  // use the push to slice flag to show detach since it appears in all the same situations
        {
            // Detach slice entity
            {
                // Detach entities action currently acts on entities and all descendants, so include those as part of the selection
                AzToolsFramework::EntityIdSet selectedTransformHierarchyEntities =
                    hierarchy->GetEditorWindow()->GetSliceManager()->GatherEntitiesAndAllDescendents(selectedEntities);

                AzToolsFramework::EntityIdList selectedDetachEntities;
                selectedDetachEntities.insert(selectedDetachEntities.begin(), selectedTransformHierarchyEntities.begin(), selectedTransformHierarchyEntities.end());

                QString detachEntitiesActionText;
                if (selectedDetachEntities.size() == 1)
                {
                    detachEntitiesActionText = QObject::tr("Detach slice entity...");
                }
                else
                {
                    detachEntitiesActionText = QObject::tr("Detach slice entities...");
                }
                QAction* action = addAction(detachEntitiesActionText);
                QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedDetachEntities]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->DetachSliceEntities(selectedDetachEntities);
                }
                );
            }

            // Detach slice instance
            {
                QString detachSlicesActionText;
                if (sliceInstances.size() == 1)
                {
                    detachSlicesActionText = QObject::tr("Detach slice instance...");
                }
                else
                {
                    detachSlicesActionText = QObject::tr("Detach slice instances...");
                }
                QAction* action = addAction(detachSlicesActionText);
                QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedEntities]

                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->DetachSliceInstances(selectedEntities);
                }
                );
            }
        }

    }
    else
    {
        if (showMask & Show::kNewSlice)
        {
            QAction* action = addAction(QObject::tr("Make New &Slice from Selection..."));
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->MakeSliceFromSelectedItems(hierarchy, false);
                }
            );

            if (selectedItems.size() == 0)
            {
                action->setEnabled(false);
            }
        }
    }
#endif
}

void HierarchyMenu::New_EmptyElement(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    QMenu* menu,
    bool addAtRoot,
    const QPoint* optionalPos)
{
    menu->addAction(HierarchyHelpers::CreateAddElementAction(hierarchy,
            selectedItems,
            addAtRoot,
            optionalPos));
}

void HierarchyMenu::New_ElementFromPrefabs(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    QMenu* menu,
    bool addAtRoot,
    const QPoint* optionalPos)
{
    PrefabHelpers::CreateAddPrefabMenu(hierarchy,
        selectedItems,
        menu,
        addAtRoot,
        optionalPos);
}

void HierarchyMenu::New_ElementFromSlice(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    QMenu* menu,
    bool addAtRoot,
    const QPoint* optionalPos)
{
#if ENABLE_UI_SLICE_MENU_ITEMS
    AZ::Vector2 viewportPosition(-1.0f,-1.0f); // indicates no viewport position specified
    if (optionalPos)
    {
        viewportPosition = EntityHelpers::QPointFToVec2(*optionalPos);
    }

    SliceMenuHelpers::CreateInstantiateSliceMenu(hierarchy,
        selectedItems,
        menu,
        addAtRoot,
        viewportPosition);

    QAction* action = menu->addAction(QObject::tr("Element from Slice &Browser..."));
    QObject::connect(
        action,
        &QAction::triggered,
        hierarchy, [hierarchy, optionalPos]
        {
            AZ::Vector2 viewportPosition(-1.0f,-1.0f); // indicates no viewport position specified
            if (optionalPos)
            {
                viewportPosition = EntityHelpers::QPointFToVec2(*optionalPos);
            }
            hierarchy->GetEditorWindow()->GetSliceManager()->InstantiateSliceUsingBrowser(hierarchy, viewportPosition);
        }
    );
#endif
}

void HierarchyMenu::AddComponents(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    addActions(ComponentHelpers::CreateAddComponentActions(hierarchy,
            selectedItems,
            this));
}

void HierarchyMenu::DeleteElement(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    // Delete element.
    {
        action = new QAction("Delete", this);
        action->setShortcut(QKeySequence::Delete);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ](bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "DeleteSelectedItems", Qt::QueuedConnection);
            });
        addAction(action);

        if (selectedItems.empty())
        {
            // Nothing has been selected.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }
    }
}

void HierarchyMenu::RemoveComponents(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    const AZ::Component* optionalOnlyThisComponentType)
{
    addActions(ComponentHelpers::CreateRemoveComponentActions(hierarchy,
            selectedItems,
            optionalOnlyThisComponentType));
}

#include <HierarchyMenu.moc>
