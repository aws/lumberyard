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

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/string/string.h>


namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    /*!
    * Wrapper for entity utility functions exposed to Lua for StarterGame.
    */
    class StarterGameEntityUtility
    {
    public:
        AZ_TYPE_INFO(StarterGameEntityUtility, "{54761E81-5476-4047-BA09-CC0CF76FB6F3}");
        AZ_CLASS_ALLOCATOR(StarterGameEntityUtility, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflection);

        static void GetEntitysVelocity(const AZ::EntityId& entityId, AZ::Vector3& velocity);
        static bool GetEntitysVelocityLegacy(const AZ::EntityId& entityId, AZ::Vector3& velocity);
        static AZ::EntityId GetParentEntity(const AZ::EntityId& entityId);
        static AZStd::string GetEntityName(const AZ::EntityId& entityId);
        static bool EntityHasTag(const AZ::EntityId& entityId, const AZStd::string& tag);
        static bool EntityHasComponent(const AZ::EntityId& entityId, const AZStd::string& componentName);
        static bool AddTagComponentToEntity(const AZ::EntityId& entityId);
        static void SetCharacterHalfHeight(const AZ::EntityId& entityId, float halfHeight);
    };
} // namespace StarterGameGem
