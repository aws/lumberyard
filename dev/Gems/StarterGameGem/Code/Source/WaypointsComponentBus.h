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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector4.h>


namespace StarterGameGem
{
    typedef AZStd::vector<AZ::EntityId> VecOfEntityIds;

    struct WaypointsConfiguration
    {
        AZ_CLASS_ALLOCATOR(WaypointsConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(WaypointsConfiguration, "{36B50649-8905-4D40-906E-8A916A2B4EFC}");

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<WaypointsConfiguration>()
                    ->Version(1)
                    ->Field("IsSentry", &WaypointsConfiguration::m_isSentry)
                    ->Field("IsLazySentry", &WaypointsConfiguration::m_isLazySentry)
                    ->Field("Waypoints", &WaypointsConfiguration::m_waypoints)
                    ->Field("CurrentWaypoint", &WaypointsConfiguration::m_currentWaypoint)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<WaypointsConfiguration>("Waypoints Configuration", "Waypoint configuration parameters")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "AI")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/SG_Icon.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/SG_Icon.dds")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->DataElement(0, &WaypointsConfiguration::m_isSentry, "Sentry?", "If ticked, will ignore waypoints and periodically turn on the spot.")
                        ->DataElement(0, &WaypointsConfiguration::m_isLazySentry, "Lazy Sentry?", "If ticked, will become sentry that doesn't turn (prioritised over 'Sentry?').")
                        ->DataElement(0, &WaypointsConfiguration::m_waypoints, "Waypoints", "A list of waypoints.")
                        ;
                }
            }
        }

        WaypointsConfiguration()
        {
            m_isSentry = false;
            m_isLazySentry = false;
            m_currentWaypoint = 0;
        }

        virtual ~WaypointsConfiguration() = default;

        int NumberOfValidWaypoints() const
        {
            int count = 0;
            for (VecOfEntityIds::const_iterator it = m_waypoints.begin(); it != m_waypoints.end(); ++it)
            {
                if (it->IsValid())
                {
                    ++count;
                }
            }

            return count;
        }

        bool m_isSentry;
        bool m_isLazySentry;
        VecOfEntityIds m_waypoints;
        AZ::u32 m_currentWaypoint;
    };

    /*!
    * WaypointsComponentRequests
    * Messages serviced by the WaypointsComponent
    */
    class WaypointsComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~WaypointsComponentRequests() {}

        //! Gets the first waypoint in the list.
        virtual AZ::EntityId GetFirstWaypoint() = 0;
        //! Gets the current waypoint in the list.
        virtual AZ::EntityId GetCurrentWaypoint() { return AZ::EntityId(); }
        //! Gets the next waypoint in the list.
        virtual AZ::EntityId GetNextWaypoint() = 0;

        //! Gets the number of waypoints in the list.
        virtual int GetWaypointCount() = 0;

        //! Gets whether or not the owner of this component should be a sentry.
        virtual bool IsSentry() = 0;
        //! Gets whether or not the owner of this component should be a lazy sentry.
        virtual bool IsLazySentry() = 0;

        //! Clones the waypoints from the specified entity into this component.
        virtual void CloneWaypoints(const AZ::EntityId& srcEntityId) = 0;
        //! Get the waypoint component.
        virtual VecOfEntityIds* GetWaypoints() { return nullptr; }
    };

    using WaypointsComponentRequestsBus = AZ::EBus<WaypointsComponentRequests>;

    static const AZ::Vector4    s_waypointsDebugLineColourVec = AZ::Vector4(1.0f, 1.0f, 0.509f, 1.0f);
    static const ColorB         s_waypointsDebugLineColour = ColorB(    uint8(s_waypointsDebugLineColourVec.GetX() * 255.0f),
                                                                        uint8(s_waypointsDebugLineColourVec.GetY() * 255.0f),
                                                                        uint8(s_waypointsDebugLineColourVec.GetZ() * 255.0f),
                                                                        uint8(s_waypointsDebugLineColourVec.GetW() * 255.0f));

} // namespace StarterGameGem