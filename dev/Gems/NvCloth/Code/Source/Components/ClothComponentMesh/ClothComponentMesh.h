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

#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>

#include <LmbrCentral/Rendering/MeshModificationBus.h>

#include <System/ClothConfiguration.h>
#include <NvCloth/SystemBus.h>

#include <Utils/AssetHelper.h>
#include <Utils/TangentSpaceCalculation.h>

namespace NvCloth
{
    class Cloth;
    class ActorClothColliders;
    class ActorClothSkinning;
    class ClothDebugDisplay;

    //! Class that handles cloth simulation and updating the
    //! rendering vertex data on the correct render mesh for an entity.
    class ClothComponentMesh
        : public LmbrCentral::MeshModificationNotificationBus::Handler
        , public SystemNotificationsBus::Handler
        , public AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_RTTI(ClothComponentMesh, "{15A0F10C-6248-4CE4-A6FD-0E2D8AFCFEE8}");

        ClothComponentMesh(AZ::EntityId entityId, const ClothConfiguration& config);
        ~ClothComponentMesh();

        AZ_DISABLE_COPY_MOVE(ClothComponentMesh);

    protected:
        // Functions used to setup and tear down cloth component mesh
        void Setup(AZ::EntityId entityId, const ClothConfiguration& config);
        void TearDown();

        // SystemNotificationsBus::Handler overrides
        void OnPreUpdateClothSimulation(float deltaTime) override;
        void OnPostUpdateClothSimulation(float deltaTime) override;

        // AZ::TransformNotificationBus::Handler overrides
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // LmbrCentral::MeshModificationNotificationBus::Handler overrides
        void ModifyMesh(size_t lodIndex, size_t primitiveIndex, IRenderMesh* renderMesh) override;

    private:
        // Rendering data. Stores the final particles that are going to be sent for rendering.
        // Particles are the result of blending cloth simulation and skinning animation.
        // It also stores the tangent space reference systems of each particle that are calculated every frame.
        struct RenderData
        {
            AZStd::vector<SimParticleType> m_particles;
            TangentSpaceCalculation m_tangentSpaces;
        };

        AZStd::vector<SimParticleType>& GetRenderParticles();
        const AZStd::vector<SimParticleType>& GetRenderParticles() const;
        TangentSpaceCalculation& GetRenderTangentSpaces();
        const TangentSpaceCalculation& GetRenderTangentSpaces() const;

        void ClearData();

        // Update functions called every frame in OnTick
        void UpdateSimulationCollisions();
        void UpdateSimulationStaticParticles();
        void BlendSkinningAnimation();
        void RecalculateTangentSpaces();

        bool IsClothFullySimulated() const;
        bool IsClothFullyAnimated() const;

        // Entity Id of the cloth component
        AZ::EntityId m_entityId;

        // Configuration parameters of cloth simulation
        ClothConfiguration m_config;

        // Instance of cloth simulation
        AZStd::unique_ptr<Cloth> m_clothSimulation;

        // Original UVs of the mesh, they are used to calculate tangent space data.
        AZStd::vector<SimUVType> m_meshInitialUVs;

        // Use a double buffer of render data to always have access to the previous frame's data.
        // The previous frame's data is used to workaround that debug draw is one frame delayed.
        static const AZ::u32 RenderDataBufferSize = 2;
        AZ::u32 m_renderDataBufferIndex = 0;
        AZStd::array<RenderData, RenderDataBufferSize> m_renderDataBuffer;

        // Information to map the simulation particles to render mesh nodes.
        MeshNodeInfo m_meshNodeInfo;

        // Cloth Colliders from the character
        AZStd::unique_ptr<ActorClothColliders> m_actorClothColliders;

        // Cloth Skinning from the character
        AZStd::unique_ptr<ActorClothSkinning> m_actorClothSkinning;

        AZStd::unique_ptr<ClothDebugDisplay> m_clothDebugDisplay;
        friend class ClothDebugDisplay; // Give access to data to draw debug information
    };
} // namespace NvCloth
