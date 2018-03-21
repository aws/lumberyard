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
#include <AzFramework/Physics/RigidBody.h>

namespace PhysX
{
    class PhysXRigidBodyComponent;

    /**
    * Configuration data for PhysXRigidBodyComponent.
    */
    struct PhysXRigidBodyConfiguration
    {
        AZ_CLASS_ALLOCATOR(PhysXRigidBodyConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PhysXRigidBodyConfiguration, "{2B300168-D69A-43EB-8010-8F25E84BEFDC}");
        static void Reflect(AZ::ReflectContext* context);

        // not in the generic API currently
        AZ::Vector3                                 m_centreOfMassOffset = AZ::Vector3::CreateZero();
        AZ::Vector3                                 m_scale = AZ::Vector3::CreateOne();

        // note these settings are mapped to settings in the generic physics API
        // but we maintain them separately here to allow specialization for PhysX

        // Basic initial settings.
        Physics::MotionType                         m_motionType = Physics::MotionType::Static;
        AZ::Vector3                                 m_initialLinearVelocity = AZ::Vector3::CreateZero();
        AZ::Vector3                                 m_initialAngularVelocity = AZ::Vector3::CreateZero();
        Physics::Ptr<Physics::ShapeConfiguration>   m_bodyShape = nullptr;

        // Simulation parameters.
        float                                       m_mass = 1.f;
        bool                                        m_computeInertiaDiagonal = true;
        AZ::Vector3                                 m_inertiaDiagonal = AZ::Vector3::CreateOne();
        float                                       m_linearDamping = 0.05f;
        float                                       m_angularDamping = 0.15f;
        float                                       m_sleepMinEnergy = 0.005f;
        bool                                        m_continuousCollisionDetection = false;

        // editor visibility
        bool ShowDynamicOnlyAttributes() const
        {
            return m_motionType == Physics::MotionType::Dynamic;
        }

        bool ShowInertia() const
        {
            return m_motionType == Physics::MotionType::Dynamic && !m_computeInertiaDiagonal;
        }
    };

    /**
    * PhysX specific implementation of generic physics API RigidBody class.
    */
    class PhysXRigidBody
        : public Physics::RigidBody
    {
    public:
        friend class PhysXRigidBodyComponent;

        AZ_CLASS_ALLOCATOR(PhysXRigidBody, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(PhysXRigidBody, "{30CD41DD-9783-47A1-B935-9E5634238F45}", Physics::RigidBody);
        static void Reflect(AZ::ReflectContext* context);

        PhysXRigidBody() = default;
        PhysXRigidBody(const Physics::Ptr<Physics::RigidBodySettings>& settings);
        ~PhysXRigidBody() = default;

        // Physics::RigidBody
        Physics::Ptr<Physics::NativeShape> GetNativeShape() override;

        virtual AZ::Vector3 GetCenterOfMassWorld() const override;
        virtual AZ::Vector3 GetCenterOfMassLocal() const override;

        AZ::Matrix3x3 GetInverseInertiaWorld() const override;
        AZ::Matrix3x3 GetInverseInertiaLocal() const override;

        float GetEnergy() const override;

        Physics::MotionType GetMotionType() const override;
        void SetMotionType(Physics::MotionType motionType) override;

        float GetMass() const override;
        float GetInverseMass() const override;
        void SetMass(float mass) const override;

        AZ::Vector3 GetLinearVelocity() const override;
        void SetLinearVelocity(AZ::Vector3 velocity) override;

        AZ::Vector3 GetAngularVelocity() const override;
        void SetAngularVelocity(AZ::Vector3 angularVelocity) override;

        AZ::Vector3 GetLinearVelocityAtWorldPoint(AZ::Vector3 worldPoint) override;

        void ApplyLinearImpulse(AZ::Vector3 impulse) override;
        void ApplyLinearImpulseAtWorldPoint(AZ::Vector3 impulse, AZ::Vector3 worldPoint) override;

        void ApplyAngularImpulse(AZ::Vector3 angularImpulse) override;

        bool IsAwake() const override;
        void ForceAsleep() override;
        void ForceAwake() override;

        // Physics::WorldBody
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;

        Physics::ObjectCollisionFilter GetCollisionFilter() const override;
        void SetCollisionFilter(const Physics::ObjectCollisionFilter& filter) override;

        AZ::Aabb GetAabb() const override;

        void RayCast(const Physics::RayCastRequest& request, Physics::RayCastResult& result) const override;

        // Physics::ReferenceBase
        virtual AZ::Crc32 GetNativeType() const override;
        virtual void* GetNativePointer() const override;

        // Not in API but needed to support PhysicsComponentBus
        float GetLinearDamping();
        void SetLinearDamping(float damping);
        float GetAngularDamping();
        void SetAngularDamping(float damping);
        float GetSleepThreshold();
        void SetSleepThreshold(float threshold);
    private:
        physx::PxRigidActor*             m_pxRigidActor = nullptr;
        PhysXRigidBodyConfiguration   m_config;
        AZ::Transform                    m_transform = AZ::Transform::CreateIdentity();
        AZ::Entity*                      m_entity = nullptr;

        physx::PxRigidActor* CreatePhysXActor();
        void AddShapes();
        void ReleasePhysXActor();
        void SetConfig(const PhysXRigidBodyConfiguration& config);
        void SetShape(Physics::Ptr<Physics::ShapeConfiguration> shape);
        void SetScale(const AZ::Vector3&& scale);
        void SetEntity(AZ::Entity* entity);
        void GetTransformUpdateFromPhysX();

        AZ_DISABLE_COPY_MOVE(PhysXRigidBody);
    };
} // namespace PhysX