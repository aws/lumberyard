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

#include <Integration/ActorComponentBus.h>

// Needed to access the Mesh information inside Actor.
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/ActorInstance.h>

#include <Utils/ActorAssetHelper.h>

namespace NvCloth
{
    ActorAssetHelper::ActorAssetHelper(AZ::EntityId entityId)
        : AssetHelper(entityId)
    {
    }

    void ActorAssetHelper::GatherClothMeshNodes(MeshNodeList& meshNodes)
    {
        EMotionFX::ActorInstance* actorInstance = nullptr;
        EMotionFX::Integration::ActorComponentRequestBus::EventResult(
            actorInstance, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
        if (!actorInstance)
        {
            return;
        }

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        if (!actor)
        {
            return;
        }

        const uint32 numNodes = actor->GetNumNodes();
        const uint32 numLODs = actor->GetNumLODLevels();

        for (uint32 lodLevel = 0; lodLevel < numLODs; ++lodLevel)
        {
            for (uint32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
            {
                const EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                if (!mesh)
                {
                    continue;
                }

                const bool hasClothInverseMasses = (mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_CLOTH_INVMASSES) != nullptr);
                if (hasClothInverseMasses)
                {
                    const EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
                    AZ_Assert(node, "Invalid node %d in actor '%s'", nodeIndex, actor->GetFileNameString().c_str());
                    meshNodes.push_back(node->GetNameString());
                }
            }
        }
    }

    bool ActorAssetHelper::ObtainClothMeshNodeInfo(
        const AZStd::string& meshNode,
        MeshNodeInfo& meshNodeInfo,
        AZStd::vector<SimParticleType>& meshParticles,
        AZStd::vector<SimIndexType>& meshIndices,
        AZStd::vector<SimUVType>& meshUVs)
    {
        EMotionFX::ActorInstance* actorInstance = nullptr;
        EMotionFX::Integration::ActorComponentRequestBus::EventResult(
            actorInstance, m_entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
        if (!actorInstance)
        {
            return false;
        }

        const EMotionFX::Actor* actor = actorInstance->GetActor();
        if (!actor)
        {
            return false;
        }

        const uint32 numNodes = actor->GetNumNodes();
        const uint32 numLODs = actor->GetNumLODLevels();

        const EMotionFX::Mesh* emfxMesh = nullptr;
        uint32 meshFirstPrimitiveIndex = 0;

        // Find the render data of the mesh node
        for (uint32 lodLevel = 0; lodLevel < numLODs; ++lodLevel)
        {
            meshFirstPrimitiveIndex = 0;

            for (uint32 nodeIndex = 0; nodeIndex < numNodes; ++nodeIndex)
            {
                const EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, nodeIndex);
                if (!mesh || mesh->GetIsCollisionMesh())
                {
                    // Skip invalid and collision meshes.
                    continue;
                }

                const EMotionFX::Node* node = actor->GetSkeleton()->GetNode(nodeIndex);
                if (meshNode != node->GetNameString())
                {
                    // Skip. Increase the index of all primitives of the mesh we're skipping.
                    meshFirstPrimitiveIndex += mesh->GetNumSubMeshes();
                    continue;
                }

                // Mesh found, save the lod in mesh info
                meshNodeInfo.m_lodLevel = lodLevel;
                emfxMesh = mesh;
                break;
            }

            if (emfxMesh)
            {
                break;
            }
        }

        bool infoObtained = false;

        if (emfxMesh)
        {
            bool dataCopied = CopyDataFromEMotionFXMesh(*emfxMesh, meshParticles, meshIndices, meshUVs);

            if (dataCopied)
            {
                const uint32 numSubMeshes = emfxMesh->GetNumSubMeshes();
                for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                {
                    const EMotionFX::SubMesh* emfxSubMesh = emfxMesh->GetSubMesh(subMeshIndex);

                    MeshNodeInfo::SubMesh subMesh;
                    subMesh.m_primitiveIndex = static_cast<int>(meshFirstPrimitiveIndex + subMeshIndex);
                    subMesh.m_verticesFirstIndex = emfxSubMesh->GetStartVertex();
                    subMesh.m_numVertices = emfxSubMesh->GetNumVertices();
                    subMesh.m_indicesFirstIndex = emfxSubMesh->GetStartIndex();
                    subMesh.m_numIndices = emfxSubMesh->GetNumIndices();

                    meshNodeInfo.m_subMeshes.push_back(subMesh);
                }

                infoObtained = true;
            }
            else
            {
                AZ_Error("ActorAssetHelper", false, "Failed to extract data from node %s in actor %s",
                    meshNode.c_str(), actor->GetFileNameString().c_str());
            }
        }

        return infoObtained;
    }

    bool ActorAssetHelper::CopyDataFromEMotionFXMesh(
        const EMotionFX::Mesh& emfxMesh,
        AZStd::vector<SimParticleType>& meshParticles,
        AZStd::vector<SimIndexType>& meshIndices,
        AZStd::vector<SimUVType>& meshUVs)
    {
        const int numVertices = emfxMesh.GetNumVertices();
        const int numIndices = emfxMesh.GetNumIndices();
        if (numVertices == 0 || numIndices == 0)
        {
            return false;
        }

        const uint32* sourceIndices = emfxMesh.GetIndices();
        const AZ::Vector3* sourcePositions = static_cast<AZ::Vector3*>(emfxMesh.FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
        const AZ::u32* sourceClothInverseMasses = static_cast<AZ::u32*>(emfxMesh.FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_CLOTH_INVMASSES));
        const AZ::Vector2* sourceUVs = static_cast<AZ::Vector2*>(emfxMesh.FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0)); // first UV set

        if (!sourceIndices || !sourcePositions || !sourceClothInverseMasses)
        {
            return false;
        }

        const SimUVType uvZero(0.0f, 0.0f);

        meshParticles.resize(numVertices);
        meshUVs.resize(numVertices);
        for (int index = 0; index < numVertices; ++index)
        {
            meshParticles[index].x = sourcePositions[index].GetX();
            meshParticles[index].y = sourcePositions[index].GetY();
            meshParticles[index].z = sourcePositions[index].GetZ();

            AZ::Color inverseMassColor;
            inverseMassColor.FromU32(sourceClothInverseMasses[index]);
            meshParticles[index].w = inverseMassColor.GetR(); // Cloth inverse masses is in the red channel

            meshUVs[index] = (sourceUVs) ? sourceUVs[index] : uvZero;
        }

        meshIndices.resize(numIndices);
        // Fast copy when SimIndexType is the same size as the EMFX indices type.
        if (sizeof(SimIndexType) == sizeof(uint32))
        {
            memcpy(meshIndices.data(), sourceIndices, numIndices * sizeof(SimIndexType));
        }
        else
        {
            for (int index = 0; index < numIndices; ++index)
            {
                meshIndices[index] = static_cast<SimIndexType>(sourceIndices[index]);
            }
        }

        return true;
    }
} // namespace NvCloth
