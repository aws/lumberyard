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
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Action.h>
#include <AzFramework/Physics/Character.h>
#include <AzFramework/Physics/Ghost.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/World.h>

namespace PhysX
{
    /**
    * PhysX specific implementation of generic physics API World class.
    */
    class PhysXWorld
        : public Physics::World
    {
    public:

        AZ_CLASS_ALLOCATOR(PhysXWorld, AZ::SystemAllocator, 0);
        AZ_RTTI(PhysXWorld, "{C116A4D3-8843-45CA-9F32-F7B5CCB7F3AB}", Physics::World);

        PhysXWorld(AZ::Crc32 id, const Physics::Ptr<Physics::WorldSettings>& settings);
        ~PhysXWorld() override;

        physx::PxScene* GetNativeWorld() const { return m_world; }
        AZ::Crc32 GetWorldId() const { return m_worldId; }

        // Physics::World
        void RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) override;
        AZ::u32 QueueRayCast(const Physics::RayCastRequest& request, const Physics::RayCastResultCallback& callback) override;
        void CancelQueuedRayCast(AZ::u32 queueHandle) override;
        void ShapeCast(const Physics::ShapeCastRequest& request, Physics::ShapeCastResult& result) override;
        AZ::u32 QueueShapeCast(const Physics::ShapeCastRequest& request, const Physics::ShapeCastResultCallback& callback) override;
        void CancelQueuedShapeCast(AZ::u32 queueHandle) override;
        void AddBody(const Physics::Ptr<Physics::WorldBody>& body) override;
        void RemoveBody(const Physics::Ptr<Physics::WorldBody>& body) override;
        void AddAction(const Physics::Ptr<Physics::Action>& action) override;
        void RemoveAction(const Physics::Ptr<Physics::Action>& action) override;
        void SetCollisionFilter(const Physics::Ptr<Physics::WorldCollisionFilter>& collisionFilter) override;
        void Update(float deltaTime) override;
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;
        void OnAddBody(const Physics::Ptr<Physics::WorldBody>& body) override;
        void OnRemoveBody(const Physics::Ptr<Physics::WorldBody>& body) override;
        void OnAddAction(const Physics::Ptr<Physics::Action>& action) override;
        void OnRemoveAction(const Physics::Ptr<Physics::Action>& action) override;

    private:

        physx::PxScene*         m_world = nullptr;
        AZ::Crc32               m_worldId;
        float                   m_maxDeltaTime = 0.0f;
        float                   m_fixedDeltaTime = 0.0f;
        float                   m_accumulatedTime = 0.0f;
    };
} // namespace PhysX