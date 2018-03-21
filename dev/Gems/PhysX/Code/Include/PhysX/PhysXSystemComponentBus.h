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

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Physics/RigidBody.h>

namespace PhysX
{
    class AzPhysXCpuDispatcher;

    /**
    * Requests for the PhysX system component.
    * The system component owns fundamental PhysX objects which manage worlds, rigid bodies, shapes, materials,
    * constraints etc., and perform cooking (processing assets such as meshes and heightfields ready for use in PhysX).
    */
    class PhysXSystemRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual ~PhysXSystemRequests() = default;

        /**
        * Adds rigid body to the default scene.
        * @param actor Rigid body to be added.
        */
        virtual void AddActor(physx::PxActor& actor) = 0;
        /**
        * Removes rigid body from the default scene.
        * @param actor Rigid body to be removed.
        */
        virtual void RemoveActor(physx::PxActor& actor) = 0;
        /**
        * Creates a new scene (world).
        * @param sceneDesc Scene descriptor specifying details of scene to be created.
        * @return Pointer to the created scene.
        */
        virtual physx::PxScene* CreateScene( physx::PxSceneDesc& sceneDesc) = 0;
        /**
        * Creates a new convex mesh.
        * @param vertices Pointer to beginning of vertex data.
        * @param vertexNum Number of vertices in mesh.
        * @param vertexStride Size of each entry in the vertex data.
        * @return Pointer to the created mesh.
        */
        virtual physx::PxConvexMesh* CreateConvexMesh(const void* vertices, AZ::u32 vertexNum, AZ::u32 vertexStride) = 0; // should we use AZ::Vector3* or physx::PxVec3 here?
        /**
        * Creates a new convex mesh from pre-cooked convex mesh data.
        * @param cookedMeshData Pointer to the cooked convex mesh data.
        * @param bufferSize Size of the cookedMeshData buffer in bytes.
        * @return Pointer to the created convex mesh.
        */
        virtual physx::PxConvexMesh* CreateConvexMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) = 0;
        /**
        * Creates a new triangle mesh from pre-cooked mesh data.
        * @param cookedMeshData Pointer to the cooked mesh data.
        * @param bufferSize Size of the cookedMeshData buffer in bytes.
        * @return Pointer to the created mesh.
        */
        virtual physx::PxTriangleMesh* CreateTriangleMeshFromCooked(const void* cookedMeshData, AZ::u32 bufferSize) = 0;
    };

    using PhysXSystemRequestBus = AZ::EBus<PhysXSystemRequests>;

    /**
    * Broadcast PhysX system events.
    */
    class PhysXSystemEvents
        : public AZ::EBusTraits
    {
    public:
        virtual ~PhysXSystemEvents() {}
    };

    using PhysXSystemEventBus = AZ::EBus<PhysXSystemEvents>;
} // namespace PhysX
