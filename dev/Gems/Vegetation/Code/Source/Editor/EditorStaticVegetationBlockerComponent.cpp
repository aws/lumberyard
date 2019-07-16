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

#include "Vegetation_precompiled.h"
#include "EditorStaticVegetationBlockerComponent.h"
#include <MathConversion.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>

#include <ISystem.h>
#include <I3DEngine.h>


namespace Vegetation
{
    void EditorStaticVegetationBlockerComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectSubClass<EditorStaticVegetationBlockerComponent, BaseClassType>(context, 1, &EditorVegetationComponentBaseVersionConverter<typename BaseClassType::WrappedComponentType, typename BaseClassType::WrappedConfigType>);
    }

    void EditorStaticVegetationBlockerComponent::Activate()
    {
        BaseClassType::Activate();
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
    }

    void EditorStaticVegetationBlockerComponent::Deactivate()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        BaseClassType::Deactivate();
    }

    void EditorStaticVegetationBlockerComponent::DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        ISystem* sys = GetISystem();

        if (!m_configuration.m_showBoundingBox && !m_configuration.m_showBlockedPoints) // nothing to do here.
        {
            return;
        }


        if (!sys || !sys->GetI3DEngine())
        {
            return;
        }

        AZ::Vector3 cameraPos = LYVec3ToAZVec3(sys->GetI3DEngine()->GetRenderingCamera().GetPosition());
        AZ::Vector2 cameraPos2d(cameraPos.GetX(), cameraPos.GetY());

        if (m_configuration.m_showBoundingBox)
        {
            MapHandle mapHandle;
            StaticVegetationRequestBus::BroadcastResult(mapHandle, &StaticVegetationRequestBus::Events::GetStaticVegetation);
            if(mapHandle)
            {
                DrawVegetationBoundingBoxes(mapHandle.Get(), cameraPos2d, debugDisplay);
            }
        }

        if (m_configuration.m_showBlockedPoints)
        {
            DrawBlockedClaimPoints(cameraPos2d, debugDisplay);
        }
    }

    void EditorStaticVegetationBlockerComponent::DrawVegetationBoundingBoxes(const StaticVegetationMap* vegetationList, AZ::Vector2& cameraPos2d, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        static const AZ::Color s_green = AZ::Color(0.3f, 0.9f, 0.3f, .05f);
        static const AZ::Color s_greenOutline = AZ::Color(0.3f, 0.9f, 0.3f, 1);

        AZ::Aabb aabb = m_component.GetEncompassingAabb();

        for (const auto& pair : *vegetationList)
        {
            auto vegAabb = StaticVegetationBlockerComponent::GetPaddedAabb(pair.second.m_aabb, m_configuration.m_globalPadding);
            AZ::Vector2 aabbPos2d(vegAabb.GetCenter().GetX(), vegAabb.GetCenter().GetY());
            float distanceToCamera = cameraPos2d.GetDistance(aabbPos2d);

            if (distanceToCamera <= m_configuration.m_maxDebugRenderDistance && vegAabb.Overlaps(aabb))
            {
                debugDisplay.SetColor(s_green);
                debugDisplay.DrawSolidBox(vegAabb.GetMin(), vegAabb.GetMax());

                debugDisplay.SetColor(s_greenOutline);
                debugDisplay.DrawWireBox(vegAabb.GetMin(), vegAabb.GetMax());
            }
        }
    }

    void EditorStaticVegetationBlockerComponent::DrawBlockedClaimPoints(AZ::Vector2& cameraPos2d, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        static const AZ::Color s_blue = AZ::Color(0.3f, 0.3f, 0.9f, .1f);
        static const AZ::Color s_blueOutline = AZ::Color(0.3f, 0.3f, 0.9f, 1);
        float pointsPerMeter = 1.0;

        GradientSignal::SectorDataRequestBus::Broadcast(&GradientSignal::SectorDataRequestBus::Events::GetPointsPerMeter, pointsPerMeter);

        AZ::Aabb boundsAabb = m_component.GetCachedBounds();
        AZ::Vector3 minBoundsPoint, maxBoundsPoint, boundsExtents;
        StaticVegetationBlockerComponent::GetBoundsPointsAndExtents(boundsAabb, pointsPerMeter, minBoundsPoint, maxBoundsPoint, boundsExtents);

        int xAxisPoints = StaticVegetationBlockerComponent::MetersToPoints(boundsExtents.GetX(), pointsPerMeter) + 1; // +1 because we want the number of points, not the index of the last point
        int yAxisPoints = StaticVegetationBlockerComponent::MetersToPoints(boundsExtents.GetY(), pointsPerMeter) + 1;
        int size = xAxisPoints * yAxisPoints;

        // Try to take the lock, if we can't then the map is being updated, which can take a while, so don't bother waiting and just skip this draw attempt
        AZStd::unique_lock<decltype(m_component.m_vegetationMapMutex)> scopedLock(m_component.m_vegetationMapMutex, AZStd::try_to_lock_t{});

        if (xAxisPoints == 0 || yAxisPoints == 0 || size != m_component.m_vegetationMap.size() || !scopedLock.owns_lock())
        {
            return;
        }

        auto viewAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3(cameraPos2d.GetX(), cameraPos2d.GetY(), 0), AZ::Vector3(m_configuration.m_maxDebugRenderDistance, m_configuration.m_maxDebugRenderDistance, FLT_MAX));

        StaticVegetationBlockerComponent::IterateIntersectingPoints(viewAabb, boundsAabb, pointsPerMeter, [this, &debugDisplay](int index, const AZ::Aabb& pointAabb)
        {
            if (index < m_component.m_vegetationMap.size() && m_component.m_vegetationMap[index].m_blocked)
            {
                // Expand the aabb slightly on the Z axis to avoid Z-fighting with the area box
                AZ::Vector3 zOffset(0, 0, 0.05f);

                debugDisplay.SetColor(s_blue);
                debugDisplay.DrawSolidBox(pointAabb.GetMin() - zOffset, pointAabb.GetMax() + zOffset);

                debugDisplay.SetColor(s_blueOutline);
                debugDisplay.DrawWireBox(pointAabb.GetMin() - zOffset, pointAabb.GetMax() + zOffset);
            }
        });
    }
}