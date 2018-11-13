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

#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>


namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    /*!
    * Wrapper for A.I. utility functions exposed to Lua for StarterGame.
    */
    class StarterGameAIUtility
    {
    public:
        AZ_TYPE_INFO(StarterGameAIUtility, "{99787330-BF1F-4B86-B53B-C5CC3FF5B23F}");
        AZ_CLASS_ALLOCATOR(StarterGameAIUtility, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflection);

        static bool IsOnNavMesh(const AZ::Vector3& pos);
        static bool GetClosestPointInNavMesh(const AZ::Vector3& pos, AZ::Vector3& found);
        static float GetArrivalDistanceThreshold(const AZ::EntityId& entityId);
    };
} // namespace StarterGameGem
