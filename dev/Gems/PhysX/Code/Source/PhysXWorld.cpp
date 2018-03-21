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

#include <PhysX_precompiled.h>
#include <PhysXWorld.h>
#include <PhysXMathConversion.h>
#include <PhysXSystemComponent.h>
#include <Include/PhysX/PhysXSystemComponentBus.h>
#include <Include/PhysX/PhysXNativeTypeIdentifiers.h>
#include <AzPhysXCpuDispatcher.h>

namespace PhysX
{
    PhysXWorld::PhysXWorld(AZ::Crc32 id, const Physics::Ptr<Physics::WorldSettings>& settings)
        : Physics::World(settings)
        , m_worldId(id)
        , m_maxDeltaTime(settings->m_maxTimeStep)
        , m_fixedDeltaTime(settings->m_fixedTimeStep)
    {
        physx::PxTolerancesScale tolerancesScale = physx::PxTolerancesScale();
        physx::PxSceneDesc sceneDesc(tolerancesScale);
        sceneDesc.gravity = PxVec3FromLYVec3(settings->m_gravity);
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;

        PhysXSystemRequestBus::BroadcastResult(m_world, &PhysXSystemRequests::CreateScene, sceneDesc);

        m_worldCollisionFilter = nullptr;
    }

    PhysXWorld::~PhysXWorld()
    {
        if (m_world)
        {
            m_world->release();
            m_world = nullptr;
        }
    }

