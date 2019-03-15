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
    static const float epsilon = 1e-3f;

    enum class SlopeBehaviour
    {
        PreventClimbing,
        ForceSliding
    };

    /// Allows PhysX specific character controller properties that are not included in the generic configuration.
    class CharacterControllerConfiguration
        : public Physics::CharacterConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(CharacterControllerConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(CharacterControllerConfiguration, "{23A8DFD6-7DA4-4CB3-BBD3-7FB58DEE6F9D}", Physics::CharacterConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        CharacterControllerConfiguration() = default;
        CharacterControllerConfiguration(const CharacterControllerConfiguration&) = default;
        virtual ~CharacterControllerConfiguration() = default;

        SlopeBehaviour m_slopeBehaviour = SlopeBehaviour::PreventClimbing; ///< Behaviour on surfaces above maximum slope.
        float m_contactOffset = 0.1f; ///< Extra distance outside the controller used to give smoother contact resolution.
        float m_scaleCoefficient = 0.8f; ///< Scalar coefficient used to scale the controller, usually slightly smaller than 1.
    };

    class CharacterController
        : public Physics::Character
    {
        friend class CharacterControllerComponent;

    public:
        AZ_CLASS_ALLOCATOR(CharacterController, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(CharacterController, "{A75A7D19-BC21-4F7E-A3D9-05031D2DFC94}", Physics::Character);
        static void Reflect(AZ::ReflectContext* context);

        CharacterController() = default;
        explicit CharacterController(physx::PxController* pxController);
        ~CharacterController();

        void SetFilterDataAndShape(const Physics::CharacterConfiguration& characterConfig);
        void SetUserData(const Physics::CharacterConfiguration& characterConfig);
        void SetActorName(const AZStd::string& name = "Character Controller");
        void SetMinimumMovementDistance(float distance);
        void CreateShadowBody(const Physics::CharacterConfiguration& configuration, Physics::World&);

        // Physics::Character
        AZ::Vector3 GetBasePosition() const override;
        void SetBasePosition(const AZ::Vector3& position) override;
        AZ::Vector3 GetCenterPosition() const override;
        float GetStepHeight() const override;
        void SetStepHeight(float stepHeight) override;
        AZ::Vector3 GetUpDirection() const override;
        void SetUpDirection(const AZ::Vector3& upDirection) override;
        float GetSlopeLimitDegrees() const override;
        void SetSlopeLimitDegrees(float slopeLimitDegrees) override;
        AZ::Vector3 GetVelocity() const override;
        AZ::Vector3 TryRelativeMove(const AZ::Vector3& deltaPosition, float deltaTime) override;
        void SetRotation(const AZ::Quaternion& rotation) override;
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

        // CharacterController specific
        void Resize(float height);
        float GetHeight() const;
        void SetHeight(float height);
        float GetRadius() const;
        void SetRadius(float radius);
        float GetHalfSideExtent() const;
        void SetHalfSideExtent(float halfSideExtent);
        float GetHalfForwardExtent() const;
        void SetHalfForwardExtent(float halfForwardExtent);

    private:
        /// Update the velocity based on the outcome of the controller's movement in the simulation.  This can differ
        /// from the desired velocity, for example if the character is stuck in a corner its observed velocity may be
        /// zero in spite of having a non-zero desired velocity.
        void UpdateObservedVelocity(const AZ::Vector3& observedVelocity);

        physx::PxController* m_pxController = nullptr; ///< The underlying PhysX controller.
        float m_minimumMovementDistance = 0.0f; ///< To avoid jittering, the controller will not attempt to move distances below this.
        AZ::Vector3 m_observedVelocity = AZ::Vector3::CreateZero(); ///< Velocity observed in the simulation, may not match desired.
        PhysX::ActorData m_actorUserData; ///< Used to populate the user data on the PxActor associated with the controller.
        physx::PxFilterData m_filterData; ///< Controls filtering for collisions with other objects and scene queries.
        physx::PxControllerFilters m_pxControllerFilters; ///< Controls which objects the controller interacts with when moving.
        AZStd::shared_ptr<Physics::Material> m_material; ///< The generic physics API material for the controller.
        AZStd::shared_ptr<Physics::Shape> m_shape; ///< The generic physics API shape associated with the controller.
        AZStd::shared_ptr<Physics::RigidBody> m_shadowBody; ///< A kinematic-synchronised rigid body used to store additional colliders.
        AZStd::string m_name = "Character Controller"; ///< Name to set on the PhysX actor associated with the controller.
    };
} // namespace PhysXCharacters