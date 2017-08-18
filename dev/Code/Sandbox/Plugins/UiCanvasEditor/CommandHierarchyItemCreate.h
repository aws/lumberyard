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
#pragma once

class CommandHierarchyItemCreate
    : public QUndoCommand
{
public:

    using PostCreationCallback = std::function<void(AZ::Entity* element)>;

    void undo() override;
    void redo() override;

    static void Push(UndoStack* stack,
        HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        PostCreationCallback postCreationCB = [](AZ::Entity* element){});

private:

    CommandHierarchyItemCreate(UndoStack* stack,
        HierarchyWidget* hierarchy,
        EntityHelpers::EntityIdList& parents,
        PostCreationCallback postCreationCB);

    UndoStack* m_stack;

    HierarchyWidget* m_hierarchy;
    EntityHelpers::EntityIdList m_parents;
    SerializeHelpers::SerializedEntryList m_entries;
    PostCreationCallback m_postCreationCB;
};
