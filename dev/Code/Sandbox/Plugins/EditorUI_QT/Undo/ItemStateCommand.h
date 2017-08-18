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
#ifndef EDITORUIPLUGIN_ITEMSTATECOMMAND_H
#define EDITORUIPLUGIN_ITEMSTATECOMMAND_H

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

#pragma once

using namespace AzToolsFramework;

namespace EditorUIPlugin
{
    // The Item State URSequencePoint stores the state of an particle item before and after some change to it.
    // it does so by serializing the particle item, so its a good "default behavior" that cannot miss any particular change.
    // this Item State Command should be able to capture everything in its entirety.
    class ItemStateCommand
        : public UndoSystem::URSequencePoint
    {
    public:
        AZ_RTTI(ItemStateCommand, "{4AD0180B-D15A-410E-B6C1-E0C86E5E2486}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(ItemStateCommand, AZ::SystemAllocator, 0);

        ItemStateCommand(const AZStd::string& friendlyName);
        virtual ~ItemStateCommand();

        void Undo() override;
        void Redo() override;

        // capture the initial state - this fills the undo with the initial data if captureUndo is true
        // otherwise is captures the final state.
        void Capture(const AZStd::string& itemId, bool selected, int lodIdx);

    protected:

        void RestoreItem(XmlNodeRef savedData) const;

        AZStd::string m_itemId;                                ///< The id of the captured item (the path name is the id we can use at this moment)
        int m_lodIdx;
        bool m_selected;

        XmlNodeRef m_undoState;
        XmlNodeRef m_redoState;

        // DISABLE COPY
        ItemStateCommand(const ItemStateCommand& other) = delete;
        const ItemStateCommand& operator= (const ItemStateCommand& other) = delete;
    };

    //  This is for undo of a group of items in special circumstances, where it must apply undo and redo steps in the same order
    // (normally batched undo and redo steps are done in opposite order)
    // The use case we have this requirement for is the lod for particle items.
    class ItemGroupChangeCommand
        : public UndoSystem::URSequencePoint
    {
        typedef AZStd::shared_ptr<ItemStateCommand> ItemStateCommandPtr;

    public:
        AZ_RTTI(ItemGroupChangeCommand, "{DB094B8A-DC44-4360-9B96-02C0D0907741}", UndoSystem::URSequencePoint);
        AZ_CLASS_ALLOCATOR(ItemGroupChangeCommand, AZ::SystemAllocator, 0);

        ItemGroupChangeCommand(const AZStd::string& friendlyName);
        virtual ~ItemGroupChangeCommand();

        void Undo() override;
        void Redo() override;

        void Capture(const AZStd::list<AZStd::string>& itemIds);

    protected:
        AZStd::list<ItemStateCommandPtr> m_commands;

        // DISABLE COPY
        ItemGroupChangeCommand(const ItemGroupChangeCommand& other) = delete;
        const ItemGroupChangeCommand& operator= (const ItemGroupChangeCommand& other) = delete;
    };

} // namespace EditorUIPlugin

#endif // EDITORUIPLUGIN_ITEMSTATECOMMAND_H
