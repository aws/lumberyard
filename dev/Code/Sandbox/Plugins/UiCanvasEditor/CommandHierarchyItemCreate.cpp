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

#define UICANVASEDITOR_ELEMENT_NAME_DEFAULT     "Element"

CommandHierarchyItemCreate::CommandHierarchyItemCreate(UndoStack* stack,
    HierarchyWidget* hierarchy,
    EntityHelpers::EntityIdList& parents,
    PostCreationCallback postCreationCB)
    : QUndoCommand()
    , m_stack(stack)
    , m_hierarchy(hierarchy)
    , m_parents(parents)
    , m_entries()
    , m_postCreationCB(postCreationCB)
{
    setText(QString("create element") + (m_parents.empty() ? "" : "s"));
}

void CommandHierarchyItemCreate::undo()
{
    UndoStackExecutionScope s(m_stack);

    HierarchyHelpers::Delete(m_hierarchy, m_entries);
}

void CommandHierarchyItemCreate::redo()
{
    UndoStackExecutionScope s(m_stack);

    if (m_entries.empty())
    {
        // This is the first call to redo().

        HierarchyItemRawPtrList items;

        for (auto& p : m_parents)
        {
            auto parent = HierarchyHelpers::ElementToItem(m_hierarchy, p, true);

            // Find a unique name for the new element
            AZStd::string uniqueName;
            EBUS_EVENT_ID_RESULT(uniqueName,
                m_hierarchy->GetEditorWindow()->GetCanvas(),
                UiCanvasBus,
                GetUniqueChildName,
                p,
                UICANVASEDITOR_ELEMENT_NAME_DEFAULT,
                nullptr);

            items.push_back(new HierarchyItem(m_hierarchy->GetEditorWindow(),
                    parent,
                    QString(uniqueName.c_str()),
                    nullptr));

            m_hierarchy->ReparentItems(true, QTreeWidgetItemRawPtrList({ parent }), HierarchyItemRawPtrList({ items.back() }));

            AZ::Entity* element = items.back()->GetElement();
            m_postCreationCB(element);
        }

        // true: Put the serialized data in m_undoXml.
        HierarchyClipboard::Serialize(m_hierarchy, m_hierarchy->selectedItems(), &items, m_entries, true);
        AZ_Assert(!m_entries.empty(), "Failed to serialize");
    }
    else
    {
        HierarchyHelpers::CreateItemsAndElements(m_hierarchy, m_entries);
    }

    HierarchyHelpers::ExpandParents(m_hierarchy, m_entries);

    m_hierarchy->clearSelection();
    HierarchyHelpers::SetSelectedItems(m_hierarchy, &m_entries);
}

void CommandHierarchyItemCreate::Push(UndoStack* stack,
    HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    PostCreationCallback postCreationCB)
{
    if (stack->GetIsExecuting())
    {
        // This is a redundant Qt notification.
        // Nothing else to do.
        return;
    }

    stack->push(new CommandHierarchyItemCreate(stack,
            hierarchy,
            SelectionHelpers::GetSelectedElementIds(hierarchy,
                selectedItems,
                true
                ),
            postCreationCB));
}
