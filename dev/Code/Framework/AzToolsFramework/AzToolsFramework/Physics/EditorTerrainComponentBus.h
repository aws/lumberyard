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
#include <AzCore/EBus/EBus.h>

namespace Physics
{
    class MaterialId;
}

namespace PhysX
{
    /// Services provided by the PhysX Mesh Collider Component.
    class EditorTerrainComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual ~EditorTerrainComponentRequests() = default;

        /// Gets the mesh triangles as a list of verts and indices.
        /// @param verts The list of verts in the mesh
        /// @param indices The ordering of the verts into triangles
        virtual void GetTrianglesForHeightField(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices, AZStd::vector<Physics::MaterialId>& materialIds) const = 0;
    };

    /// Bus to service the PhysX Mesh Collider Component event group.
    using EditorTerrainComponentRequestsBus = AZ::EBus<EditorTerrainComponentRequests>;
} // namespace PhysX