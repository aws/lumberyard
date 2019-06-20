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

#include <AzCore/Math/Vector3.h>

namespace AzToolsFramework
{
    /// Calculate the offset along an axis to adjust a position
    /// to stay snapped to a given grid size.
    AZ::Vector3 CalculateSnappedOffset(
        const AZ::Vector3& unsnappedPosition, const AZ::Vector3& axis, float size);

    /// For a given point on the terrain, calculate the closest xy position snapped to the grid
    /// (z position is aligned to terrain height, not snapped to z grid)
    AZ::Vector3 CalculateSnappedTerrainPosition(
        const AZ::Vector3& worldSurfacePosition, const AZ::Transform& worldFromLocal,
        int viewportId, float gridSize);

    /// Wrapper for grid snapping enabled bus call.
    bool GridSnapping(int viewportId);

    /// Wrapper for grid size bus call.
    float GridSize(int viewportId);

    /// Wrapper for angle snapping enabled bus call.
    bool AngleSnapping(int viewportId);

    /// Wrapper for angle snapping increment bus call.
    /// @return Angle in degrees
    float AngleStep(int viewportId);

    /// Round to x number of significant digits.
    /// @param value Number to round.
    /// @param exponent Precision to use when rounding.
    inline float Round(const float value, const float exponent)
    {
        const float precision = std::powf(10.0f, exponent);
        return roundf(value * precision) / precision;
    }

    /// Round to 3 significant digits (3 digits common usage).
    inline float Round3(const float value)
    {
        return Round(value, 3.0f);
    }

    /// Util to return sign of floating point number.
    /// value > 0 return 1.0
    /// value < 0 return -1.0
    /// value == 0 return 0.0
    inline float Sign(float value)
    {
        return static_cast<float>((0.0f < value) - (value < 0.0f));
    }
} // namespace AzToolsFramework