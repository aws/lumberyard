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
#include <AzCore/Asset/AssetCommon.h>

#include <System/DataTypes.h>

namespace NvCloth
{
    extern const int InvalidIndex;

    //! Structure holding information about the submeshes of a render mesh node.
    //! While the simulation data is a single buffer for vertices and indices,
    //! this structure knows how to separate them in different submeshes, this will be
    //! be used when the MeshModificationNotificationBus request the modification
    //! of an specific submesh (lodLevel and primitiveIndex).
    struct MeshNodeInfo
    {
        // LOD level of the mesh node inside the asset.
        int m_lodLevel = InvalidIndex;

        struct SubMesh
        {
            // Primitive index inside the asset.
            int m_primitiveIndex = InvalidIndex;

            int m_verticesFirstIndex = InvalidIndex;
            int m_numVertices = 0;

            int m_indicesFirstIndex = InvalidIndex;
            int m_numIndices = 0;
        };

        // List of submeshes.
        AZStd::vector<SubMesh> m_subMeshes;
    };

    //! Interface to obtain cloth information from inside an Asset.
    class AssetHelper
    {
    public:
        AZ_RTTI(AssetHelper, "{8BBDFB6C-4615-4092-B38A-A1FEFEBD1A1F}");

        explicit AssetHelper(AZ::EntityId entityId);
        virtual ~AssetHelper() = default;

        //! Creates the appropriate asset helper depending on the entity's render service.
        static AZStd::unique_ptr<AssetHelper> CreateAssetHelper(AZ::EntityId entityId);

        //! Populates the list of mesh nodes inside the Asset that contains cloth information.
        virtual void GatherClothMeshNodes(MeshNodeList& meshNodes) = 0;

        //! Extracts the cloth mesh information of a node inside the Asset.
        virtual bool ObtainClothMeshNodeInfo(
            const AZStd::string& meshNode,
            MeshNodeInfo& meshNodeInfo,
            AZStd::vector<SimParticleType>& meshParticles,
            AZStd::vector<SimIndexType>& meshIndices,
            AZStd::vector<SimUVType>& meshUVs) = 0;

        //! Returns whether the asset has support for skinned animation or not.
        virtual bool DoesSupportSkinnedAnimation() const = 0;

    protected:
        AZ::EntityId m_entityId;
    };
} // namespace NvCloth
