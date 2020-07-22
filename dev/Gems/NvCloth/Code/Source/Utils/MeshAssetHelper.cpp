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

#include <NvCloth_precompiled.h>

#include <platform.h> // Needed for MeshAsset.h
#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MeshAsset.h>

#include <Utils/MeshAssetHelper.h>

namespace NvCloth
{
    MeshAssetHelper::MeshAssetHelper(AZ::EntityId entityId)
        : AssetHelper(entityId)
    {
    }

    void MeshAssetHelper::GatherClothMeshNodes(MeshNodeList& meshNodes)
    {
        AZ::Data::Asset<LmbrCentral::MeshAsset> meshAsset;
        LmbrCentral::MeshComponentRequestBus::EventResult(
            meshAsset, m_entityId, &LmbrCentral::MeshComponentRequestBus::Events::GetMeshAsset);
        if (!meshAsset.IsReady())
        {
            return;
        }

        IStatObj* statObj = meshAsset.Get()->m_statObj.get();
        if (!statObj)
        {
            return;
        }

        if (!statObj->GetClothInverseMasses().empty())
        {
            meshNodes.push_back(statObj->GetCGFNodeName().c_str());
        }

        const int subObjectCount = statObj->GetSubObjectCount();
        for (int i = 0; i < subObjectCount; ++i)
        {
            IStatObj::SSubObject* subObject = statObj->GetSubObject(i);
            if (subObject &&
                subObject->pStatObj &&
                !subObject->pStatObj->GetClothInverseMasses().empty())
            {
                meshNodes.push_back(subObject->pStatObj->GetCGFNodeName().c_str());
            }
        }
    }

    bool MeshAssetHelper::ObtainClothMeshNodeInfo(
        const AZStd::string& meshNode, 
        MeshNodeInfo& meshNodeInfo,
        AZStd::vector<SimParticleType>& meshParticles,
        AZStd::vector<SimIndexType>& meshIndices,
        AZStd::vector<SimUVType>& meshUVs)
    {
        AZ::Data::Asset<LmbrCentral::MeshAsset> meshAsset;
        LmbrCentral::MeshComponentRequestBus::EventResult(
            meshAsset, m_entityId, &LmbrCentral::MeshComponentRequestBus::Events::GetMeshAsset);
        if (!meshAsset.IsReady())
        {
            return false;
        }

        IStatObj* statObj = meshAsset.Get()->m_statObj.get();
        if (!statObj)
        {
            return false;
        }

        IStatObj* selectedStatObj = nullptr;
        int primitiveIndex = 0;

        // Find the render data of the mesh node
        if (meshNode == statObj->GetCGFNodeName().c_str())
        {
            selectedStatObj = statObj;
        }
        else
        {
            const int subObjectCount = statObj->GetSubObjectCount();
            for (int i = 0; i < subObjectCount; ++i)
            {
                IStatObj::SSubObject* subObject = statObj->GetSubObject(i);
                if (subObject &&
                    subObject->pStatObj &&
                    subObject->pStatObj->GetRenderMesh() &&
                    meshNode == subObject->pStatObj->GetCGFNodeName().c_str())
                {
                    selectedStatObj = subObject->pStatObj;
                    primitiveIndex = i;
                    break;
                }
            }
        }

        bool infoObtained = false;

        if (selectedStatObj)
        {
            bool dataCopied = CopyDataFromRenderMesh(
                *selectedStatObj->GetRenderMesh(), selectedStatObj->GetClothInverseMasses(),
                meshParticles, meshIndices, meshUVs);

            if (dataCopied)
            {
                // subStatObj contains the buffers for all its submeshes
                // so only 1 submesh is necessary.
                MeshNodeInfo::SubMesh subMesh;
                subMesh.m_primitiveIndex = primitiveIndex;
                subMesh.m_verticesFirstIndex = 0;
                subMesh.m_numVertices = meshParticles.size();
                subMesh.m_indicesFirstIndex = 0;
                subMesh.m_numIndices = meshIndices.size();

                meshNodeInfo.m_lodLevel = 0; // Cloth is alway in LOD 0
                meshNodeInfo.m_subMeshes.push_back(subMesh);

                infoObtained = true;
            }
            else
            {
                AZ_Error("MeshAssetHelper", false, "Failed to extract data from node %s in mesh %s",
                    meshNode.c_str(), statObj->GetFileName().c_str());
            }
        }

        return infoObtained;
    }

    bool MeshAssetHelper::CopyDataFromRenderMesh(
        IRenderMesh& renderMesh,
        const AZStd::vector<float>& clothInverseMasses,
        AZStd::vector<SimParticleType>& meshParticles,
        AZStd::vector<SimIndexType>& meshIndices,
        AZStd::vector<SimUVType>& meshUVs)
    {
        const int numVertices = renderMesh.GetNumVerts();
        const int numIndices = renderMesh.GetNumInds();
        if (numVertices == 0 || numIndices == 0)
        {
            return false;
        }
        else if (numVertices != clothInverseMasses.size())
        {
            AZ_Error("MeshAssetHelper", false,
                "Number of vertices (%d) doesn't match the number of cloth inverse masses (%d)",
                numVertices,
                clothInverseMasses.size());
            return false;
        }

        {
            IRenderMesh::ThreadAccessLock lockRenderMesh(&renderMesh);

            vtx_idx* renderMeshIndices = nullptr;
            strided_pointer<Vec3> renderMeshVertices;
            strided_pointer<Vec2> renderMeshUVs;

            renderMeshIndices = renderMesh.GetIndexPtr(FSL_READ);
            renderMeshVertices.data = reinterpret_cast<Vec3*>(renderMesh.GetPosPtr(renderMeshVertices.iStride, FSL_READ));
            renderMeshUVs.data = reinterpret_cast<Vec2*>(renderMesh.GetUVPtr(renderMeshUVs.iStride, FSL_READ, 0)); // first UV set

            const SimUVType uvZero(0.0f, 0.0f);

            meshParticles.resize(numVertices);
            meshUVs.resize(numVertices);
            for (int index = 0; index < numVertices; ++index)
            {
                meshParticles[index].x = renderMeshVertices[index].x;
                meshParticles[index].y = renderMeshVertices[index].y;
                meshParticles[index].z = renderMeshVertices[index].z;
                meshParticles[index].w = clothInverseMasses[index];

                meshUVs[index] = (renderMeshUVs.data) ? SimUVType(renderMeshUVs[index].x, renderMeshUVs[index].y) : uvZero;
            }

            meshIndices.resize(numIndices);
            // Fast copy when SimIndexType is the same size as the vtx_idx.
            if (sizeof(SimIndexType) == sizeof(vtx_idx))
            {
                memcpy(meshIndices.data(), renderMeshIndices, numIndices * sizeof(SimIndexType));
            }
            else
            {
                for (int index = 0; index < numIndices; ++index)
                {
                    meshIndices[index] = static_cast<SimIndexType>(renderMeshIndices[index]);
                }
            }

            renderMesh.UnlockStream(VSF_GENERAL);
            renderMesh.UnlockIndexStream();
        }

        return true;
    }
} // namespace NvCloth
