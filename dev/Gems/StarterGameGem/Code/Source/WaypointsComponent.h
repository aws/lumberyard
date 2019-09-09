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

#include "WaypointsComponentBus.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <MathConversion.h>

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>


namespace AZ
{
    class ReflectContext;
}

namespace StarterGameGem
{
    class WaypointsComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private WaypointsComponentRequestsBus::Handler
    {
    public:
        friend class EditorWaypointsComponent;

        AZ_COMPONENT(WaypointsComponent, "{D3830430-7D36-4BC5-8E91-11CA817A366B}");

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("WaypointsService", 0x863c80b2));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("WaypointsService", 0x863c80b2));
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // WaypointsComponentRequestsBus::Handler
        AZ::EntityId GetFirstWaypoint() override;
        AZ::EntityId GetCurrentWaypoint() override;
        AZ::EntityId GetNextWaypoint() override;

        int GetWaypointCount() override;

        bool IsSentry() override;
        bool IsLazySentry() override;

        void CloneWaypoints(const AZ::EntityId& srcEntityId) override;
        VecOfEntityIds* GetWaypoints() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        WaypointsConfiguration m_config;
    };
} // namespace StarterGameGem
