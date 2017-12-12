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
#include <AzCore/std/sort.h>

CommandHierarchyItemReparent::CommandHierarchyItemReparent(UndoStack* stack,
    HierarchyWidget* hierarchy,
    HierarchyItemRawPtrList& items)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_listToValidate()
    , m_isFirstExecution(true)
    , m_sourceChildrenSorted(false)
    , m_destinationChildrenSorted(false)
{
    for (auto item : items)
    {
        // Add source child info
        ChildItem sourceChildItem;
        sourceChildItem.m_id = item->GetEntityId();
        sourceChildItem.m_parentId = item->GetPreMoveParentId();
        sourceChildItem.m_row = item->GetPreMoveChildRow();
        m_sourceChildren.push_back(sourceChildItem);

        // Add destination child info
        ChildItem destChildItem;
        destChildItem.m_id = sourceChildItem.m_id;

        QTreeWidgetItem* itemParent = item->QTreeWidgetItem::parent();
        if (!itemParent)
        {
            itemParent = m_hierarchy->invisibleRootItem();
        }
        HierarchyItem* postMoveParent = dynamic_cast<HierarchyItem*>(itemParent);
        destChildItem.m_parentId = (postMoveParent ? postMoveParent->GetEntityId() : AZ::EntityId());

        destChildItem.m_row = itemParent->indexOfChild(item);

        m_destinationChildren.push_back(destChildItem);

        // Add items to validate
        if (sourceChildItem.m_id.IsValid())
        {
            m_listToValidate.push_back(sourceChildItem.m_id);
        }

        if (sourceChildItem.m_parentId.IsValid())
        {
            m_listToValidate.push_back(sourceChildItem.m_parentId);
        }

        if (destChildItem.m_parentId.IsValid())
        {
            m_listToValidate.push_back(destChildItem.m_parentId);
        }
    }

    if (items.size() == 1)
    {
        setText(QString("move \"%1\"").arg(items.front()->GetElement()->GetName().c_str()));

    }
    else
    {
        setText(QString("move elements"));
    }
}

void CommandHierarchyItemReparent::undo()
{
    UndoStackExecutionScope s(m_stack);

    Reparent(m_destinationChildren, m_sourceChildren, m_sourceChildrenSorted);
}

void CommandHierarchyItemReparent::redo()
{
    UndoStackExecutionScope s(m_stack);

    Reparent(m_sourceChildren, m_destinationChildren, m_destinationChildrenSorted);
}

void CommandHierarchyItemReparent::Reparent(ChildItemList& sourceChildren, ChildItemList& destChildren, bool& childrenSorted)
{
    if (!HierarchyHelpers::AllItemExists(m_hierarchy, m_listToValidate))
    {
        // At least one item is missing.
        // Nothing to do.
        return;
    }

    if (!childrenSorted)
    {
        // Sort children by destination index to be inserted in that order
        destChildren.sort([](const ChildItem& i1, const ChildItem& i2) { return i1.m_row < i2.m_row; });

        childrenSorted = true;
    }

    if (m_isFirstExecution)
    {
        m_isFirstExecution = false;
    }
    else
    {
        // Editor-side.
        //
        // IMPORTANT: The editor-side MUST be done BEFORE the runtime-side.

        // First remove all items
        for (auto child : sourceChildren)
        {
            HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(m_hierarchy, child.m_id, false));
            QTreeWidgetItem* sourceParentItem = HierarchyHelpers::ElementToItem(m_hierarchy, child.m_parentId, true);
            sourceParentItem->removeChild(item);
        }

        // Add items to new parents
        for (auto child : destChildren)
        {
            HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(m_hierarchy, child.m_id, false));
            QTreeWidgetItem* destinationParentItem = HierarchyHelpers::ElementToItem(m_hierarchy, child.m_parentId, true);
            destinationParentItem->insertChild(child.m_row, item);
        }
    }

    // Runtime-side.
    //
    // IMPORTANT: The runtime-side depends on the editor-side being done FIRST.

    // First remove all elements
    for (auto child : sourceChildren)
    {
        EBUS_EVENT_ID(child.m_id, UiElementBus, RemoveFromParent);
    }

    // Add elements to new parents
    LyShine::EntityArray selectedElements;
    for (auto child : destChildren)
    {
        QTreeWidgetItem* destinationParentItem = HierarchyHelpers::ElementToItem(m_hierarchy, child.m_parentId, true);
        HierarchyItem* destinationParent = dynamic_cast<HierarchyItem*>(destinationParentItem);
        AZ::Entity* parentElement = (destinationParent ? destinationParent->GetElement() : nullptr);

        // Add the element to its new parent
        EBUS_EVENT_ID(child.m_id, UiElementBus, AddToParentAtIndex, parentElement, child.m_row);

        // Add item to selected item list
        HierarchyItem* item = dynamic_cast<HierarchyItem*>(HierarchyHelpers::ElementToItem(m_hierarchy, child.m_id, false));
        selectedElements.push_back(item->GetElement());
    }

    // Set the focus to the currently manipulated items
    HierarchyHelpers::SetSelectedItems(m_hierarchy, &selectedElements);
}

void CommandHierarchyItemReparent::Push(bool onCreationOfElement,
    UndoStack* stack,
    HierarchyWidget* hierarchy,
    HierarchyItemRawPtrList& items,
    QTreeWidgetItemRawPtrList& itemParents)
{
    if (onCreationOfElement)
    {
        // This ISN'T a real move. This is a reparenting done "on creation".
        // We DON'T need to be able to undo this here. The undoing of the
        // creation is done in a different QUndoCommand.
        //
        // IMPORTANT: We ONLY need to do the runtime-side because
        // the editor-side is already done for us "on creation".

        AZ_Assert(items.size() == 1, "Creating more than one element at a time is not supported");

        // QTreeWidgetItem* -> AZ::Entity*
        HierarchyItem* destinationParent = dynamic_cast<HierarchyItem*>(itemParents.front());
        AZ::Entity* parentElement = (destinationParent ? destinationParent->GetElement() : nullptr);

        // QTreeWidgetItem* -> AZ::Entity*
        HierarchyItem* insertAboveThisItem = dynamic_cast<HierarchyItem*>(itemParents.front()->child(itemParents.front()->indexOfChild(items.front()) + 1));
        AZ::Entity* insertAboveThisElement = (insertAboveThisItem ? insertAboveThisItem->GetElement() : nullptr);

        // Reparent.
        EBUS_EVENT_ID(items.front()->GetEntityId(), UiElementBus, Reparent, parentElement, insertAboveThisElement);
    }
    else
    {
        // This is a move of an existing element.
        // NOT a reparenting done "on creation".

        stack->push(new CommandHierarchyItemReparent(stack,
                hierarchy,
                items));
    }
}
