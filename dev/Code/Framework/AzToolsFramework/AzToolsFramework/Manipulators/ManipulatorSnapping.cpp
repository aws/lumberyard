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
    AZ::Vector3 CalculateSnappedOffset(
        const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, const float size)
    {
        // calculate total distance along axis
        const float axisDistance = axis.Dot(unsnappedPosition);
        // round to nearest step size
        const float snappedAxisDistance = floorf((axisDistance / size) + 0.5f) * size;
        // return offset along axis to snap to step size
        return axis * (snappedAxisDistance - axisDistance);
    }

    AZ::Vector3 CalculateSnappedTerrainPosition(
        const AZ::Vector3& worldSurfacePosition, const AZ::Transform& worldFromLocal,
        const int viewportId, const float gridSize)
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverseFast();
        const AZ::Vector3 localSurfacePosition = localFromWorld * worldSurfacePosition;

        // snap in xy plane
        AZ::Vector3 localSnappedSurfacePosition = localSurfacePosition +
            CalculateSnappedOffset(localSurfacePosition, AZ::Vector3::CreateAxisX(), gridSize) +
            CalculateSnappedOffset(localSurfacePosition, AZ::Vector3::CreateAxisY(), gridSize);

        // find terrain height at xy snapped location
        float terrainHeight = 0.0f;
        ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
            terrainHeight, viewportId,
            &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::TerrainHeight,
            Vector3ToVector2(worldFromLocal * localSnappedSurfacePosition));

        // set snapped z value to terrain height
        AZ::Vector3 localTerrainHeight = localFromWorld * AZ::Vector3(0.0f, 0.0f, terrainHeight);
        localSnappedSurfacePosition.SetZ(localTerrainHeight.GetZ());

        return localSnappedSurfacePosition;
    }

    bool GridSnapping(const int viewportId)
    {
        bool snapping = false;
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            snapping, viewportId,
            &ViewportInteraction::ViewportInteractionRequestBus::Events::GridSnappingEnabled);
        return snapping;
    }

    float GridSize(const int viewportId)
    {
        float gridSize = 0.0f;
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            gridSize, viewportId,
            &ViewportInteraction::ViewportInteractionRequestBus::Events::GridSize);
        return gridSize;
    }

    bool AngleSnapping(const int viewportId)
    {
        bool snapping = false;
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            snapping, viewportId,
            &ViewportInteraction::ViewportInteractionRequestBus::Events::AngleSnappingEnabled);
        return snapping;
    }

    float AngleStep(const int viewportId)
    {
        float angle = 0.0f;
        ViewportInteraction::ViewportInteractionRequestBus::EventResult(
            angle, viewportId,
            &ViewportInteraction::ViewportInteractionRequestBus::Events::AngleStep);
        return angle;
    }
}