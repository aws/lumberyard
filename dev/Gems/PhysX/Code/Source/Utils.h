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

#include <PhysX/ForceRegionComponentBus.h>

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Physics/Material.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

#include <PxPhysicsAPI.h>

namespace Physics
{
    class RigidBodyConfiguration;
    class ColliderConfiguration;
    class ShapeConfiguration;
    class RigidBodyConfiguration;
    class WorldBodyConfiguration;
    class RigidBodyStatic;
    class CollisionGroup;
}

namespace PhysX
{
    class World;


    class Shape;
    class ActorData;
    class Material;
    struct TerrainConfiguration;

    namespace Pipeline
    {
        class MeshAssetCookedData;
    }

    namespace Utils
    {
        bool CreatePxGeometryFromConfig(const Physics::ShapeConfiguration& shapeConfiguration, physx::PxGeometryHolder& pxGeometry, bool warnForNullAsset = true);

        physx::PxShape* CreatePxShapeFromConfig(
            const Physics::ColliderConfiguration& colliderConfiguration, 
            const Physics::ShapeConfiguration& shapeConfiguration, 
            Physics::CollisionGroup& assignedCollisionGroup
        );

        World* GetDefaultWorld();

        AZStd::string ConvexCookingResultToString(physx::PxConvexMeshCookingResult::Enum convexCookingResultCode);
        AZStd::string TriMeshCookingResultToString(physx::PxTriangleMeshCookingResult::Enum triangleCookingResultCode);

        bool WriteCookedMeshToFile(const AZStd::string& filePath, const Pipeline::MeshAssetCookedData& cookedMesh);

        bool MeshDataToPxGeometry(physx::PxBase* meshData, physx::PxGeometryHolder &pxGeometry, const AZ::Vector3& scale);

        /// Terrain Utils
        AZStd::unique_ptr<Physics::RigidBodyStatic> CreateTerrain(
            const PhysX::TerrainConfiguration& terrainConfiguration, const AZ::EntityId& entityId, const AZStd::string_view& name);

        
        void GetMaterialList(
            AZStd::vector<physx::PxMaterial*>& pxMaterials, const AZStd::vector<int>& materialIndexMapping,
            const Physics::TerrainMaterialSurfaceIdMap& terrainMaterialsToSurfaceIds);
        /// Returns all connected busIds of the specified type.
        template<typename BusT>
        AZStd::vector<typename BusT::BusIdType> FindConnectedBusIds()
        {
            AZStd::vector<typename BusT::BusIdType> busIds;
            BusT::EnumerateHandlers([&busIds](typename BusT::Events* /*handler*/)
            {
                busIds.emplace_back(*BusT::GetCurrentBusId());
                return true;
            });
            return busIds;
        }

        /// Logs a warning message using the names of the entities provided.
        void WarnEntityNames(const AZStd::vector<AZ::EntityId>& entityIds, const char* category, const char* message);

        /// Converts collider position and orientation offsets to a transform.
        AZ::Transform GetColliderLocalTransform(const AZ::Vector3& colliderRelativePosition
            , const AZ::Quaternion& colliderRelativeRotation);

        /// Combines collider position and orientation offsets and world transform to a transform.
        AZ::Transform GetColliderWorldTransform(const AZ::Transform& worldTransform
            , const AZ::Vector3& colliderRelativePosition
            , const AZ::Quaternion& colliderRelativeRotation);

        /// Converts points in a collider's local space to world space positions 
        /// accounting for collider position and orientation offsets.
        void ColliderPointsLocalToWorld(AZStd::vector<AZ::Vector3>& pointsInOut
            , const AZ::Transform& worldTransform
            , const AZ::Vector3& colliderRelativePosition
            , const AZ::Quaternion& colliderRelativeRotation);

        /// Returns AABB of collider by constructing PxGeometry from collider and shape configuration,
        /// and invoking physx::PxGeometryQuery::getWorldBounds.
        /// This function is used only by editor components.
        AZ::Aabb GetColliderAabb(const AZ::Transform& worldTransform
            , const ::Physics::ShapeConfiguration& shapeConfiguration
            , const ::Physics::ColliderConfiguration& colliderConfiguration);

        bool TriggerColliderExists(AZ::EntityId entityId);

        /// Logs a warning if there is more than one connected bus of the particular type.
        template<typename BusT>
        void LogWarningIfMultipleComponents(const char* messageCategroy, const char* messageFormat)
        {
            const auto entityIds = FindConnectedBusIds<BusT>();
            if (entityIds.size() > 1)
            {
                WarnEntityNames(entityIds, messageCategroy, messageFormat);
            }
        }
    }

    namespace ReflectionUtils
    {
        /// Forwards invokation of CalculateNetForce in a force region to script canvas.
        class ForceRegionBusBehaviorHandler
            : public ForceRegionNotificationBus::Handler
            , public AZ::BehaviorEBusHandler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER(ForceRegionBusBehaviorHandler, "{EB6C0F7A-0BDA-4052-84C0-33C05E3FF739}", AZ::SystemAllocator
                , OnCalculateNetForce
            );

            static void Reflect(AZ::ReflectContext* context);

            /// Callback invoked when net force exerted on object is computed by a force region.
            void OnCalculateNetForce(AZ::EntityId forceRegionEntityId
                , AZ::EntityId targetEntityId
                , const AZ::Vector3& netForceDirection
                , float netForceMagnitude) override;
        };

        /// Reflect API specific to PhysX physics. Generic physics API should be reflected in Physics::ReflectionUtils::ReflectPhysicsApi.
        void ReflectPhysXOnlyApi(AZ::ReflectContext* context);
    }

    namespace PxActorFactories
    {
        physx::PxRigidDynamic* CreatePxRigidBody(const Physics::RigidBodyConfiguration& configuration);
        physx::PxRigidStatic* CreatePxStaticRigidBody(const Physics::WorldBodyConfiguration& configuration);

        void ReleaseActor(physx::PxActor* actor);
    }
}
