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

#include <AzFramework/Physics/Material.h>
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
        bool CreatePxGeometryFromConfig(const Physics::ShapeConfiguration& shapeConfiguration, physx::PxGeometryHolder& pxGeometry);

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
    }

    namespace PxActorFactories
    {
        physx::PxRigidDynamic* CreatePxRigidBody(const Physics::RigidBodyConfiguration& configuration);
        physx::PxRigidStatic* CreatePxStaticRigidBody(const Physics::WorldBodyConfiguration& configuration);

        void ReleaseActor(physx::PxActor* actor);
    }
}
