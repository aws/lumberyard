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

#include "ManipulatorSnapping.h"

#include <AzCore/Math/Internal/VectorConversions.inl>
#include <AzCore/Math/Transform.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzToolsFramework
{
    /**
     * For a distance, return the offset to closest edge from center
     * of size. Mod distance with size, if result is > half size, snap
     * forward, otherwise closer to beginning, snap back.
     */
    static float CalculateClosestSnapOffset(float distance, float size)
    {
        // mod distance with size to see how far
        // along size increment distance lies.
        float range;
        if (distance < 0.0f)
        {
            // special handling for negative case
            range = size - fmodf(fabsf(distance), size);
        }
        else
        {
            range = fmodf(distance, size);
        }

        // see if range is closer to beginning or end of step size
        const float center = size * 0.5f;
        if (range < center)
        {
            // distance to snap back
            return -range;
        }

        // distance to snap forward
        return size - range;
    }

    AZ::Vector3 CalculateSnappedOffset(
        const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, float size)
    {
        // calculate total distance along axis
        const float axisDistance = axis.Dot(unsnappedPosition);
        // return offset along axis to snap to step size
        return axis * CalculateClosestSnapOffset(axisDistance, size);
    }

    AZ::Vector3 CalculateSnappedSurfacePosition(
        const AZ::Vector3& worldSurfacePosition, const AZ::Transform& worldFromLocal, int viewportId, float gridSize)
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Vector3 localSurfacePosition = localFromWorld * worldSurfacePosition;

        // snap in xy plane
        AZ::Vector3 localSnappedSurfacePosition = localSurfacePosition +
            CalculateSnappedOffset(localSurfacePosition, AZ::Vector3::CreateAxisX(), gridSize) +
            CalculateSnappedOffset(localSurfacePosition, AZ::Vector3::CreateAxisY(), gridSize);

        // find terrain height at xy snapped location
        float terrainHeight = 0.0f;
        ViewportInteractionRequestBus::EventResult(
            terrainHeight, viewportId,
            &ViewportInteractionRequestBus::Events::TerrainHeight,
            Vector3ToVector2(worldFromLocal * localSnappedSurfacePosition));

        // set snapped z value to terrain height
        AZ::Vector3 localTerrainHeight = localFromWorld * AZ::Vector3(0.0f, 0.0f, terrainHeight);
        localSnappedSurfacePosition.SetZ(localTerrainHeight.GetZ());

        return localSnappedSurfacePosition;
    }

    bool GridSnapping(int viewportId)
    {
        bool snapping;
        ViewportInteractionRequestBus::EventResult(
            snapping, viewportId,
            &ViewportInteractionRequestBus::Events::GridSnappingEnabled);
        return snapping;
    }

    float GridSize(int viewportId)
    {
        float gridSize;
        ViewportInteractionRequestBus::EventResult(
            gridSize, viewportId,
            &ViewportInteractionRequestBus::Events::GridSize);
        return gridSize;
    }
}
