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

#include <AzCore/EBus/EBus.h>

namespace Physics
{
    /// TerrainRequests serviced by the TerrainComponent.
    class TerrainRequests
        : public AZ::EBusTraits
    {
    public:

        virtual ~TerrainRequests() {}

        /// Gets the height of the terrain at position x, y.
        /// @return the height of the terrain at the given location.
        virtual float GetHeight(float x, float y) = 0;

        /// Gets the terrain tile rigid body at the specified index.
        /// @param x position in the x direction.
        /// @param y position in the y direction.
        /// return The rigid body representing the tile at the specified index. Returns nullptr if the terrain is not constructed.
        virtual AZStd::shared_ptr<Physics::RigidBodyStatic> GetTerrainTile(float x, float y) = 0;
    };

    using TerrainRequestBus = AZ::EBus<TerrainRequests>;

} // namespace Physics
