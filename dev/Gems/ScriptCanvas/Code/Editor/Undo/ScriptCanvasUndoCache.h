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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_map.h>

namespace ScriptCanvasEditor
{
    // This cache maintains the previous state of the Script Canvas graph items recorded for Undo
    class UndoCache
    {
    public:
        AZ_CLASS_ALLOCATOR(UndoCache, AZ::SystemAllocator, 0);

        UndoCache();
        ~UndoCache();

        // Update the graph item within the cache
        void UpdateCache(AZ::EntityId scriptCanvasEntityId);

        // remove the graph item from the cache
        void PurgeCache(AZ::EntityId scriptCanvasEntityId);

        // retrieve the last known state for the graph item
        const AZStd::vector<AZ::u8>& Retrieve(AZ::EntityId scriptCanvasEntityId);

        // Populate the cache from a ScriptCanvas Entity graph entity
        void PopulateCache(AZ::Entity* scriptCanvasEntity);
        // clear the entire cache:
        void Clear();
    protected:
        AZStd::unordered_map <AZ::EntityId, AZStd::vector<AZ::u8>> m_dataMap; ///< Stores an Entity Id to serialized graph data(Node/Connection) map

        AZStd::vector<AZ::u8> m_emptyData;
    };
}