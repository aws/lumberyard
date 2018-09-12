
#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Quaternion.h>

#include <AzFramework/Physics/Base.h>
#include <AzFramework/Physics/CollisionFilter.h>
#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/ShapeConfiguration.h>

namespace Physics
{
    class ShapeConfiguration;
    class World;
    struct MaterialProperties;

    /**
     * 
     */
    enum class MotionType : AZ::u8
    {
        Static,         ///< Body is collidable, but static (does not simulate). Useful for static geometry.
        Dynamic,        ///< Body is fully dynamic and simulated. Useful for all types of real-time simulated objects.
        Keyframed,      ///< Body is collidable and controlled directly (does not simulate). Useful for moving platforms or animated objects.
    };

    /**
     * 
     */
    enum class SleepType : AZ::u8
    {
        None,
        Simple,
        Energy,
    };

    /**
     * 
     */
    enum class ContinuousType : AZ::u8
    {
        None,
        Simple,
        Full,
    };

    class RigidBody;

    /**
     * 
     */
    class RigidBodySettings : public WorldBodySettings
    {
    public:

        AZ_CLASS_ALLOCATOR(RigidBodySettings, AZ::SystemAllocator, 0);
        AZ_RTTI(RigidBodySettings, "{ACFA8900-8530-4744-AF00-AA533C868A8E}", WorldBodySettings);

        static Ptr<RigidBodySettings> Create()
        {
            return aznew RigidBodySettings();
        }

        // Basic initial settings.
        MotionType                      m_motionType                = MotionType::Static;
        AZ::Vector3                     m_initialLinearVelocity     = AZ::Vector3::CreateZero();
        AZ::Vector3                     m_initialAngularVelocity    = AZ::Vector3::CreateZero();
        Ptr<ShapeConfiguration>         m_bodyShape                 = nullptr;

        // Simulation parameters.
        float                           m_mass                      = 1.f;
        AZ::Matrix3x3                   m_inertiaTensor             = AZ::Matrix3x3::CreateIdentity();
        bool                            m_computeInertiaTensor      = false;
        float                           m_linearDamping             = 0.05f;
        float                           m_angularDamping            = 0.15f;
        SleepType                       m_sleepType                 = SleepType::Energy;
        float                           m_sleepMinEnergy            = 0.5f; 
        ContinuousType                  m_continuousType            = ContinuousType::None;

    private:
        RigidBodySettings()
        {}
    };

    /**
     * 
     */
    class RigidBodyEventHandler : public ReferenceBase
    {
    public:

        friend class RigidBody;

        AZ_CLASS_ALLOCATOR(RigidBodyEventHandler, AZ::SystemAllocator, 0);

        virtual ~RigidBodyEventHandler() = default;

        virtual void OnWake(const Ptr<RigidBody>& rigidBody) = 0;
        virtual void OnSleep(const Ptr<RigidBody>& rigidBody) = 0;

    private:

        Ptr<RigidBody> m_owningBody;
    };

    /**
     * Helper routine for certain physics engines that don't directly expose this property on rigid bodies.
     */
    AZ_INLINE AZ::Matrix3x3 InverseInertiaLocalToWorld(const AZ::Vector3& diag, const AZ::Matrix3x3& rotationToWorld)
    {
        return rotationToWorld * AZ::Matrix3x3::CreateDiagonal(diag) * rotationToWorld.GetTranspose();
    }

    /**
     * 
     */
    class RigidBody : public WorldBody
    {
    public:

        AZ_CLASS_ALLOCATOR(RigidBody, AZ::SystemAllocator, 0);
        AZ_RTTI(RigidBody, "{156E459F-7BB7-4B4E-ADA0-2130D96B7E80}", WorldBody);

        friend class AZStd::intrusive_ptr<RigidBody>;

    public:

        virtual Ptr<NativeShape> GetNativeShape() { return Ptr<NativeShape>(); }

        virtual AZ::Vector3 GetCenterOfMassWorld() const = 0;
        virtual AZ::Vector3 GetCenterOfMassLocal() const = 0;

        virtual AZ::Matrix3x3 GetInverseInertiaWorld() const = 0;
        virtual AZ::Matrix3x3 GetInverseInertiaLocal() const = 0;

        virtual float GetEnergy() const = 0;

        virtual MotionType GetMotionType() const = 0;
        virtual void SetMotionType(MotionType motionType) = 0;

        virtual float GetMass() const = 0;
        virtual float GetInverseMass() const = 0;
        virtual void SetMass(float mass) const = 0;

        /// Retrieves the velocity at center of mass; only linear velocity, no rotational velocity contribution.
        virtual AZ::Vector3 GetLinearVelocity() const = 0;
        virtual void SetLinearVelocity(AZ::Vector3 velocity) = 0;

        virtual AZ::Vector3 GetAngularVelocity() const = 0;
        virtual void SetAngularVelocity(AZ::Vector3 angularVelocity) = 0;

        /// Get the world linear velocity of a world space point attached to this body
        virtual AZ::Vector3 GetLinearVelocityAtWorldPoint(AZ::Vector3 worldPoint) = 0;

        /// Applies an impulse to only the linear components of the rigid body
        virtual void ApplyLinearImpulse(AZ::Vector3 impulse) = 0;

        /// Applies an impulse at a point on the rigid body specified in world space
        virtual void ApplyLinearImpulseAtWorldPoint(AZ::Vector3 impulse, AZ::Vector3 worldPoint) = 0;

        virtual void ApplyAngularImpulse(AZ::Vector3 angularImpulse) = 0;

        virtual bool IsAwake() const = 0;
        virtual void ForceAsleep() = 0;
        virtual void ForceAwake() = 0;

        virtual void RegisterEventHandler(const Ptr<RigidBodyEventHandler>& handler)
        {
            AZ_Assert(m_eventHandlers.end() == AZStd::find(m_eventHandlers.begin(), m_eventHandlers.end(), handler), "Event handler is already registered.");

            m_eventHandlers.push_back(handler.get());
        }

        virtual void UnregisterEventHandler(const Ptr<RigidBodyEventHandler>& handler)
        {
            auto iter = AZStd::find(m_eventHandlers.begin(), m_eventHandlers.end(), handler);
            if (iter != m_eventHandlers.end())
            {
                m_eventHandlers.erase(iter);
            }
        }

        const AZStd::vector<Ptr<RigidBodyEventHandler>>& GetRigidBodyEventHandlers() const
        {
            return m_eventHandlers;
        }

    protected:

        RigidBody()
            : WorldBody(RigidBodySettings::Create())
        {
        }

        explicit RigidBody(const Ptr<RigidBodySettings>& settings)
            : WorldBody(settings)
        {
            if (settings && settings->m_bodyShape)
            {
                m_shapeHierarchyMaterialMap = settings->m_bodyShape->m_shapeHierarchyMaterialMap;
            }
        }

        ~RigidBody() override
        {
        }

        AZStd::vector<Ptr<RigidBodyEventHandler>> m_eventHandlers;

    private:
        AZ_DISABLE_COPY_MOVE(RigidBody);
    };
    
} // namespace Physics
