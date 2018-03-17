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

#include "StarterGameGem_precompiled.h"
#include "WaypointsComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>

#include <IRenderAuxGeom.h>


namespace StarterGameGem
{
    void WaypointsComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
        WaypointsComponentRequestsBus::Handler::BusConnect(GetEntityId());
    }

    void WaypointsComponent::Deactivate()
    {
        WaypointsComponentRequestsBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void WaypointsComponent::Reflect(AZ::ReflectContext* reflection)
    {
        WaypointsConfiguration::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<WaypointsComponent>()
                ->Version(1)
                ->Field("Config", &WaypointsComponent::m_config)
            ;
        }

        if (AZ::BehaviorContext* behavior = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behavior->EBus<WaypointsComponentRequestsBus>("WaypointsComponentRequestsBus")
                ->Event("GetFirstWaypoint", &WaypointsComponentRequestsBus::Events::GetFirstWaypoint)
                ->Event("GetNextWaypoint", &WaypointsComponentRequestsBus::Events::GetNextWaypoint)
                ->Event("GetWaypointCount", &WaypointsComponentRequestsBus::Events::GetWaypointCount)
                ->Event("IsSentry", &WaypointsComponentRequestsBus::Events::IsSentry)
                ->Event("IsLazySentry", &WaypointsComponentRequestsBus::Events::IsLazySentry)
                ->Event("CloneWaypoints", &WaypointsComponentRequestsBus::Events::CloneWaypoints)
            ;
        }
    }

    void WaypointsComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        IRenderAuxGeom* renderAuxGeom = gEnv->pRenderer->GetIRenderAuxGeom();
        SAuxGeomRenderFlags flags;
        flags.SetAlphaBlendMode(e_AlphaNone);
        flags.SetCullMode(e_CullModeBack);
        flags.SetDepthTestFlag(e_DepthTestOn);
        flags.SetFillMode(e_FillModeSolid);
        renderAuxGeom->SetRenderFlags(flags);

        static ICVar* const cvarDrawWaypoints= gEnv->pConsole->GetCVar("ai_debugDrawWaypoints");
        if (cvarDrawWaypoints)
        {
            if (cvarDrawWaypoints->GetFVal() != 0 && m_config.m_waypoints.size() >= 2)
            {
                AZ::Vector3 raised = AZ::Vector3(0.0f, 0.0, cvarDrawWaypoints->GetFVal());

                Vec3* positions = new Vec3[m_config.m_waypoints.size()];
                int i = 0;
                for (i; i < m_config.m_waypoints.size(); ++i)
                {
                    AZ::Transform tm = AZ::Transform::CreateIdentity();
                    AZ::TransformBus::EventResult(tm, m_config.m_waypoints[i], &AZ::TransformInterface::GetWorldTM);
                    positions[i] = AZVec3ToLYVec3(tm.GetTranslation() + raised);
                }

                renderAuxGeom->DrawPolyline(positions, i, (i > 2), s_waypointsDebugLineColour);
            }
        }
    }

    AZ::EntityId WaypointsComponent::GetFirstWaypoint()
    {
        AZ::EntityId res;
        if (m_config.m_waypoints.size() > 0)
        {
            res = m_config.m_waypoints[0];
            m_config.m_currentWaypoint = 0;
        }
        else
        {
            AZ_Assert("AI", "WaypointsComponent: Tried to get the first waypoint but no waypoints exist.");
        }

        return res;
    }

    AZ::EntityId WaypointsComponent::GetCurrentWaypoint()
    {
        return m_config.m_waypoints[m_config.m_currentWaypoint];
    }

    AZ::EntityId WaypointsComponent::GetNextWaypoint()
    {
        ++m_config.m_currentWaypoint;

        AZ::EntityId res;
        if (m_config.m_currentWaypoint >= m_config.m_waypoints.size())
        {
            // This function should reset the waypoint counter.
            res = GetFirstWaypoint();
        }
        else
        {
            res = m_config.m_waypoints[m_config.m_currentWaypoint];
        }

        return res;
    }

    int WaypointsComponent::GetWaypointCount()
    {
        return m_config.m_waypoints.size();
    }

    bool WaypointsComponent::IsSentry()
    {
        return m_config.m_isSentry && !m_config.m_isLazySentry;
    }

    bool WaypointsComponent::IsLazySentry()
    {
        return m_config.m_isLazySentry;
    }

    void WaypointsComponent::CloneWaypoints(const AZ::EntityId& srcEntityId)
    {
        AZ_Assert(srcEntityId.IsValid(), "WaypointsComponent: 'CloneWaypoints()' was given a bad entity ID.");

        bool res = false;
        WaypointsComponentRequestsBus::EventResult(res, srcEntityId, &WaypointsComponentRequestsBus::Events::IsSentry);
        m_config.m_isSentry = res;

        WaypointsComponentRequestsBus::EventResult(res, srcEntityId, &WaypointsComponentRequestsBus::Events::IsLazySentry);
        m_config.m_isLazySentry = res;

        // Only bother copying the waypoints if we're not a sentry.
        if (!this->IsSentry() && !this->IsLazySentry())
        {
            VecOfEntityIds* waypoints;
            WaypointsComponentRequestsBus::EventResult(waypoints, srcEntityId, &WaypointsComponentRequestsBus::Events::GetWaypoints);
            m_config.m_waypoints = *waypoints;

            // Clean up the vector so we don't have empty or invalid entries.
            for (VecOfEntityIds::iterator it = m_config.m_waypoints.begin(); it != m_config.m_waypoints.end(); )
            {
                if (!(*it).IsValid())
                {
                    it = m_config.m_waypoints.erase(it);
                }
                else
                {
                    ++it;
                }
            }

            // If we didn't copy any waypoints then assign ourselves as a lazy sentry.
            if (m_config.m_waypoints.size() == 0)
            {
                m_config.m_isLazySentry = true;
            }
        }

        // If we don't have any waypoints then use the source entity as our only waypoint. This is
        // purely so that we have somewhere to go back to if we're drawn away for combat.
        if (m_config.m_waypoints.size() == 0)
        {
            m_config.m_waypoints.push_back(srcEntityId);
        }
    }

    VecOfEntityIds* WaypointsComponent::GetWaypoints()
    {
        return &m_config.m_waypoints;
    }

}
