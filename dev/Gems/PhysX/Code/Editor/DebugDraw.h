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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Shape.h>
#include <PhysX/ConfigurationBus.h>
#include <PhysX/MeshAsset.h>

namespace PhysX
{
    namespace DebugDraw
    {
        //! Open the PhysX Settings Window on the Global Settings tab.
        void OpenPhysXSettingsWindow();

        //! Determine if the global debug draw preference is set to the specified state.
        //! @param requiredState The collider debug state to check against the global state.
        //! @return True if global collider debug state matches the input requiredState.
        bool IsGlobalColliderDebugCheck(PhysX::EditorConfiguration::GlobalCollisionDebugState requiredState);

        class DisplayCallback
        {
        public:
            virtual void Display(AzFramework::DebugDisplayRequests& debugDisplayRequests) const = 0;
        protected:
            ~DisplayCallback() = default;
        };

        class Collider
            : protected AzFramework::EntityDebugDisplayEventBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(Collider, AZ::SystemAllocator, 0);
            AZ_RTTI(Collider, "{7DE9CA01-DF1E-4D72-BBF4-76C9136BE6A2}");
            static void Reflect(AZ::ReflectContext* context);

            Collider() = default;

            void Connect(AZ::EntityId entityId);
            void SetDisplayCallback(const DisplayCallback* callback);
            void Disconnect();

            void SetMeshDirty();
            void BuildMeshes(const Physics::ShapeConfiguration& shapeConfig,
                const AZ::Data::Asset<Pipeline::MeshAsset>& meshColliderAsset) const;
            void BuildAABBVerts(const AZ::Vector3& boxMin, const AZ::Vector3& boxMax) const;
            void BuildTriangleMesh(physx::PxBase* meshData) const;
            void BuildConvexMesh(physx::PxBase* meshData) const;

            struct ElementDebugInfo
            {
                // Note: this doesn't use member initializer because of a bug in Mac OS compiler
                ElementDebugInfo()
                    : m_materialSlotIndex(0)
                    , m_numTriangles(0)
                {}

                int m_materialSlotIndex;
                AZ::u32 m_numTriangles;
            };
            AZ::Color CalcDebugColor(const Physics::ColliderConfiguration& colliderConfig
                , const ElementDebugInfo& elementToDebugInfo = ElementDebugInfo()) const;
            AZ::Color CalcDebugColorWarning(const AZ::Color& baseColor, AZ::u32 triangleCount) const;

            void DrawSphere(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::SphereShapeConfiguration& sphereShapeConfig,
                const AZ::Vector3& transformScale) const;
            void DrawBox(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::BoxShapeConfiguration& boxShapeConfig,
                const AZ::Vector3& transformScale) const;
            void DrawCapsule(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::CapsuleShapeConfiguration& capsuleShapeConfig,
                const AZ::Vector3& transformScale) const;
            void DrawMesh(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig,
                const Physics::PhysicsAssetShapeConfiguration& assetConfig,
                const AZ::Data::Asset<Pipeline::MeshAsset>& meshColliderAsset) const;
            void DrawTriangleMesh(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig, physx::PxBase* meshData) const;
            void DrawConvexMesh(AzFramework::DebugDisplayRequests& debugDisplay,
                const Physics::ColliderConfiguration& colliderConfig, physx::PxBase* meshData) const;

            AZ::Transform GetColliderLocalTransform(const Physics::ColliderConfiguration& colliderConfig) const;
            float GetUniformScale() const;
            AZ::Vector3 GetNonUniformScale() const;

            const AZStd::vector<AZ::Vector3>& GetVerts() const;
            const AZStd::vector<AZ::Vector3>& GetPoints() const;
            const AZStd::vector<AZ::u32>& GetIndices() const;
        protected:
            // AzFramework::EntityDebugDisplayEventBus
            void DisplayEntityViewport(
                const AzFramework::ViewportInfo& viewportInfo,
                AzFramework::DebugDisplayRequests& debugDisplay) override;

            bool m_globalButtonState = false; //!< Button linked to the global debug preference.
            bool m_locallyEnabled = true; //!< Local setting to enable displaying the collider in editor view.
            AZ::EntityId m_entityId;
            const DisplayCallback* m_displayCallback = nullptr;

            mutable bool m_meshDirty = true;
            mutable AZStd::unordered_map<int, AZStd::vector<AZ::u32>> m_triangleIndexesByMaterialSlot;
            mutable AZStd::vector<AZ::Vector3> m_verts;
            mutable AZStd::vector<AZ::Vector3> m_points;
            mutable AZStd::vector<AZ::u32> m_indices;
        };
    } // namespace DebugDraw
} // namespace PhysX
