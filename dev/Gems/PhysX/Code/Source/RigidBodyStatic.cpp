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

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <PxPhysicsAPI.h>
#include <Source/RigidBodyStatic.h>
#include <Source/Utils.h>
#include <PhysX/Utils.h>
#include <Source/Shape.h>
#include <Include/PhysX/NativeTypeIdentifiers.h>

namespace PhysX
{
    RigidBodyStatic::RigidBodyStatic(const Physics::WorldBodyConfiguration& configuration)
    {
        CreatePxActor(configuration);
    }

    void RigidBodyStatic::CreatePxActor(const Physics::WorldBodyConfiguration& configuration)
    {
        if (auto staticBody = PxActorFactories::CreatePxStaticRigidBody(configuration))
        {
            m_staticRigidBody = AZStd::shared_ptr<physx::PxRigidStatic>(staticBody, [](auto& actor)
            {
                PxActorFactories::ReleaseActor(actor);
            });
            m_actorUserData = PhysX::ActorData(m_staticRigidBody.get());
            m_actorUserData.SetRigidBodyStatic(this);
            m_actorUserData.SetEntityId(configuration.m_entityId);
            m_debugName = configuration.m_debugName;
            m_staticRigidBody->setName(m_debugName.c_str());
        }
    }

    void RigidBodyStatic::AddShape(const AZStd::shared_ptr<Physics::Shape>& shape)
    {
        auto pxShape = AZStd::rtti_pointer_cast<PhysX::Shape>(shape);
        if (pxShape && pxShape->GetPxShape())
        {
            m_staticRigidBody->attachShape(*pxShape->GetPxShape());
            m_shapes.push_back(pxShape);
        }
        else
        {
            AZ_Error("PhysX Rigid Body Static", false, "Trying to add an invalid shape.");
        }
    }

    AZStd::shared_ptr<Physics::Shape> RigidBodyStatic::GetShape(AZ::u32 index)
    {
        if (index >= m_shapes.size())
        {
            return nullptr;
        }
        return m_shapes[index];
    }

    AZ::u32 RigidBodyStatic::GetShapeCount()
    {
        return static_cast<AZ::u32>(m_shapes.size());
    }

    // Physics::WorldBody
    Physics::World* RigidBodyStatic::GetWorld() const
    {
        return m_staticRigidBody ? Utils::GetUserData(m_staticRigidBody->getScene()) : nullptr;
    }

    AZ::Transform RigidBodyStatic::GetTransform() const
    {
        return m_staticRigidBody ? PxMathConvert(m_staticRigidBody->getGlobalPose()) : AZ::Transform::CreateZero();
    }

    void RigidBodyStatic::SetTransform(const AZ::Transform & transform)
    {
        if (m_staticRigidBody)
        {
            m_staticRigidBody->setGlobalPose(PxMathConvert(transform));
        }
    }

    AZ::Vector3 RigidBodyStatic::GetPosition() const
    {
        return m_staticRigidBody ? PxMathConvert(m_staticRigidBody->getGlobalPose().p) : AZ::Vector3::CreateZero();
    }

    AZ::Quaternion RigidBodyStatic::GetOrientation() const
    {
        return m_staticRigidBody ? PxMathConvert(m_staticRigidBody->getGlobalPose().q) : AZ::Quaternion::CreateZero();
    }

    AZ::Aabb RigidBodyStatic::GetAabb() const
    {
        return m_staticRigidBody ? PxMathConvert(m_staticRigidBody->getWorldBounds(1.0f)) : AZ::Aabb::CreateNull();
    }

    void RigidBodyStatic::RayCast(const Physics::RayCastRequest & request, Physics::RayCastResult & result) const
    {
        AZ_Warning("PhysX Rigid Body Static", false, "RayCast not implemented.");
    }

    AZ::EntityId RigidBodyStatic::GetEntityId() const
    {
        return m_actorUserData.GetEntityId();
    }

    AZ::Crc32 RigidBodyStatic::GetNativeType() const
    {
        return PhysX::NativeTypeIdentifiers::RigidBodyStatic;
    }

    void* RigidBodyStatic::GetNativePointer() const
    {
        return m_staticRigidBody.get();
    }
}