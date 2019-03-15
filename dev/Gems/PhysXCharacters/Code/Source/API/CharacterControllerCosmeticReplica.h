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
#include <AzFramework/Physics/Character.h>
#include <PhysX/UserDataTypes.h>
#include <PxPhysicsAPI.h>

namespace PhysXCharacters
{
    class CharacterControllerConfiguration;

    /// A copy of the character controller which kinematically reproduces its movement in another world.
    /// This can be used if there are multiple worlds and it is still desirable for the character to collider with
    /// objects in those worlds where the character controller is not directly controlled (for example the character
    /// may be directly controlled in a gameplay world, but still need to influence objects in a cosmetic world).
    class CharacterControllerCosmeticReplica
        : public Physics::Character
    {
        friend class CharacterControllerComponent;

    public:
        AZ_CLASS_ALLOCATOR(CharacterControllerCosmeticReplica, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CharacterControllerCosmeticReplica, "{B32BB80F-26AD-4F72-A84A-9FA2F6989D67}", Physics::Character);
        static void Reflect(AZ::ReflectContext* context);

        CharacterControllerCosmeticReplica() = default;
        ~CharacterControllerCosmeticReplica();

        CharacterControllerCosmeticReplica(const Physics::CharacterConfiguration& characterConfig,
            const Physics::ShapeConfiguration& shapeConfig, Physics::World& world);

        /// Kinematically move the controller replica to a new position.
        void Move(const AZ::Vector3& worldPosition);
        void CreateShadowBody(const Physics::CharacterConfiguration& characterConfig, Physics::World&);

        // Physics::Character
        AZ::Vector3 GetBasePosition() const override;
        void SetBasePosition(const AZ::Vector3& position) override;
        void SetRotation(const AZ::Quaternion& rotation) override;
        AZ::Vector3 GetCenterPosition() const override;
        float GetStepHeight() const override;
        void SetStepHeight(float stepHeight) override;
        AZ::Vector3 GetUpDirection() const override;
        void SetUpDirection(const AZ::Vector3& upDirection) override;
        float GetSlopeLimitDegrees() const override;
        void SetSlopeLimitDegrees(float slopeLimitDegrees) override;
        AZ::Vector3 GetVelocity() const override;
        AZ::Vector3 TryRelativeMove(const AZ::Vector3& deltaPosition, float deltaTime) override;
        void CheckSupport(const AZ::Vector3& direction, float distance, const Physics::CharacterSupportInfo& supportInfo) override;
        void AttachShape(AZStd::shared_ptr<Physics::Shape> shape) override;

        // Physics::WorldBody
        AZ::EntityId GetEntityId() const override;
        Physics::World* GetWorld() const override;
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;
        void RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const override;
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;

    private:
        AZStd::shared_ptr<Physics::RigidBody> m_rigidBody = nullptr;
        AZStd::shared_ptr<Physics::Shape> m_shape = nullptr;
        AZStd::shared_ptr<Physics::RigidBody> m_shadowBody; ///< A kinematic-synchronised rigid body used to store additional colliders.
        AZ::Vector3 m_upDirection = AZ::Vector3::CreateAxisZ();
        AZ::Transform m_transform = AZ::Transform::CreateIdentity();
        float m_stepHeight = 0.0f;
        float m_maximumSlopeAngle = 30.0f;
        float m_scale = 0.8f;
        float m_contactOffset = 0.1f;
        AZ::Vector3 m_centreOffset = AZ::Vector3::CreateZero(); ///< Position of the controller replica centre relative to its base.
        AZ::Vector3 m_observedVelocity = AZ::Vector3::CreateZero(); ///< Velocity observed in the simulation, may not match desired.
        PhysX::ActorData m_actorUserData; ///< Used to populate the user data on the PxActor associated with the replica.
    };
} // namespace PhysXCharacters
