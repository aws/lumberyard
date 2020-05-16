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

#include <Include/IDataBaseItem.h>
#include <Include/IDataBaseLibrary.h>
#include <Include/IBaseLibraryManager.h>

#include "ItemPreemptiveUndoCache.h"

namespace EditorUIPlugin
{
    ItemPreemptiveUndoCache::ItemPreemptiveUndoCache(IBaseLibraryManager* libMgr)
        : m_libMgr(libMgr)
    {
        m_saveContext.bIgnoreChilds = true;
        m_saveContext.bLoading = false; //saving
        m_saveContext.bUniqName = false;
        m_saveContext.bCopyPaste = true;
        m_saveContext.bUndo = true;
    }

    ItemPreemptiveUndoCache::~ItemPreemptiveUndoCache()
    {
    }
    
    void ItemPreemptiveUndoCache::Clear()
    {
        m_LibItemStateMap.clear();
    }

    void ItemPreemptiveUndoCache::PurgeCache(const AZStd::string& itemId)
    {
        m_LibItemStateMap.erase(itemId);
    }

    void ItemPreemptiveUndoCache::UpdateCache(const AZStd::string& itemId)
    {
        // capture it
        //find the item
        IDataBaseItem *libItem = m_libMgr->FindItemByName(itemId.c_str());
        if (libItem == nullptr)
        {
            PurgeCache(itemId);
            return;
        }

        //save the libItem to save context
        m_saveContext.node = GetIEditor()->GetSystem()->CreateXmlNode(libItem->GetName().toUtf8().data());
        libItem->Serialize(m_saveContext);
        
        // save the item
        m_LibItemStateMap[itemId] = m_saveContext.node;
        m_saveContext.node = nullptr;
    }
    
    XmlNodeRef ItemPreemptiveUndoCache::Retrieve(const AZStd::string& itemId)
    {
        auto it = m_LibItemStateMap.find(itemId);

        if (it == m_LibItemStateMap.end())
        {
            return m_Empty;
        }

        return it->second;
    }

} // namespace EditorUIPlugin
