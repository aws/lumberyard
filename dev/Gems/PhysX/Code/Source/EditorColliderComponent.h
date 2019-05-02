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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Manipulators/BoxManipulators.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Shape.h>
#include <PhysX/MeshColliderComponentBus.h>
#include <PhysX/MeshAsset.h>
#include <PhysX/ConfigurationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Quaternion.h>

namespace Physics
{
    class RigidBodyStatic;
}

namespace PhysX
{
    /// Proxy container for only displaying a specific shape configuration depending on the shapeType selected.
    ///
    struct EditorProxyShapeConfig
    {
        AZ_CLASS_ALLOCATOR(EditorProxyShapeConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorProxyShapeConfig, "{531FB42A-42A9-4234-89BA-FD349EF83D0C}");
        static void Reflect(AZ::ReflectContext* context);

        EditorProxyShapeConfig() = default;
        EditorProxyShapeConfig(const Physics::ShapeConfiguration& shapeConfiguration);
        virtual ~EditorProxyShapeConfig() = default;

        Physics::ShapeType m_shapeType = Physics::ShapeType::PhysicsAsset;
        Physics::SphereShapeConfiguration m_sphere;
        Physics::BoxShapeConfiguration m_box;
        Physics::CapsuleShapeConfiguration m_capsule;
        Physics::PhysicsAssetShapeConfiguration m_physicsAsset;

        bool IsSphereConfig() const;
        bool IsBoxConfig() const;
        bool IsCapsuleConfig() const;
        bool IsAssetConfig() const;
        Physics::ShapeConfiguration& GetCurrent();

        AZ::u32 OnConfigurationChanged();
    };

    /// Editor PhysX Collider Component.
    ///
    class EditorColliderComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , protected AzFramework::EntityDebugDisplayEventBus::Handler
        , protected AzToolsFramework::EntitySelectionEvents::Bus::Handler
        , private AzToolsFramework::BoxManipulatorHandler
        , private AZ::Data::AssetBus::MultiHandler
        , private PhysX::MeshColliderComponentRequestsBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , public AZ::TickBus::Handler
        , private PhysX::ConfigurationNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorColliderComponent, "{FD429282-A075-4966-857F-D0BBF186CFE6}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
            provided.push_back(AZ_CRC("ProximityTriggerService", 0x561f262c));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            // Not compatible with cry engine colliders
            required.push_back(AZ_CRC("ColliderService", 0x902d4e93));
        }

        EditorColliderComponent() = default;
        EditorColliderComponent(const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration);

        // these functions are made virtual because we call them from other modules
        virtual const EditorProxyShapeConfig& GetShapeConfiguration() const;
        virtual const Physics::ColliderConfiguration& GetColliderConfiguration() const;

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    private:
        AZ_DISABLE_COPY_MOVE(EditorColliderComponent)

        // AZ::Component
        void Activate() override;

        void Deactivate() override;

        /// AzToolsFramework::EntitySelectionEvents
        void OnSelected() override;
        void OnDeselected() override;

        // AzFramework::EntityDebugDisplayEventBus
        void DisplayEntity(bool& handled) override;
        void Display(AzFramework::EntityDebugDisplayRequests& displayContext);

        // AZ::Data::AssetBus::Handler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        // PhysXMeshColliderComponentRequestBus
        AZ::Data::Asset<Pipeline::MeshAsset> GetMeshAsset() const override;
        void GetStaticWorldSpaceMeshTriangles(AZStd::vector<AZ::Vector3>& verts, AZStd::vector<AZ::u32>& indices) const override;
        Physics::MaterialId GetMaterialId() const override;
        void SetMeshAsset(const AZ::Data::AssetId& id) override;
        void SetMaterialAsset(const AZ::Data::AssetId& id) override;
        void SetMaterialId(const Physics::MaterialId& id) override;

        void UpdateMaterialSlotsFromMeshAsset();

        // TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // TransformBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        // PhysX::ConfigurationNotificationBus
        virtual void OnConfigurationRefreshed(const Configuration& configuration) override;

        AZ::Transform GetColliderTransform() const;
        float GetUniformScale() const;
        AZ::Vector3 GetNonUniformScale() const;

        EditorProxyShapeConfig m_shapeConfiguration;
        Physics::ColliderConfiguration m_configuration;

        AZ::u32 OnConfigurationChanged();
        void UpdateShapeConfigurationScale();

        void DrawSphere(AzFramework::EntityDebugDisplayRequests& displayContext, const Physics::SphereShapeConfiguration& config);
        void DrawBox(AzFramework::EntityDebugDisplayRequests& displayContext, const Physics::BoxShapeConfiguration& config);
        void DrawCapsule(AzFramework::EntityDebugDisplayRequests& displayContext, const Physics::CapsuleShapeConfiguration& config);
        void DrawMesh(AzFramework::EntityDebugDisplayRequests& displayContext);

        // Mesh collider
        void DrawTriangleMesh(AzFramework::EntityDebugDisplayRequests& displayContext, physx::PxBase* meshData) const;
        void DrawConvexMesh(AzFramework::EntityDebugDisplayRequests& displayContext, physx::PxBase* meshData) const;
        void UpdateColliderMeshColor(AZ::Color& baseColor, AZ::u32 triangleCount) const;
        bool IsAssetConfig() const;
        void UpdateMeshAsset();
        void CreateStaticEditorCollider();

        AZ::Data::Asset<Pipeline::MeshAsset> m_meshColliderAsset;
        mutable AZStd::vector<AZ::Vector3> m_verts;
        mutable AZStd::vector<AZ::Vector3> m_points;
        mutable AZStd::vector<AZ::u32> m_indices;

        mutable AZ::Color m_triangleCollisionMeshColor;
        mutable AZ::Color m_convexCollisionMeshColor;
        mutable bool m_meshDirty = true;

        void BuildMeshes() const;
        void BuildAABBVerts(const AZ::Vector3& boxMin, const AZ::Vector3& boxMax) const;
        void BuildTriangleMesh(physx::PxBase* meshData) const;
        void BuildConvexMesh(physx::PxBase* meshData) const;
        AZ::Color m_wireFrameColor;
        AZ::Color m_warningColor;

        float m_colliderVisualizationLineWidth = 2.0f;

        double m_time = 0.0f;

        // Box collider
        AZ::Vector3 GetDimensions() override;
        void SetDimensions(const AZ::Vector3& dimensions) override;
        AZ::Transform GetCurrentTransform() override;
        AzToolsFramework::BoxManipulator m_boxManipulator; ///< Manipulator for editing box dimensions.

        // Capsule collider
        AZ::Vector3 GetCapsuleScale();

        AZStd::shared_ptr<Physics::RigidBodyStatic> m_editorBody;
    };
}
