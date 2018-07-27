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
#ifndef EDITORUIPLUGIN_ITEMPREEMPTIVEUNDOCACHE_H
#define EDITORUIPLUGIN_ITEMPREEMPTIVEUNDOCACHE_H

#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

#include <Include/IDataBaseItem.h>

namespace EditorUIPlugin
{
    // The purpose of the preemptive Undo Cache is to keep a snapshot of the last known state of any undoable library item (not including children) 
    // so that the user does not have to inform us before they make a change, only after they make a change.
    // It also allows us to detect errors with change notification and not have to do multiple snapshots (before and after).
    // Any item hierarchy change, lod add/remove or rename change will be handled in library modification undo
    class ItemPreemptiveUndoCache
    {
    public:
        AZ_CLASS_ALLOCATOR(ItemPreemptiveUndoCache, AZ::SystemAllocator, 0);

        ItemPreemptiveUndoCache(IBaseLibraryManager* libMgr);
        ~ItemPreemptiveUndoCache();
                
        // Store the new particle item or replace the old state
        void UpdateCache(const AZStd::string& itemId);

        // remove the cache for the particle item, if there is one
        void PurgeCache(const AZStd::string& itemId);

        // retrieve the last known state for an entity
        XmlNodeRef Retrieve(const AZStd::string& itemId);

        // clear the entire cache:
        void Clear();
    protected:
        typedef AZStd::unordered_map<AZStd::string, XmlNodeRef> LibItemMap;

        IDataBaseItem::SerializeContext m_saveContext;

        LibItemMap m_LibItemStateMap;

        XmlNodeRef m_Empty;

        struct IBaseLibraryManager* m_libMgr;
    };
} // namespace EditorUIPlugin

#pragma once

#endif //EDITORUIPLUGIN_ITEMPREEMPTIVEUNDOCACHE_H
