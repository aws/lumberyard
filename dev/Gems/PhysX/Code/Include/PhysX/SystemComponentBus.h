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

#include <PxPhysicsAPI.h>

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/Shape.h>
#include "ConfigurationBus.h"

namespace PhysX
{
    class AzPhysXCpuDispatcher;

    /// Requests for the PhysX system component.
    /// The system component owns fundamental PhysX objects which manage worlds, rigid bodies, shapes, materials,
    /// constraints etc., and perform cooking (processing assets such as meshes and heightfields ready for use in PhysX).
    class SystemRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~SystemRequests() = default;

        /// Creates a new scene (world).
        /// @param sceneDesc Scene descriptor specifying details of scene to be created.
        /// @return Pointer to the created scene.
        virtual physx::PxScene* CreateScene(physx::PxSceneDesc& sceneDesc) = 0;

        /// Creates a new convex mesh.
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexNum Number of vertices in mesh.
        /// @param vertexStride Size of each entry in the vertex data.
        /// @return Pointer to the created mesh.
        virtual physx::PxConvexMesh* CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride) = 0; // should we use AZ::Vector3* or physx::PxVec3 here?

        /// Creates a new convex mesh from pre-cooked convex mesh data.
        /// @param cookedMeshData Pointer to the cooked convex mesh data.
        /// @param bufferSize Size of the cookedMeshData buffer in bytes.
        /// @return Pointer to the created convex mesh.
        virtual physx::PxConvexMesh* CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) = 0;

        /// Creates a new triangle mesh from pre-cooked mesh data.
        /// @param cookedMeshData Pointer to the cooked mesh data.
        /// @param bufferSize Size of the cookedMeshData buffer in bytes.
        /// @return Pointer to the created mesh.
        virtual physx::PxTriangleMesh* CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) = 0;

        /// Creates a new convex mesh.
        /// @param filePath Path to the output file
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in mesh.
        /// @return Result of cooking.
        virtual bool CookConvexMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount) = 0;

        /// Creates a new triangular mesh.
        /// @param filePath Path to the output file
        /// @param vertices Pointer to beginning of vertex data.
        /// @param vertexCount Number of vertices in mesh.
        /// @param indices Pointer to beginning of index data.
        /// @param indexCount Number of indices in mesh.
        /// @return Result of cooking.
        virtual bool CookTriangleMeshToFile(const AZStd::string& filePath, const AZ::Vector3* vertices, AZ::u32 vertexCount,
            const AZ::u32* indices, AZ::u32 indexCount) = 0;

        /// Adds an appropriate collider component to the entity based on the provided shape configuration.
        /// @param entity Entity where the component should be added to.
        /// @param colliderConfiguration Configuration of the collider.
        /// @param shapeConfiguration Configuration of the shape of the collider.
        /// @param addEditorComponents Tells whether to add the Editor version of the collider component or the Game one.
        virtual void AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration, bool addEditorComponents = false) = 0;

        /// Creates PhysX collision filter data from generic collision filtering settings.
        /// @param layer The collision layer the object belongs to.
        /// @param group The set of collision layers the object will interact with.
        virtual physx::PxFilterData CreateFilterData(const Physics::CollisionLayer& layer, const Physics::CollisionGroup& group) = 0;

        /// Creates a generic physics API shape that wraps a native PhysX shape.
        /// Make sure material in nativeShape is created via MaterialManager.
        /// @param nativeShape The native PhysX shape to be wrapped.
        virtual AZStd::shared_ptr<Physics::Shape> CreateWrappedNativeShape(physx::PxShape* nativeShape) = 0;

        /// Connects to the PhysX Visual Debugger.
        virtual bool ConnectToPvd() = 0;

        /// Disconnects from the PhysX Visual Debugger.
        virtual void DisconnectFromPvd() = 0;

        /// Gets the cooking object.
        /// It is possible to update the current cooking params with setParams on PxCooking,
        /// this way the default cooking params can be overridden if required.
        /// References: https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Geometry.html#triangle-meshes,
        /// https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/Startup.html#cooking
        virtual physx::PxCooking* GetCooking() = 0;

        /// Creates a controller manager.
        virtual physx::PxControllerManager* CreateControllerManager(physx::PxScene* scene) = 0;

        /// Releases a controller manager.
        virtual void ReleaseControllerManager(physx::PxControllerManager* controllerManager) = 0;

        /// Configure the visualization of colliders based on proximity from a camera.
        /// @param enabled to activate collider visualization by proximity or not.
        /// @param cameraPosition current position of the camera.
        /// @param radius radius around the camera position to visualize colliders.
        virtual void UpdateColliderProximityVisualization(bool enabled, const AZ::Vector3& cameraPosition, float radius) = 0;
    };

    using SystemRequestsBus = AZ::EBus<SystemRequests>;

    /// Broadcast PhysX system events.
    class SystemEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~SystemEvents() {}
    };

    using SystemEventBus = AZ::EBus<SystemEvents>;
} // namespace PhysX
