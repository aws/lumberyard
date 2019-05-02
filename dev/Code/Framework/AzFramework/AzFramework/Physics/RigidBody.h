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

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>

#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace
{
    class ReflectContext;
}

namespace Physics
{
    class ShapeConfiguration;
    class World;
    class Shape;

    /// Default values used for initializing RigidBodySettings.
    /// These can be modified by Physics Implementation gems.
    struct DefaultRigidBodyConfiguration
    {
        static float m_mass;
        static bool  m_computeInertiaTensor;
        static float m_linearDamping;
        static float m_angularDamping;
        static float m_sleepMinEnergy;
    };

    class RigidBodyConfiguration
        : public WorldBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(RigidBodyConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(RigidBodyConfiguration, "{ACFA8900-8530-4744-AF00-AA533C868A8E}", WorldBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);

        enum PropertyVisibility : AZ::u16
        {
            InitialVelocities = 1 << 0, ///< Whether the initial linear and angular velocities are visible.
            InertiaProperties = 1 << 1, ///< Whether the whole category of inertia properties (mass, compute inertia,
                                        ///< inertia tensor etc) is visible.
            Damping = 1 << 2, ///< Whether linear and angular damping are visible.
            SleepOptions = 1 << 3, ///< Whether the sleep threshold and start asleep options are visible.
            Interpolation = 1 << 4, ///< Whether the interpolation option is visible.
            Gravity = 1 << 5, ///< Whether the effected by gravity option is visible.
            Kinematic = 1 << 6, ///< Whether the option to make the body kinematic is visible.
            ContinuousCollisionDetection = 1 << 7 ///< Whether the option to enable continuous collision detection is visible.
        };

        RigidBodyConfiguration() = default;
        RigidBodyConfiguration(const RigidBodyConfiguration& settings) = default;

        // Visibility functions.
        AZ::Crc32 GetPropertyVisibility(PropertyVisibility property) const;
        void SetPropertyVisibility(PropertyVisibility property, bool isVisible);

        AZ::Crc32 GetInitialVelocitiesVisibility() const;
        /// Returns whether the whole category of inertia settings (mass, inertia, center of mass offset etc) is visible.
        AZ::Crc32 GetInertiaSettingsVisibility() const;
        /// Returns whether the individual inertia tensor field is visible or is hidden because the compute inertia option is selected.
        AZ::Crc32 GetInertiaVisibility() const;
        /// Returns whether the individual centre of mass offset field is visible or is hidden because compute CoM option is selected.
        AZ::Crc32 GetCoMVisibility() const;
        AZ::Crc32 GetDampingVisibility() const;
        AZ::Crc32 GetSleepOptionsVisibility() const;
        AZ::Crc32 GetInterpolationVisibility() const;
        AZ::Crc32 GetGravityVisibility() const;
        AZ::Crc32 GetKinematicVisibility() const;
        AZ::Crc32 GetCCDVisibility() const;

        // Basic initial settings.
        AZ::Vector3 m_initialLinearVelocity = AZ::Vector3::CreateZero();
        AZ::Vector3 m_initialAngularVelocity = AZ::Vector3::CreateZero();
        AZ::Vector3 m_centerOfMassOffset = AZ::Vector3::CreateZero();
        bool m_computeCenterOfMass = true;

        // Simulation parameters.
        float m_mass = DefaultRigidBodyConfiguration::m_mass;
        AZ::Matrix3x3 m_inertiaTensor = AZ::Matrix3x3::CreateIdentity();
        bool m_computeInertiaTensor = DefaultRigidBodyConfiguration::m_computeInertiaTensor;
        float m_linearDamping = DefaultRigidBodyConfiguration::m_linearDamping;
        float m_angularDamping = DefaultRigidBodyConfiguration::m_angularDamping;
        float m_sleepMinEnergy = DefaultRigidBodyConfiguration::m_sleepMinEnergy;
        bool m_startAsleep = false;
        bool m_interpolateMotion = false;
        bool m_gravityEnabled = true;
        bool m_simulated = true;
        bool m_kinematic = false;
        bool m_ccdEnabled = false; ///< Whether continuous collision detection is enabled.

        // Visibility settings.
        AZ::u16 m_propertyVisibilityFlags = (std::numeric_limits<AZ::u16>::max)();
    };

