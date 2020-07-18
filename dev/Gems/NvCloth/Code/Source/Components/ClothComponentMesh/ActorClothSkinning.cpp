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

        bool GetActorAndPose(AZ::EntityId entityId, EMotionFX::Actor*& actor, EMotionFX::Pose*& currentPose)
        {
            EMotionFX::ActorInstance* actorInstance = nullptr;
            EMotionFX::Integration::ActorComponentRequestBus::EventResult(actorInstance, entityId, 
                &EMotionFX::Integration::ActorComponentRequestBus::Events::GetActorInstance);
            if (!actorInstance)
            {
                return false;
            }

            actor = actorInstance->GetActor();
            if (!actor)
            {
                return false;
            }

            EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
            if (!transformData)
            {
                return false;
            }

            currentPose = transformData->GetCurrentPose();
            if (!currentPose)
            {
                return false;
            }

            return true;
        }

        SimParticleType ComputeSkinnedPosition(
            const EMotionFX::Actor& actor,
            const EMotionFX::Pose& pose,
            const SkinningInfo& skinningInfo,
            const SimParticleType& originalPosition)
        {
            EMotionFX::Transform overallSkinningTransform;
            overallSkinningTransform.Zero();
            for (int weightIndex = 0; weightIndex < MaxSkinningBones; ++weightIndex)
            {
                if (skinningInfo.m_jointWeights[weightIndex] == 0)
                {
                    continue;
                }

                const AZ::u32 jointIndex = skinningInfo.m_jointIndices[weightIndex];
                const float jointWeight = skinningInfo.m_jointWeights[weightIndex] / 255.0f;

                EMotionFX::Transform skinningTransform;
                skinningTransform = actor.GetInverseBindPoseTransform(jointIndex);
                skinningTransform.Multiply(pose.GetModelSpaceTransform(jointIndex));
                overallSkinningTransform.Add(skinningTransform, jointWeight);
            }

            const physx::PxMat44 clothSkinningMatrix = PxMathConvert(overallSkinningTransform);

            // Need to have w component set to 1 so that we transform using position too.
            SimParticleType beforeSkinningPosition(originalPosition.getXYZ(), 1.0f);

            return clothSkinningMatrix.transform(beforeSkinningPosition);
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

        // Gather the indices of static / dynamic particles
        for (uint32_t particleIndex = 0; particleIndex < simParticles.size(); ++particleIndex)
        {
            if (simParticles[particleIndex].w == 0.0f)
            {
                actorClothSkinning->m_staticParticleIndices.push_back(particleIndex);
            }
            else
            {
                actorClothSkinning->m_dynamicParticleIndices.push_back(particleIndex);
            }
        }

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
        EMotionFX::Actor* actor = nullptr;
        EMotionFX::Pose* pose = nullptr;
        if (!GetActorAndPose(m_entityId, actor, pose))
        {
            return;
        }

        for (uint32_t index = 0; index < m_staticParticleIndices.size(); ++index)
        {
            const uint32_t currentIndex = m_staticParticleIndices[index];

            const SimParticleType skinnedPosition = ComputeSkinnedPosition(
                *actor,
                *pose,
                m_skinningData[currentIndex],
                originalPositions[currentIndex]);

            // Avoid overwriting the w component that contains the inverse masses
            positions[currentIndex].x = skinnedPosition.x;
            positions[currentIndex].y = skinnedPosition.y;
            positions[currentIndex].z = skinnedPosition.z;
        }
    }

    void ActorClothSkinning::UpdateDynamicParticles(
        float animationBlendFactor, 
        const AZStd::vector<SimParticleType>& originalPositions, 
        AZStd::vector<SimParticleType>& positions)
    {
        EMotionFX::Actor* actor = nullptr;
        EMotionFX::Pose* pose = nullptr;
        if (!GetActorAndPose(m_entityId, actor, pose))
        {
            return;
        }

        animationBlendFactor = AZ::GetClamp(animationBlendFactor, 0.0f, 1.0f);

        for (uint32_t index = 0; index < m_dynamicParticleIndices.size(); ++index)
        {
            const int32_t currentIndex = m_dynamicParticleIndices[index];

            const SimParticleType skinnedPosition = ComputeSkinnedPosition(
                *actor,
                *pose,
                m_skinningData[currentIndex],
                originalPositions[currentIndex]);

            // Avoid overwriting the w component that contains the inverse masses
            positions[currentIndex].x = AZ::Lerp(positions[currentIndex].x, skinnedPosition.x, animationBlendFactor);
            positions[currentIndex].y = AZ::Lerp(positions[currentIndex].y, skinnedPosition.y, animationBlendFactor);
            positions[currentIndex].z = AZ::Lerp(positions[currentIndex].z, skinnedPosition.z, animationBlendFactor);
        }
    }

    void ActorClothSkinning::UpdateParticles(
        const AZStd::vector<SimParticleType>& originalPositions, 
        AZStd::vector<SimParticleType>& positions)
    {
        EMotionFX::Actor* actor = nullptr;
        EMotionFX::Pose* pose = nullptr;
        if (!GetActorAndPose(m_entityId, actor, pose))
        {
            return;
        }

        for (uint32_t currentIndex = 0; currentIndex < m_skinningData.size(); ++currentIndex)
        {
            const SimParticleType skinnedPosition = ComputeSkinnedPosition(
                *actor,
                *pose,
                m_skinningData[currentIndex],
                originalPositions[currentIndex]);

            // Avoid overwriting the w component that contains the inverse masses
            positions[currentIndex].x = skinnedPosition.x;
            positions[currentIndex].y = skinnedPosition.y;
            positions[currentIndex].z = skinnedPosition.z;
        }
    }
} // namespace NvCloth
