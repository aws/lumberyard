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

#include <AzCore/Component/Entity.h>

#include <System/DataTypes.h>

namespace NvCloth
{
    static const int MaxSkinningBones = 4;
    struct SkinningInfo
    {
        AZStd::array<AZ::u8, MaxSkinningBones> m_jointWeights;
        AZStd::array<AZ::u16, MaxSkinningBones> m_jointIndices;
    };

    //! Class to retrieve skinning information from an actor on the same entity and use that data to apply skinning to
    //! cloth anchor vertices.
    class ActorClothSkinning
    {
    public:
        AZ_TYPE_INFO(ActorClothSkinning, "{3E7C664D-096B-4126-8553-3241BA965533}");

        static AZStd::unique_ptr<ActorClothSkinning> Create(
            AZ::EntityId entityId, 
            const AZStd::vector<SimParticleType>& simParticles, 
            const AZStd::string& meshNode);

        explicit ActorClothSkinning(AZ::EntityId entityId);

        // Updates the static particles with the current pose of the actor.
        void UpdateStaticParticles(
            const AZStd::vector<SimParticleType>& originalPositions, 
            AZStd::vector<SimParticleType>& positions);

        // Updates the dynamic particles with the current pose of the actor.
        void UpdateDynamicParticles(
            float animationBlendFactor, 
            const AZStd::vector<SimParticleType>& originalPositions, 
            AZStd::vector<SimParticleType>& positions);

        // Updates all the particles with the current pose of the actor.
        void UpdateParticles(
            const AZStd::vector<SimParticleType>& originalPositions, 
            AZStd::vector<SimParticleType>& positions);

    private:
        AZ::EntityId m_entityId;
        AZStd::vector<uint32_t> m_staticParticleIndices;
        AZStd::vector<uint32_t> m_dynamicParticleIndices;
        AZStd::vector<SkinningInfo> m_skinningData;
    };
}// namespace NvCloth
