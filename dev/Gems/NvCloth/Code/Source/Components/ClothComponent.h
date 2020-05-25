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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>

#include <LmbrCentral/Rendering/MeshComponentBus.h>
#include <LmbrCentral/Rendering/MeshModificationBus.h>

#include <NvCloth/Allocator.h> // Needed for nv::cloth::Vector

#include <System/ActorClothColliders.h>
#include <System/ActorClothSkinning.h>
#include <System/ClothDebugDisplay.h>
#include <System/SystemComponent.h> // Needed for NvCloth elements

#include <Utils/AssetHelper.h>
#include <Utils/TangentSpaceCalculation.h>

namespace NvCloth
{
    //! Configuration data for ClothComponent.
    struct ClothConfiguration
    {
        AZ_CLASS_ALLOCATOR(ClothConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(ClothConfiguration, "{96E2AF5E-3C98-4872-8F90-F56302A44F2A}");

        static void Reflect(AZ::ReflectContext* context);

        virtual ~ClothConfiguration() = default;

        AZStd::string m_meshNode;

        // Mass and Gravity parameters
        float m_mass = 1.0f;
        bool m_useCustomGravity = false;
        AZ::Vector3 m_customGravity = AZ::Vector3(0.0f, 0.0f, -9.81f);
        float m_gravityScale = 1.0f;

        // Blend factor of cloth simulation with authored skinning animation
        // 0 - Cloth mesh fully simulated
        // 1 - Cloth mesh fully animated
        float m_animationBlendFactor = 0.0f;

        // Global stiffness frequency
        float m_stiffnessFrequency = 10.0f;

        // Damping parameters
        AZ::Vector3 m_damping = AZ::Vector3::CreateZero();
        AZ::Vector3 m_linearDrag = AZ::Vector3(0.2f, 0.2f, 0.2f);
        AZ::Vector3 m_angularDrag = AZ::Vector3(0.2f, 0.2f, 0.2f);

        // Inertia parameters
        AZ::Vector3 m_linearInteria = AZ::Vector3::CreateOne();
        AZ::Vector3 m_angularInteria = AZ::Vector3::CreateOne();
        AZ::Vector3 m_centrifugalInertia = AZ::Vector3::CreateOne();

        // Wind parameters
        AZ::Vector3 m_windVelocity = AZ::Vector3(0.0f, 50.0f, 0.0f);
        float m_airDragCoefficient = 0.0f;
        float m_airLiftCoefficient = 0.0f;
        float m_fluidDensity = 1.0f;

        // Collision parameters
        float m_collisionFriction = 0.0f;
        float m_collisionMassScale = 0.0f;
        bool m_continuousCollisionDetection = false;

        // Self Collision parameters
        float m_selfCollisionDistance = 0.0f;
        float m_selfCollisionStiffness = 0.2f;

        // Tether Constraints parameters
        float m_tetherConstraintStiffness = 1.0f;
        float m_tetherConstraintScale = 1.0f;

        // Quality parameters
        float m_solverFrequency = 300.0f;
        uint32_t m_accelerationFilterIterations = 30;

        // Fabric phases parameters
        float m_horizontalStiffness = 1.0f;
        float m_horizontalStiffnessMultiplier = 0.0f;
        float m_horizontalCompressionLimit = 0.0f;
        float m_horizontalStretchLimit = 0.0f;
        float m_verticalStiffness = 1.0f;
        float m_verticalStiffnessMultiplier = 0.0f;
        float m_verticalCompressionLimit = 0.0f;
        float m_verticalStretchLimit = 0.0f;
        float m_bendingStiffness = 1.0f;
        float m_bendingStiffnessMultiplier = 0.0f;
        float m_bendingCompressionLimit = 0.0f;
        float m_bendingStretchLimit = 0.0f;
        float m_shearingStiffness = 1.0f;
        float m_shearingStiffnessMultiplier = 0.0f;
        float m_shearingCompressionLimit = 0.0f;
        float m_shearingStretchLimit = 0.0f;

    private:
        // Making private functionality related with the Editor Context reflection,
        // it's unnecessary for the clients using ClothConfiguration.
        friend class EditorClothComponent;

        // Callback function set by the EditorClothComponent to gather the list of mesh nodes,
        // showed to the user in a combo box for the m_meshNode member.
        AZStd::function<MeshNodeList()> m_populateMeshNodeListCallback;

        // Used in the StringList attribute of the combobox for m_meshNode member.
        // The StringList attribute doesn't work with the AZStd::function directly, so
        // this function will just call m_populateMeshNodeListCallback if it has been set.
        MeshNodeList PopulateMeshNodeList();

        // Callback function set by the EditorClothComponent to show animation blend parameter.
        AZStd::function<bool()> m_showAnimationBlendFactorCallback;

        // Used in the Visible attribute of m_animationBlendFactor member.
        // The Visible attribute doesn't work with the AZStd::function directly, so
        // this function will just call m_showAnimationBlendFactorCallback if it has been set.
        bool ShowAnimationBlendFactor();

        // Callback function set by the EditorClothComponent to provide the entity id to the combobox widget.
        AZStd::function<AZ::EntityId()> m_getEntityIdCallback;

        // Used in the EntityId attribute of the combobox for m_meshNode member.
        // The EntityId attribute doesn't work with the AZStd::function directly, so
        // this function will just call m_getEntityIdCallback if it has been set.
        AZ::EntityId GetEntityId();
    };