    /// Dynamic rigid body.
    class RigidBody
        : public WorldBody
    {
    public:

        AZ_CLASS_ALLOCATOR(RigidBody, AZ::SystemAllocator, 0);
        AZ_RTTI(RigidBody, "{156E459F-7BB7-4B4E-ADA0-2130D96B7E80}", WorldBody);

    public:
        RigidBody() = default;
        explicit RigidBody(const RigidBodyConfiguration& settings);

        virtual void AddShape(AZStd::shared_ptr<Shape> shape) = 0;
        virtual void RemoveShape(AZStd::shared_ptr<Shape> shape) = 0;
        virtual AZ::u32 GetShapeCount() { return 0; }
        virtual AZStd::shared_ptr<Shape> GetShape(AZ::u32 /*index*/) { return nullptr; }

        virtual AZ::Vector3 GetCenterOfMassWorld() const = 0;
        virtual AZ::Vector3 GetCenterOfMassLocal() const = 0;

        virtual AZ::Matrix3x3 GetInverseInertiaWorld() const = 0;
        virtual AZ::Matrix3x3 GetInverseInertiaLocal() const = 0;

        virtual float GetMass() const = 0;
        virtual float GetInverseMass() const = 0;
        virtual void SetMass(float mass) = 0;
        virtual void SetCenterOfMassOffset(const AZ::Vector3& comOffset) = 0;

        /// Retrieves the velocity at center of mass; only linear velocity, no rotational velocity contribution.
        virtual AZ::Vector3 GetLinearVelocity() const = 0;
        virtual void SetLinearVelocity(const AZ::Vector3& velocity) = 0;
        virtual AZ::Vector3 GetAngularVelocity() const = 0;
        virtual void SetAngularVelocity(const AZ::Vector3& angularVelocity) = 0;
        virtual AZ::Vector3 GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) = 0;
        virtual void ApplyLinearImpulse(const AZ::Vector3& impulse) = 0;
        virtual void ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint) = 0;
        virtual void ApplyAngularImpulse(const AZ::Vector3& angularImpulse) = 0;

        virtual float GetLinearDamping() const = 0;
        virtual void SetLinearDamping(float damping) = 0;
        virtual float GetAngularDamping() const = 0;
        virtual void SetAngularDamping(float damping) = 0;

        virtual bool IsAwake() const = 0;
        virtual void ForceAsleep() = 0;
        virtual void ForceAwake() = 0;
        virtual float GetSleepThreshold() const = 0;
        virtual void SetSleepThreshold(float threshold) = 0;

        virtual bool IsKinematic() const = 0;
        virtual void SetKinematic(bool kinematic) = 0;
        virtual void SetKinematicTarget(const AZ::Transform& targetPosition) = 0;

        virtual void SetGravityEnabled(bool enabled) = 0;
        virtual void SetSimulationEnabled(bool enabled) = 0;
        virtual void SetCCDEnabled(bool enabled) = 0;

        virtual void UpdateCenterOfMassAndInertia(bool computeCenterOfMass, const AZ::Vector3& centerOfMassOffset,
            bool computeInertia, const AZ::Matrix3x3& inertiaTensor) = 0;
    };

    /// Static rigid body.
    class RigidBodyStatic
        : public WorldBody
    {
    public:
        AZ_CLASS_ALLOCATOR(RigidBodyStatic, AZ::SystemAllocator, 0);
        AZ_RTTI(RigidBodyStatic, "{13A677BB-7085-4EDB-BCC8-306548238692}", WorldBody);

        virtual void AddShape(const AZStd::shared_ptr<Shape>& shape) = 0;
        virtual AZ::u32 GetShapeCount() { return 0; }
        virtual AZStd::shared_ptr<Shape> GetShape(AZ::u32 /*index*/) { return nullptr; }
    };
} // namespace Physics
