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

#include <AzFramework/Physics/PhysicsComponentBus.h>


namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    struct GotShotParams
    {
        AZ_TYPE_INFO(GotShotParams, "{BC1EA56B-4099-41E7-BCE3-2163BBE26D04}");
        AZ_CLASS_ALLOCATOR(GotShotParams, AZ::SystemAllocator, 0);

        GotShotParams()
            : m_damage(0.0f)
            , m_immediatelyRagdoll(false)
        {}

        float m_damage;
        AZ::Vector3 m_direction;
        AZ::EntityId m_assailant;
        bool m_immediatelyRagdoll;
    };

    struct InteractParams
    {
        AZ_TYPE_INFO(InteractParams, "{1A8A5460-8D8A-4FC9-A31A-06C7349DD427}");
        AZ_CLASS_ALLOCATOR(InteractParams, AZ::SystemAllocator, 0);

        InteractParams()
        {}

        AZ::EntityId m_positionEntity;
        AZ::EntityId m_cameraEntity;
    };

    struct PlayAnimParams
    {
        AZ_TYPE_INFO(PlayAnimParams, "{CF1E77F0-A906-4468-9EA1-71A79C9D7325}");
        AZ_CLASS_ALLOCATOR(PlayAnimParams, AZ::SystemAllocator, 0);

        PlayAnimParams()
            : m_animName("")
            , m_loop(true)
        {}

        AZStd::string m_animName;
        bool m_loop;
    };

    struct CompanionPOIParams
    {
        AZ_TYPE_INFO(CompanionPOIParams, "{398AD4C3-15D1-4755-8BA1-EAA17AE63F90}");
        AZ_CLASS_ALLOCATOR(CompanionPOIParams, AZ::SystemAllocator, 0);

        CompanionPOIParams()
            : m_reset(true)
        {}

        bool m_reset;           // if true, return to normal positioning
        AZ::EntityId m_position;
        AZ::EntityId m_lookAt;
    };

    /*!
    * Wrapper for utility functions exposed to Lua for StarterGame.
    */
    class StarterGameUtility
    {
    public:
        AZ_TYPE_INFO(StarterGameUtility, "{E8AD1E8A-A67D-44EB-8A08-B6881FD72F2F}");
        AZ_CLASS_ALLOCATOR(StarterGameUtility, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflection);

        static bool IsGameStarted();
        static void RestartLevel(const bool& fade);

        static AZ::Transform GetJointWorldTM(const AZ::EntityId& entityId, const AZStd::string& bone);

        static AzFramework::PhysicsComponentNotifications::Collision CreatePseudoCollisionEvent(const AZ::EntityId& entity, const AZ::Vector3& position, const AZ::Vector3& normal, const AZ::Vector3& direction);

        static int GetSurfaceFromRayCast(const AZ::Vector3& pos, const AZ::Vector3& direction);
        static AZStd::sys_time_t GetTimeNowMicroSecond();

        static AZ::Uuid GetUuid(const AZStd::string& className);
        static bool IsLegacyCharacter(const AZ::EntityId& entityId);

    private:
        static unsigned int EntFromEntityTypes(AZ::u32 types);
    };
} // namespace StarterGameGem
