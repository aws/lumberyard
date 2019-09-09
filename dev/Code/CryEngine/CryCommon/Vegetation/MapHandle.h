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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include "IEntityRenderState.h"

namespace Vegetation
{
    struct StaticVegetationData
    {
        AZ::Vector3 m_position;
        AZ::Aabb m_aabb;
    };

    using StaticVegetationMap = AZStd::unordered_map<void*, StaticVegetationData>;

    struct MapHandle
    {
    public:
        MapHandle() = default;

        MapHandle(const StaticVegetationMap* map, AZ::u64 updateCount, AZStd::shared_mutex* mapMutex) :
            m_map(map), m_updateCount(updateCount), m_readMutex(mapMutex)
        {
            if (m_readMutex)
            {
                m_readMutex->lock_shared();
            }
        }

        MapHandle(const MapHandle&) = delete; 

        MapHandle(MapHandle&& rhs) :
            m_updateCount(AZStd::move(rhs.m_updateCount)),
            m_map(AZStd::move(rhs.m_map)),
            m_readMutex(nullptr)
        {
            AZStd::swap(m_readMutex, rhs.m_readMutex); // assumes the rhs.m_readMutex is already locked!
            rhs.m_map = nullptr;
            rhs.m_updateCount = 0;
        }

        ~MapHandle()
        {
            if (m_readMutex)
            {
                m_readMutex->unlock_shared();
            }
        }

        MapHandle& operator=(const MapHandle&) = delete;

        MapHandle& operator=(MapHandle&& rhs)
        {
            m_updateCount = AZStd::move(rhs.m_updateCount);
            m_map = AZStd::move(rhs.m_map);
            if (m_readMutex)
            {
                m_readMutex->unlock_shared();
            }
            m_readMutex = AZStd::move(rhs.m_readMutex);
            rhs.m_readMutex = nullptr;
            rhs.m_map = nullptr;
            rhs.m_updateCount = 0;
            return *this;
        }

        const StaticVegetationMap* Get()
        {
            return m_readMutex?m_map:nullptr;
        }

        AZ::u64 GetUpdateCount() const
        {
            return m_updateCount;
        }

        operator bool() const
        {
            return m_map != nullptr && m_readMutex != nullptr;
        }

    private:
        const StaticVegetationMap* m_map = nullptr;
        AZ::u64 m_updateCount = 0;
        AZStd::shared_mutex* m_readMutex = nullptr;
    };
}