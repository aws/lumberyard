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
#include "EditorUI_QT_Precompiled.h"

#include "ItemStateCommand.h"
#include "ItemPreemptiveUndoCache.h"

#include <EditorUI_QTDLLBus.h>

#include <AzCore/std/smart_ptr/make_shared.h>

#include <Include/IBaseLibraryManager.h>

namespace EditorUIPlugin
{
    ItemStateCommand::ItemStateCommand(const AZStd::string& friendlyName)
        : UndoSystem::URSequencePoint(friendlyName, 0)
        , m_lodIdx(-1)
        , m_selected(false)
    {
    }

    ItemStateCommand::~ItemStateCommand()
    {
    }

    void ItemStateCommand::Capture(const AZStd::string& itemId, bool selected, int lodIdx)
    {
        m_itemId = itemId;
        m_lodIdx = lodIdx;
        m_selected = selected;
        EBUS_EVENT_RESULT(m_undoState, EditorUIPlugin::LibraryItemCacheRequests::Bus, RetrieveItem, m_itemId);
        EBUS_EVENT(EditorUIPlugin::LibraryItemCacheRequests::Bus, UpdateItemCache, m_itemId);
        EBUS_EVENT_RESULT(m_redoState, EditorUIPlugin::LibraryItemCacheRequests::Bus, RetrieveItem, m_itemId);
    }

    void ItemStateCommand::RestoreItem(XmlNodeRef savedData) const
    {
        if (savedData == nullptr)
        {
            return;
        }

        //try find the item
        IDataBaseItem* libItem = nullptr;
        EBUS_EVENT_RESULT(libItem, EditorLibraryUndoRequestsBus, GetItem, m_itemId);
        
        if (libItem == nullptr)
        {
            AZ_Assert(false, "Failed to add particle item %s", m_itemId.c_str());
        }

        IDataBaseItem::SerializeContext loadCtx;
        loadCtx.bIgnoreChilds = true;
        loadCtx.bLoading = true;
        loadCtx.bUniqName = false;
        loadCtx.bCopyPaste = true;
        loadCtx.bUndo = true;
        loadCtx.node = savedData;

        libItem->Serialize(loadCtx);

        EBUS_EVENT(EditorUIPlugin::LibraryItemCacheRequests::Bus, UpdateItemCache, m_itemId);
        
        EBUS_EVENT(LibraryItemUIRequests::Bus, UpdateItemUI, m_itemId, m_selected, m_lodIdx);
    }

    void ItemStateCommand::Undo()
    {
        RestoreItem(m_undoState);
    }

    void ItemStateCommand::Redo()
    {
        RestoreItem(m_redoState);
    }

    ItemGroupChangeCommand::ItemGroupChangeCommand(const AZStd::string& friendlyName)
        : UndoSystem::URSequencePoint(friendlyName, 0)
    {

    }
    
    ItemGroupChangeCommand::~ItemGroupChangeCommand()
    {
    }
    
    void ItemGroupChangeCommand::Undo()
    {
        for (auto cmd = m_commands.begin(); cmd != m_commands.end(); cmd++)
        {
            (*cmd)->Undo();
        }
    }

    void ItemGroupChangeCommand::Redo()
    {
        for (auto cmd = m_commands.begin(); cmd != m_commands.end(); cmd++)
        {
            (*cmd)->Redo();
        }
    }

    void ItemGroupChangeCommand::Capture(const AZStd::list<AZStd::string>& itemIds)
    {
        for (auto itemId = itemIds.begin(); itemId != itemIds.end(); itemId ++)
        {
            ItemStateCommandPtr cmdPtr = AZStd::make_shared<ItemStateCommand>(*itemId);
            cmdPtr->Capture(*itemId, false, -1);
            m_commands.push_back(cmdPtr);
        }
    }
   
} // namespace EditorUIPlugin