    // Physics::World
    void PhysXWorld::RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result)
    {
        auto orig = PxVec3FromLYVec3(request.m_start);
        auto dir = PxVec3FromLYVec3(request.m_dir);
        auto maxDist = request.m_time;
        physx::PxRaycastBuffer hit;

        bool status = m_world->raycast(orig, dir, maxDist, hit);
        if (status && hit.hasBlock)
        {
            const physx::PxVec3& hitPosition = hit.block.position;
            const physx::PxVec3& hitNormal = hit.block.normal;
            float hitDistance = hit.block.distance;
            physx::PxRigidActor* actor = hit.block.actor;
            void* userData = actor->userData;

            Physics::RayCastHit hitResult;
            hitResult.m_hitTime = hitDistance / maxDist; // Is this in [0;1] range?
            hitResult.m_position = LYVec3FromPxVec3(hitPosition);
            hitResult.m_normal = LYVec3FromPxVec3(hitNormal);

            if (userData)
            {
                hitResult.m_entityId = AZ::EntityId(reinterpret_cast<AZ::u64>(userData));
            }

            hitResult.m_hitBody = nullptr; // Get this from the entity
            result.m_hits.push_back(hitResult);
        }
    }

    void PhysXWorld::ShapeCast(const Physics::ShapeCastRequest& request, Physics::ShapeCastResult& result)
    {
        if (request.m_nonLinear)
        {
            AZ_Error("PhysX World", false, "PhysX does not support non-linear shape casts.");
            return;
        }

        physx::PxTransform pose(physx::PxIdentity);
        pose.p = PxVec3FromLYVec3(request.m_start.GetPosition());

        auto azDir = (request.m_end.GetPosition() - request.m_start.GetPosition());
        physx::PxVec3 dir = PxVec3FromLYVec3(azDir.GetNormalized());
        float distance = azDir.GetLength();
        physx::PxSweepBuffer pxResult;

        // PhysX only supports: box, sphere, capsule, convex
        Physics::ShapeType shapeType = request.m_shapeConfiguration->GetShapeType();
        switch (shapeType)
        {
        case Physics::ShapeType::Sphere:
        {
            Physics::SphereShapeConfiguration* sphereConfiguration = static_cast<Physics::SphereShapeConfiguration*>(request.m_shapeConfiguration.get());

            physx::PxSphereGeometry sphere(sphereConfiguration->m_radius);

            m_world->sweep(sphere, pose, dir, distance, pxResult);

            break;
        }
        case Physics::ShapeType::Box:
        {
            Physics::BoxShapeConfiguration* boxConfiguration = static_cast<Physics::BoxShapeConfiguration*>(request.m_shapeConfiguration.get());

            physx::PxBoxGeometry box(PxVec3FromLYVec3(boxConfiguration->m_halfExtents));

            m_world->sweep(box, pose, dir, distance, pxResult);

            break;
        }
        case Physics::ShapeType::Capsule:
        {
            Physics::CapsuleShapeConfiguration* capsuleConfiguration = static_cast<Physics::CapsuleShapeConfiguration*>(request.m_shapeConfiguration.get());

            float halfHeight = (capsuleConfiguration->m_pointA - capsuleConfiguration->m_pointB).GetLength() / 2.0f;

            physx::PxCapsuleGeometry capsule(capsuleConfiguration->m_radius, halfHeight);

            m_world->sweep(capsule, pose, dir, distance, pxResult);

            break;
        }
        case Physics::ShapeType::ConvexHull:
        {
            Physics::ConvexHullShapeConfiguration* convexConfiguration = static_cast<Physics::ConvexHullShapeConfiguration*>(request.m_shapeConfiguration.get());

            physx::PxConvexMesh* convexMesh = nullptr;
            PhysXSystemRequestBus::BroadcastResult(convexMesh, &PhysXSystemRequests::CreateConvexMesh,
                convexConfiguration->m_vertexData, convexConfiguration->m_vertexCount, convexConfiguration->m_vertexStride);

            physx::PxConvexMeshGeometry convex(convexMesh);
            m_world->sweep(convex, pose, dir, distance, pxResult);

            convexMesh->release();

            break;
        }
        default:
            AZ_Error("PhysX World", false, "Unsupported shape type in shape cast request.");
            break;
        }

        if (pxResult.hasBlock)
        {
            Physics::ShapeCastHit hitResult;

            hitResult.m_position = LYVec3FromPxVec3(pxResult.block.position);
            hitResult.m_normal = LYVec3FromPxVec3(pxResult.block.normal);

            float hitDistance = pxResult.block.distance;
            hitResult.m_hitTime = hitDistance / distance; // Is this in [0;1] range?

            physx::PxRigidActor* actor = pxResult.block.actor;
            void* userData = actor->userData;
            if (userData)
            {
                hitResult.m_entityId = AZ::EntityId(reinterpret_cast<AZ::u64>(userData));
            }

            hitResult.m_hitBody = nullptr; // Get this from the entity

            result.m_hits.push_back(hitResult);
        }
    }

    AZ::u32 PhysXWorld::QueueRayCast(const Physics::RayCastRequest& request, const Physics::RayCastResultCallback& callback)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
        return 0;
    }

    void PhysXWorld::CancelQueuedRayCast(AZ::u32 queueHandle)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
    }

    AZ::u32 PhysXWorld::QueueShapeCast(const Physics::ShapeCastRequest& request, const Physics::ShapeCastResultCallback& callback)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
        return 0;
    }

    void PhysXWorld::CancelQueuedShapeCast(AZ::u32 queueHandle)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
    }

    void PhysXWorld::AddBody(const Physics::Ptr<Physics::WorldBody>& body)
    {
        physx::PxRigidActor* rigidActor = static_cast<physx::PxRigidActor*>(body->GetNativePointer());

        if (!m_world)
        {
            AZ_Error("PhysX World", false, "Tried to add body to invalid world.");
            return;
        }

        if (!rigidActor)
        {
            AZ_Error("PhysX World", false, "Tried to add invalid PhysX body to world.");
            return;
        }

        m_world->addActor(*rigidActor);
    }

    void PhysXWorld::RemoveBody(const Physics::Ptr<Physics::WorldBody>& body)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
    }

    void PhysXWorld::AddAction(const Physics::Ptr<Physics::Action>& action)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
    }

    void PhysXWorld::RemoveAction(const Physics::Ptr<Physics::Action>& action)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
    }

    void PhysXWorld::SetCollisionFilter(const Physics::Ptr<Physics::WorldCollisionFilter>& collisionFilter)
    {
        AZ_Warning("PhysX World", false, "Not implemented.");
    }

    void PhysXWorld::Update(float deltaTime)
    {
        deltaTime = AZ::GetClamp(deltaTime, 0.0f, m_maxDeltaTime);

        if (m_fixedDeltaTime != 0.0f)
        {
            m_accumulatedTime += deltaTime;

            while (m_accumulatedTime >= m_fixedDeltaTime)
            {
                m_world->simulate(m_fixedDeltaTime);
                m_world->fetchResults(true);
                m_accumulatedTime -= m_fixedDeltaTime;
            }
        }
        else
        {
            m_world->simulate(deltaTime);
            m_world->fetchResults(true);
        }

        EntityPhysXEventBus::Broadcast(&EntityPhysXEvents::OnPostStep);
    }

    AZ::Crc32 PhysXWorld::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::PhysXWorld;
    }

    void* PhysXWorld::GetNativePointer() const
    {
        return m_world;
    }

    void PhysXWorld::OnAddBody(const Physics::Ptr<Physics::WorldBody>& body)
    {
    }

    void PhysXWorld::OnRemoveBody(const Physics::Ptr<Physics::WorldBody>& body)
    {
    }

    void PhysXWorld::OnAddAction(const Physics::Ptr<Physics::Action>& action)
    {
    }

    void PhysXWorld::OnRemoveAction(const Physics::Ptr<Physics::Action>& action)
    {
    }
} // namespace PhysX
