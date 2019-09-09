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

#include <PhysXCharacters_precompiled.h>
#include <API/CharacterControllerCosmeticReplica.h>
#include <PhysX/SystemComponentBus.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <API/CharacterController.h>
#include <PhysXCharacters/NativeTypeIdentifiers.h>

namespace PhysXCharacters
{
    void CharacterControllerCosmeticReplica::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CharacterControllerCosmeticReplica>()
                ->Version(1)
                ;
        }
    }

    AZ::Quaternion CalculateOrientation(const Physics::ShapeConfiguration& shapeConfig, const AZ::Vector3& upDirection)
    {
        if (shapeConfig.GetShapeType() == Physics::ShapeType::Box || upDirection.IsZero())
        {
            return AZ::Quaternion::CreateIdentity();
        }

        return AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisZ(), upDirection.GetNormalized());
    }

    float GetHeight(const Physics::ShapeConfiguration& shapeConfig)
    {
        const Physics::ShapeType& shapeType = shapeConfig.GetShapeType();
        if (shapeType == Physics::ShapeType::Box)
        {
            return static_cast<const Physics::BoxShapeConfiguration&>(shapeConfig).m_dimensions.GetZ();
        }
        if (shapeType == Physics::ShapeType::Capsule)
        {
            return static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfig).m_height;
        }

        AZ_Error("PhysX Character Controller Cosmetic Replica", false, "Only box and capsule shapes are supported.  Shape type was %i", shapeType);
        return 0.0f;
    }

    AZStd::unique_ptr<Physics::ShapeConfiguration> GetScaledShapeConfiguration(float scale, const Physics::ShapeConfiguration& shapeConfig)
    {
        const Physics::ShapeType& shapeType = shapeConfig.GetShapeType();
        if (shapeType == Physics::ShapeType::Box)
        {
            auto scaledBoxConfig = AZStd::make_unique<Physics::BoxShapeConfiguration>();
            scaledBoxConfig->m_dimensions = scale * static_cast<const Physics::BoxShapeConfiguration&>(shapeConfig).m_dimensions;
            return AZStd::move(scaledBoxConfig);
        }
        else if (shapeType == Physics::ShapeType::Capsule)
        {
            auto scaledCapsuleConfig = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
            const auto& capsuleConfig = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfig);
            scaledCapsuleConfig->m_height = scale * capsuleConfig.m_height;
            scaledCapsuleConfig->m_radius = scale * capsuleConfig.m_radius;
            return AZStd::move(scaledCapsuleConfig);
        }
        else
        {
            AZ_Error("PhysX Character Controller Cosmetic Replica", false, "Only box and capsule shapes are supported.  Shape type was %i", shapeType);
            return AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
        }
    }

    CharacterControllerCosmeticReplica::CharacterControllerCosmeticReplica(const Physics::CharacterConfiguration& characterConfig,
        const Physics::ShapeConfiguration& shapeConfig, Physics::World& world)
    {
        const Physics::ShapeType& shapeType = shapeConfig.GetShapeType();
        if (shapeType != Physics::ShapeType::Box && shapeType != Physics::ShapeType::Capsule)
        {
            AZ_Error("PhysX Character Controller Cosmetic Replica", false, "Could not create character controller replica.  "
                "Only box and capsule shapes are supported.  Shape type was %i", shapeType);
            return;
        }

        // rigid body
        Physics::RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_debugName = "Character Controller Cosmetic Replica";
        rigidBodyConfig.m_entityId = characterConfig.m_entityId;
        rigidBodyConfig.m_kinematic = true;
        rigidBodyConfig.m_orientation = CalculateOrientation(shapeConfig, characterConfig.m_upDirection);
        m_transform.SetRotationPartFromQuaternion(rigidBodyConfig.m_orientation);
        rigidBodyConfig.m_position = characterConfig.m_position;
        Physics::SystemRequestBus::BroadcastResult(m_rigidBody, &Physics::SystemRequests::CreateRigidBody, rigidBodyConfig);

        if (!m_rigidBody)
        {
            AZ_Error("PhysX Character Controller Cosmetic Replica", false, "Failed to create replica actor.");
            return;
        }

        // up direction
        if (shapeType == Physics::ShapeType::Box || characterConfig.m_upDirection.IsZero())
        {
            m_upDirection = AZ::Vector3::CreateAxisZ();
        }
        else
        {
            m_upDirection = characterConfig.m_upDirection.GetNormalized();
        }

        // shape
        Physics::ColliderConfiguration colliderConfig;
        colliderConfig.m_collisionGroupId = characterConfig.m_collisionGroupId;
        colliderConfig.m_collisionLayer = characterConfig.m_collisionLayer;
        colliderConfig.m_materialSelection = characterConfig.m_materialSelection;
        float height = GetHeight(shapeConfig);
        colliderConfig.m_position = 0.5f * height * AZ::Vector3::CreateAxisZ();
        m_centreOffset = 0.5f * height * m_upDirection;

        // PhysX specific character config settings
        if (characterConfig.RTTI_GetType() == CharacterControllerConfiguration::RTTI_Type())
        {
            const auto& extendedConfig = static_cast<const CharacterControllerConfiguration&>(characterConfig);
            auto scaledShapeConfig = GetScaledShapeConfiguration(extendedConfig.m_scaleCoefficient, shapeConfig);
            Physics::SystemRequestBus::BroadcastResult(m_shape, &Physics::SystemRequests::CreateShape, colliderConfig, *scaledShapeConfig);
            static_cast<physx::PxShape*>(m_shape->GetNativePointer())->setContactOffset(extendedConfig.m_contactOffset);
        }
        else
        {
            Physics::SystemRequestBus::BroadcastResult(m_shape, &Physics::SystemRequests::CreateShape, colliderConfig, shapeConfig);
        }

        m_rigidBody->AddShape(m_shape);

        // misc settings
        m_stepHeight = characterConfig.m_stepHeight;
        m_maximumSlopeAngle = characterConfig.m_maximumSlopeAngle;
        m_actorUserData = PhysX::ActorData(static_cast<physx::PxRigidActor*>(m_rigidBody->GetNativePointer()));
        m_actorUserData.SetCharacter(this);
        m_actorUserData.SetEntityId(characterConfig.m_entityId);

        world.AddBody(*m_rigidBody);
        CreateShadowBody(characterConfig, world);
    }

    CharacterControllerCosmeticReplica::~CharacterControllerCosmeticReplica()
    {
        if (m_rigidBody)
        {
            m_rigidBody->RemoveShape(m_shape);
            m_rigidBody = nullptr;
        }
        m_shape = nullptr;
    }

    void CharacterControllerCosmeticReplica::Move(const AZ::Vector3& worldPosition)
    {
        m_transform.SetPosition(worldPosition);
        if (m_rigidBody)
        {
            m_rigidBody->SetKinematicTarget(m_transform);
        }
        if (m_shadowBody)
        {
            m_shadowBody->SetKinematicTarget(m_transform);
        }
    }

    void CharacterControllerCosmeticReplica::CreateShadowBody(const Physics::CharacterConfiguration& characterConfig, Physics::World& world)
    {
        Physics::RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_kinematic = true;
        rigidBodyConfig.m_debugName = characterConfig.m_debugName + " (Shadow)";
        rigidBodyConfig.m_entityId = characterConfig.m_entityId;
        Physics::SystemRequestBus::BroadcastResult(m_shadowBody, &Physics::SystemRequests::CreateRigidBody, rigidBodyConfig);
        world.AddBody(*m_shadowBody);
    }

    // Physics::Character
    AZ::Vector3 CharacterControllerCosmeticReplica::GetBasePosition() const
    {
        return m_transform.GetPosition();
    }

    void CharacterControllerCosmeticReplica::SetBasePosition(const AZ::Vector3& position)
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "SetBasePosition - the replica cannot be directly controlled.");
    }

    void CharacterControllerCosmeticReplica::SetRotation(const AZ::Quaternion& rotation)
    {
        if (m_shadowBody)
        {
            AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(rotation, GetBasePosition());
            m_shadowBody->SetTransform(transform);
        }
    }

    AZ::Vector3 CharacterControllerCosmeticReplica::GetCenterPosition() const
    {
        return m_transform.GetPosition() + m_centreOffset;
    }

    float CharacterControllerCosmeticReplica::GetStepHeight() const
    {
        return m_stepHeight;
    }

    void CharacterControllerCosmeticReplica::SetStepHeight(float stepHeight)
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "SetStepHeight - the replica cannot be directly controlled.");
    }

    AZ::Vector3 CharacterControllerCosmeticReplica::GetUpDirection() const
    {
        return m_upDirection;
    }

    void CharacterControllerCosmeticReplica::SetUpDirection(const AZ::Vector3& upDirection)
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "SetUpDirection - the replica cannot be directly controlled.");
    }

    float CharacterControllerCosmeticReplica::GetSlopeLimitDegrees() const
    {
        return m_maximumSlopeAngle;
    }

    void CharacterControllerCosmeticReplica::SetSlopeLimitDegrees(float slopeLimitDegrees)
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "SetSlopeLimitDegrees - the replica cannot be directly controlled.");
    }

    AZ::Vector3 CharacterControllerCosmeticReplica::GetVelocity() const
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "GetVelocity - not yet supported.");
        return AZ::Vector3::CreateZero();
    }

    AZ::Vector3 CharacterControllerCosmeticReplica::TryRelativeMove(const AZ::Vector3& deltaPosition, float deltaTime)
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "TryRelativeMove - the replica cannot be directly controlled.");
        return GetBasePosition();
    }

    void CharacterControllerCosmeticReplica::CheckSupport(const AZ::Vector3& direction, float distance,
        const Physics::CharacterSupportInfo& supportInfo)
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "CheckSupport - not yet supported.");
    }

    // Physics::WorldBody
    AZ::EntityId CharacterControllerCosmeticReplica::GetEntityId() const
    {
        return m_rigidBody ? m_rigidBody->GetEntityId() : AZ::EntityId();
    }

    Physics::World* CharacterControllerCosmeticReplica::GetWorld() const
    {
        return m_rigidBody ? m_rigidBody->GetWorld() : nullptr;
    }

    AZ::Transform CharacterControllerCosmeticReplica::GetTransform() const
    {
        return AZ::Transform::CreateTranslation(GetBasePosition());
    }

    void CharacterControllerCosmeticReplica::SetTransform(const AZ::Transform& transform)
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "SetTransform - the replica cannot be directly controlled.");
    }

    AZ::Vector3 CharacterControllerCosmeticReplica::GetPosition() const
    {
        return GetBasePosition();
    }

    AZ::Quaternion CharacterControllerCosmeticReplica::GetOrientation() const
    {
        return AZ::Quaternion::CreateIdentity();
    }

    AZ::Aabb CharacterControllerCosmeticReplica::GetAabb() const
    {
        return m_rigidBody ? m_rigidBody->GetAabb() : AZ::Aabb::CreateNull();
    }

    void CharacterControllerCosmeticReplica::RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const
    {
        AZ_WarningOnce("PhysX Character Controller Cosmetic Replica", false, "RayCast - not yet supported.");
    }

    AZ::Crc32 CharacterControllerCosmeticReplica::GetNativeType() const
    {
        return NativeTypeIdentifiers::CharacterControllerReplica;
    }

    void* CharacterControllerCosmeticReplica::GetNativePointer() const
    {
        if (m_rigidBody)
        {
            return m_rigidBody->GetNativePointer();
        }

        return nullptr;
    }

    void CharacterControllerCosmeticReplica::AddToWorld(Physics::World& world)
    {
        if (m_rigidBody)
        {
            m_rigidBody->AddToWorld(world);
        }

        if (m_shadowBody)
        {
            m_shadowBody->AddToWorld(world);
        }
    }

    void CharacterControllerCosmeticReplica::RemoveFromWorld(Physics::World& world)
    {
        if (m_rigidBody)
        {
            m_rigidBody->RemoveFromWorld(world);
        }

        if (m_shadowBody)
        {
            m_shadowBody->RemoveFromWorld(world);
        }
    }

    void CharacterControllerCosmeticReplica::AttachShape(AZStd::shared_ptr<Physics::Shape> shape)
    {
        if (m_shadowBody)
        {
            m_shadowBody->AddShape(shape);
        }
    }
} // namespace PhysXCharacters
