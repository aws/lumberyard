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

#ifdef LY_TERRAIN_RUNTIME

#pragma once

#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

//
// Terrain::LRUCacheQueue
// Utility class for managing an LRU cache
// Keeps track of the least-recently-used PhysicalType cache entries
//

namespace Terrain
{
    template< typename PhysicalType >
    class LRUCacheQueue
    {
    public:
        LRUCacheQueue()
        {
        }

        size_t GetCacheSize()
        {
            return m_lruQueue.size();
        }

        // Add cache entries for the queue to manage
        void AddCacheEntry(const PhysicalType& physicalValue)
        {
            m_lruQueue.push_back(physicalValue);
            PhysicalType& physicalValueRef = m_lruQueue.back();

            // track queue iterator per object for easy management
            typename LRUQueue::iterator physicalValueIter = --m_lruQueue.end();
            m_lruQueueIteratorMap.insert(IteratorPair(physicalValueRef, physicalValueIter));
        }

        // Reference the physical entry, hinting at the cache queue which PhysicalType objects are used
        void ReferenceCacheEntry(const PhysicalType& physicalValue)
        {
            // Retrieve the list iterator for the physical tile
            typename LRUQueueIteratorMap::iterator iteratorMapIter = m_lruQueueIteratorMap.find(physicalValue);

            // These map entries should always exist
            AZ_Assert(iteratorMapIter != m_lruQueueIteratorMap.end(), "VTWrapper | Cache data structures in invalid state!");

            // Use the iterator, move the physical tile to the back of the queue
            typename LRUQueue::iterator& lruQueueIter = iteratorMapIter->second;
            m_lruQueue.splice(m_lruQueue.end(), m_lruQueue, lruQueueIter);
        }

        // Returns reference to the next LRU cache entry
        PhysicalType& PeekNextLRUCacheEntry()
        {
            return m_lruQueue.front();
        }

        void Clear()
        {
            m_lruQueue.clear();
            m_lruQueueIteratorMap.clear();
        }
    private:
        // LRU Cache: we keep the LRU physical tile at the head of the queue
        typedef AZStd::list<PhysicalType> LRUQueue;
        LRUQueue m_lruQueue;

        // LRU Cache queue management: hold mapping from PhysicalType objects to their respective iterators in the LRUQueue
        // In order to quickly manipulate the LRUQueue, we cache the iterators for quick access
        typedef AZStd::pair<PhysicalType, typename AZStd::list<PhysicalType>::iterator> IteratorPair;
        typedef AZStd::unordered_map<PhysicalType, typename AZStd::list<PhysicalType>::iterator> LRUQueueIteratorMap;
        LRUQueueIteratorMap m_lruQueueIteratorMap;
    };
}

#endif
