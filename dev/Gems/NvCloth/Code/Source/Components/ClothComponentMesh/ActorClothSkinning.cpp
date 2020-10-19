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
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/ActorInstance.h>

#include <Components/ClothComponentMesh/ActorClothSkinning.h>
#include <Utils/MathConversion.h>

namespace NvCloth
{
    namespace
    {
        bool ObtainSkinningData(
            AZ::EntityId entityId, 
            const AZStd::string& meshNode, 
            AZStd::vector<SkinningInfo>& skinningData)
        {
            EMotionFX::ActorInstance* actorInstance = nullptr;
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(actorInstance, entityId, &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
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

            // Find the render data of the mesh node
            for (uint32 lodLevel = 0; lodLevel < numLODs; ++lodLevel)
            {
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
                        // Skip nodes other than the one we're looking for.
                        continue;
                    }

                    emfxMesh = mesh;
                    break;
                }

                if (emfxMesh)
                {
                    break;
                }
            }

            if (!emfxMesh)
            {
                return false;
            }

            const AZ::u32* sourceOriginalVertex = static_cast<AZ::u32*>(emfxMesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));
            EMotionFX::SkinningInfoVertexAttributeLayer* sourceSkinningInfo = 
                static_cast<EMotionFX::SkinningInfoVertexAttributeLayer*>(
                    emfxMesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID));

            if (!sourceOriginalVertex || !sourceSkinningInfo)
            {
                return false;
            }

            const int numVertices = emfxMesh->GetNumVertices();
            if (numVertices == 0)
            {
                AZ_Error("ActorClothSkinning", false, "Invalid mesh data");
                return false;
            }

            skinningData.resize(numVertices);
            for (int index = 0; index < numVertices; ++index)
            {
                SkinningInfo& skinningInfo = skinningData[index];

                const AZ::u32 originalVertex = sourceOriginalVertex[index];
                const AZ::u32 influenceCount = AZ::GetMin<AZ::u32>(MaxSkinningBones, sourceSkinningInfo->GetNumInfluences(originalVertex));
                AZ::u32 influenceIndex = 0;
                AZ::u8 weightError = 255;

                for (; influenceIndex < influenceCount; ++influenceIndex)
                {
                    EMotionFX::SkinInfluence* influence = sourceSkinningInfo->GetInfluence(originalVertex, influenceIndex);
                    skinningInfo.m_jointIndices[influenceIndex] = influence->GetNodeNr();
                    skinningInfo.m_jointWeights[influenceIndex] = static_cast<AZ::u8>(AZ::GetClamp<float>(influence->GetWeight() * 255.0f, 0.0f, 255.0f));
                    if (skinningInfo.m_jointWeights[influenceIndex] >= weightError)
                    {
                        skinningInfo.m_jointWeights[influenceIndex] = weightError;
                        weightError = 0;
                        influenceIndex++;
                        break;
                    }
                    else
                    {
                        weightError -= skinningInfo.m_jointWeights[influenceIndex];
                    }
                }

                skinningInfo.m_jointWeights[0] += weightError;

                for (; influenceIndex < MaxSkinningBones; ++influenceIndex)
                {
                    skinningInfo.m_jointIndices[influenceIndex] = 0;
                    skinningInfo.m_jointWeights[influenceIndex] = 0;
                }
            }

            return true;
        }

        const AZ::Transform* GetSkinningMatrices(AZ::EntityId entityId)
        {
            EMotionFX::ActorInstance* actorInstance = nullptr;
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(actorInstance, entityId, 
                &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
            if (!actorInstance)
            {
                return nullptr;
            }

            const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
            if (!transformData)
            {
                return nullptr;
            }

            return transformData->GetSkinningMatrices();
        }

        AZ::Vector3 ComputeSkinnedPosition(
            const AZ::Vector3& originalPosition,
            const SkinningInfo& skinningInfo,
            const AZ::Transform* skinningMatrices)
        {
            AZ::Transform clothSkinningMatrix = AZ::Transform::CreateZero();
            for (int weightIndex = 0; weightIndex < MaxSkinningBones; ++weightIndex)
            {
                if (skinningInfo.m_jointWeights[weightIndex] == 0)
                {
                    continue;
                }

                const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
                const float jointWeight = skinningInfo.m_jointWeights[weightIndex] / 255.0f;

                // Blending matrices the same way done in GPU shaders, by adding each weighted matrix element by element.
                // This way the skinning results are much similar to the skinning performed in GPU.
                for (int i = 0; i < 3; ++i)
                {
                    clothSkinningMatrix.SetRow(i, clothSkinningMatrix.GetRow(i) + skinningMatrices[jointIndex].GetRow(i) * jointWeight);
                }
            }

            return clothSkinningMatrix * originalPosition;
        }
    }

    AZStd::unique_ptr<ActorClothSkinning> ActorClothSkinning::Create(
        AZ::EntityId entityId, 
        const AZStd::vector<SimParticleType>& simParticles, 
        const AZStd::string& meshNode)
    {
        AZStd::vector<SkinningInfo> skinningData;
        if (!ObtainSkinningData(entityId, meshNode, skinningData))
        {
            return nullptr;
        }

        if (simParticles.size() != skinningData.size())
        {
            AZ_Error("ActorClothSkinning", false, 
                "Number of simulation particles (%d) doesn't match with skinning data obtained (%d)", 
                simParticles.size(), skinningData.size());
            return nullptr;
        }

        AZStd::unique_ptr<ActorClothSkinning> actorClothSkinning = AZStd::make_unique<ActorClothSkinning>(entityId);

        AZStd::set<AZ::u16> staticParticleJointIndices;
        AZStd::set<AZ::u16> dynamicParticleJointIndices;

        // Gather the indices of static / dynamic particles and joints
        for (uint32_t particleIndex = 0; particleIndex < simParticles.size(); ++particleIndex)
        {
            auto insertInfluentialJoints = [](const SkinningInfo& skinningInfo, AZStd::set<AZ::u16>& particleJointIndices)
            {
                // Insert the indices of the joints that influence the particle (weight is not 0)
                for (int weightIndex = 0; weightIndex < MaxSkinningBones; ++weightIndex)
                {
                    if (skinningInfo.m_jointWeights[weightIndex] == 0)
                    {
                        continue;
                    }

                    const AZ::u16 jointIndex = skinningInfo.m_jointIndices[weightIndex];
                    particleJointIndices.insert(jointIndex);
                }
            };

            // Is an static particle?
            if (simParticles[particleIndex].w == 0.0f)
            {
                actorClothSkinning->m_staticParticleIndices.push_back(particleIndex);
                insertInfluentialJoints(skinningData[particleIndex], staticParticleJointIndices);
            }
            else
            {
                actorClothSkinning->m_dynamicParticleIndices.push_back(particleIndex);
                insertInfluentialJoints(skinningData[particleIndex], dynamicParticleJointIndices);
            }
        }

        AZStd::set<AZ::u16> jointIndices = staticParticleJointIndices;
        jointIndices.insert(dynamicParticleJointIndices.begin(), dynamicParticleJointIndices.end());

        actorClothSkinning->m_staticParticleJointIndices.assign(staticParticleJointIndices.begin(), staticParticleJointIndices.end());
        actorClothSkinning->m_dynamicParticleJointIndices.assign(dynamicParticleJointIndices.begin(), dynamicParticleJointIndices.end());
        actorClothSkinning->m_jointIndices.assign(jointIndices.begin(), jointIndices.end());

        actorClothSkinning->m_skinningData = skinningData;

        return actorClothSkinning;
    }

    ActorClothSkinning::ActorClothSkinning(AZ::EntityId entityId)
        : m_entityId(entityId)
    {
    }

    void ActorClothSkinning::UpdateStaticParticles(
        const AZStd::vector<SimParticleType>& originalPositions, 
        AZStd::vector<SimParticleType>& positions)
    {
        const AZ::Transform* skinningMatrices = GetSkinningMatrices(m_entityId);
        if (!skinningMatrices)
        {
            return;
        }

        for (uint32_t index = 0; index < m_staticParticleIndices.size(); ++index)
        {
            const uint32_t currentIndex = m_staticParticleIndices[index];

            const AZ::Vector3 skinnedPosition = ComputeSkinnedPosition(
                PxMathConvert(originalPositions[currentIndex].getXYZ()),
                m_skinningData[currentIndex],
                skinningMatrices);

            // Avoid overwriting the w component that contains the inverse masses
            positions[currentIndex].x = skinnedPosition.GetX();
            positions[currentIndex].y = skinnedPosition.GetY();
            positions[currentIndex].z = skinnedPosition.GetZ();
        }
    }

    void ActorClothSkinning::UpdateDynamicParticles(
        float animationBlendFactor, 
        const AZStd::vector<SimParticleType>& originalPositions, 
        AZStd::vector<SimParticleType>& positions)
    {
        const AZ::Transform* skinningMatrices = GetSkinningMatrices(m_entityId);
        if (!skinningMatrices)
        {
            return;
        }

        animationBlendFactor = AZ::GetClamp(animationBlendFactor, 0.0f, 1.0f);
        if (animationBlendFactor == 0.0f)
        {
            return;
        }

        for (uint32_t index = 0; index < m_dynamicParticleIndices.size(); ++index)
        {
            const int32_t currentIndex = m_dynamicParticleIndices[index];

            const AZ::Vector3 skinnedPosition = ComputeSkinnedPosition(
                PxMathConvert(originalPositions[currentIndex].getXYZ()),
                m_skinningData[currentIndex],
                skinningMatrices);

            // Avoid overwriting the w component that contains the inverse masses
            positions[currentIndex].x = AZ::Lerp(positions[currentIndex].x, skinnedPosition.GetX(), animationBlendFactor);
            positions[currentIndex].y = AZ::Lerp(positions[currentIndex].y, skinnedPosition.GetY(), animationBlendFactor);
            positions[currentIndex].z = AZ::Lerp(positions[currentIndex].z, skinnedPosition.GetZ(), animationBlendFactor);
        }
    }

    void ActorClothSkinning::UpdateParticles(
        const AZStd::vector<SimParticleType>& originalPositions, 
        AZStd::vector<SimParticleType>& positions)
    {
        const AZ::Transform* skinningMatrices = GetSkinningMatrices(m_entityId);
        if (!skinningMatrices)
        {
            return;
        }

        for (uint32_t currentIndex = 0; currentIndex < m_skinningData.size(); ++currentIndex)
        {
            const AZ::Vector3 skinnedPosition = ComputeSkinnedPosition(
                PxMathConvert(originalPositions[currentIndex].getXYZ()),
                m_skinningData[currentIndex],
                skinningMatrices);

            // Avoid overwriting the w component that contains the inverse masses
            positions[currentIndex].x = skinnedPosition.GetX();
            positions[currentIndex].y = skinnedPosition.GetY();
            positions[currentIndex].z = skinnedPosition.GetZ();
        }
    }
} // namespace NvCloth