    //! Runtime component that handles cloth simulation and updating the
    //! rendering vertex data on the correct render mesh for this entity.
    class ClothComponent
        : public AZ::Component
        , public LmbrCentral::MeshComponentNotificationBus::Handler
        , public LmbrCentral::MeshModificationNotificationBus::Handler
        , public AZ::TickBus::Handler
        , public AZ::TransformNotificationBus::Handler
        , private TriangleInputProxy
    {
    public:
        AZ_COMPONENT(ClothComponent, "{AC9B8FA0-A6DA-4377-8219-25BA7E4A22E9}");

        static void Reflect(AZ::ReflectContext* context);

        ClothComponent() = default;
        explicit ClothComponent(const ClothConfiguration& config);

        AZ_DISABLE_COPY_MOVE(ClothComponent);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;

        // LmbrCentral::MeshComponentNotificationBus::Handler overrides
        void OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void OnMeshDestroyed() override;

        // AZ::TickBus::Handler overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

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

        // Functions used to setup and tear down the component, which is bound to
        // MeshComponentNotificationBus, when the entity's mesh has been created/destroyed.
        void SetupComponent();
        void TearDownComponent();

        void ClearData();

        // Functions to setup the NvCloth elements
        void CreateCloth();
        void DestroyCloth();
        void InitializeCloth();
        void SetupFabricPhases();

        // Update functions called every frame in OnTick
        void UpdateSimulationCollisions();
        void UpdateSimulationStaticParticles();
        void UpdateSimulation(float deltaTime);
        bool RetrieveSimulationResults();
        void RestoreSimulation();
        void BlendSkinningAnimation();
        void RecalculateTangentSpaces();

        // Update functions called from notification events
        void UpdateSimulationTransform(const AZ::Transform& transformWorld);
        void UpdateSimulationGravity();

        bool IsClothFullySimulated() const;
        bool IsClothFullyAnimated() const;

        // TriangleInputProxy overrides
        // These function are used by CTangentSpaceCalculation when recalculating the tangent spaces.
        AZ::u32 GetTriangleCount() const override;
        AZ::u32 GetVertexCount() const override;
        TriangleIndices GetTriangleIndices(AZ::u32 index) const override;
        Vec3 GetPosition(AZ::u32 index) const override;
        Vec2 GetUV(AZ::u32 index) const override;

        ClothConfiguration m_config;

        // Original cloth data when cloth was created.
        // Initial cloth particles are used to apply skinning animation.
        AZStd::vector<SimParticleType> m_clothInitialParticles;
        AZStd::vector<SimIndexType> m_clothInitialIndices;
        AZStd::vector<SimUVType> m_clothInitialUVs;

        // Simulation particles. Stores the results of the cloth simulation.
        AZStd::vector<SimParticleType> m_simParticles;

        // Use a double buffer of render data to always have access to the previous frame's data.
        // The previous frame's data is used to workaround that debug draw is one frame delayed.
        static const AZ::u32 RenderDataBufferSize = 2;
        AZ::u32 m_renderDataBufferIndex = 0;
        AZStd::array<RenderData, RenderDataBufferSize> m_renderDataBuffer;

        // Information to map the simulation particles to render mesh nodes.
        MeshNodeInfo m_meshNodeInfo;

        // True when all the particles of the fabric are static (inverse masses are all 0)
        bool m_fullyStaticFabric = false;

        // Cloth Colliders from the character
        AZStd::unique_ptr<ActorClothColliders> m_actorClothColliders;

        // Cloth Skinning from the character
        AZStd::unique_ptr<ActorClothSkinning> m_actorClothSkinning;

        AZStd::unique_ptr<ClothDebugDisplay> m_clothDebugDisplay;
        friend class ClothDebugDisplay; // Give access to data to draw debug information

        AZ::u32 m_numInvalidSimulations = 0;

        // NvCloth elements
        SolverUniquePtr m_solver;
        ClothUniquePtr m_cloth;
        FabricUniquePtr m_fabric;
        nv::cloth::Vector<int32_t>::Type m_fabricPhaseTypes;
    };
} // namespace NvCloth
