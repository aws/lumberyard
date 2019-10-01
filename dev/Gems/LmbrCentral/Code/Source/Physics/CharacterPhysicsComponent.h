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

#include <AzCore/Math/Crc.h>
#include <IProximityTriggerSystem.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <AzFramework/Physics/PhysicsComponentBus.h>
#include <AzFramework/Physics/PhysicsSystemComponentBus.h>

#include "PhysicsSystemComponent.h"

//CryCommon Includes
#include <physinterface.h>
#include <LmbrCentral/Physics/CryCharacterPhysicsBus.h>
#include <LmbrCentral/Physics/CryPhysicsComponentRequestBus.h>

namespace AZ
{
    class Transform;
}

namespace LmbrCentral
{
    /*!
     * In-game physics component for characters.
     * An entity with a character physics component can be used for players, bots or other complicated entities
     */
    class CharacterPhysicsComponent
        : public AZ::Component
        , private AzFramework::PhysicsComponentRequestBus::Handler
        , private CryPhysicsComponentRequestBus::Handler
        , private EntityPhysicsEventBus::Handler
        , private AZ::TransformNotificationBus::Handler
        , private CryCharacterPhysicsRequestBus::Handler
    {
    public:


        //////////////////////////////////////////////////////////////////////////
        // Private types

        /*!
        * Configuration data for CryPlayerPhysicsBehavior.
        * Exposes the living-entity specific properties from CryPhysics.
        */
        struct CryPlayerPhysicsConfiguration
        {
            AZ_CLASS_ALLOCATOR(CryPlayerPhysicsConfiguration, AZ::SystemAllocator, 0);
            AZ_TYPE_INFO(CryPlayerPhysicsConfiguration, "{97A1C07E-0444-4FAC-A394-8317AFE5696B}");
            virtual ~CryPlayerPhysicsConfiguration() = default;
            static void Reflect(AZ::ReflectContext* context);

            /*!
            * A reflected version of pe_player_dimensions.
            * These parameters control the shape and positioning of the player.
            *
            * Note: CryPhysics imbues living-entities with cylinder/capsule geometry.
            * Additional colliders are not required.
            *
            * Note: The default values for these variables were taken from the
            * defaults used in CLivingEntity's constructor.
            */
            struct PlayerDimensions
            {
                AZ_TYPE_INFO(PlayerDimensions, "{64B9DBDA-90D4-4D3D-88EA-36810AF2C98F}");
                virtual ~PlayerDimensions() {}
                static void Reflect(AZ::ReflectContext* context);

                //! Returns pe_player_dimensions based upon variables in PlayerDimensions
                pe_player_dimensions GetParams() const;

                const bool s_defaultLivingEntityUseCapsule = false;
                const float s_defaultLivingEntityColliderRadius = 0.4f;
                const float s_defaultLivingEntityColliderHalfHeight = 0.6f;
                const float s_defaultLivingEntityHeightCollider = 1.1f;
                const float s_defaultLivingEntityHeightPivot = 0.f;
                const float s_defaultLivingEntityHeightEye = 1.7f;
                const float s_defaultLivingEntityHeightHead = 1.9f;
                const float s_defaultLivingEntityHeadRadius = 0.f;
                const AZ::Vector3 s_defaultLivingEntityUnprojectionDirection = AZ::Vector3(0.f, 0.f, 1.f);
                const float s_defaultLivingEntityMaxUnprojection = 0.f;
                const float s_defaultLivingEntityGroundContactEpsilon = 0.006f;

                //! switches between capsule and cylinder collider geometry
                bool m_useCapsule = s_defaultLivingEntityUseCapsule;

                //! radius of collision cylinder/capsule
                float m_colliderRadius = s_defaultLivingEntityColliderRadius;

                //! half-height of straight section of collision cylinder/capsule
                float m_colliderHalfHeight = s_defaultLivingEntityColliderHalfHeight;

                //! vertical offset of collision geometry center
                float m_heightCollider = s_defaultLivingEntityHeightCollider;

                //! offset from central ground position that is considered entity center
                float m_heightPivot = s_defaultLivingEntityHeightPivot;

                //! vertical offset of camera
                float m_heightEye = s_defaultLivingEntityHeightEye;

                //! center.z of the head geometry
                float m_heightHead = s_defaultLivingEntityHeightHead;

                //! radius of the 'head' geometry (used for camera offset)
                float m_headRadius = s_defaultLivingEntityHeadRadius;

                //! unprojection direction to test in case the new position overlaps with the environment (can be 0 for 'auto')
                AZ::Vector3 m_unprojectionDirection = s_defaultLivingEntityUnprojectionDirection;

                //! maximum allowed unprojection
                float m_maxUnprojection = s_defaultLivingEntityMaxUnprojection;

                //! the amount that the living needs to move upwards before ground contact is lost.
                //! defaults to which ever is greater 0.004, or 0.01*geometryHeight
                float m_groundContactEpsilon = s_defaultLivingEntityGroundContactEpsilon;
            };

            /*!
            * A reflected version of pe_player_dynamics.
            * These parameters control how the player moves.
            *
            * The default values for these variables come from the defaults
            * used in CLivingEntity's constructor.
            */
            struct PlayerDynamics
            {
                AZ_TYPE_INFO(PlayerDynamics, "{B1237004-14E1-4327-8774-D2C5796230E7}");
                virtual ~PlayerDynamics() {}
                static void Reflect(AZ::ReflectContext* context);

                //! Returns pe_player_dynamics based upon variables in PlayerDynamics
                pe_player_dynamics GetParams() const;

                const float s_defaultLivingEntityMass = 80.f;
                const float s_defaultLivingEntityInertia = 0.f;
                const float s_defaultLivingEntityInertiaAcceleration = 0.f;
                const float s_defaultTimeImpulseRecover = 5.f;
                const float s_defaultLivingEntityAirControl = 0.1f;
                const float s_defaultLivingEntityAirResistance = 0.f;
                const bool s_defaultLivingEntityUseCustomGravity = false;
                const AZ::Vector3 s_defaultLivingEntityGravity = AZ::Vector3(0.f, 0.f, -9.8f);
                const float s_defaultLivingEntityNodSpeed = 60.f;
                const bool s_defaultLivingEntityIsActive = true;
                const bool s_defaultLivingEntityReleaseGroundColliderWhenNotActive = true;
                const bool s_defaultLivingEntityIsSwimming = false;
                const int s_defaultLivingEntitySurfaceIndex = 0;
                const float s_defaultLivingEntityMinFallAngle = 70.2f;
                const float s_defaultLivingEntityMinSlideAngle = 36.f;
                const float s_defaultLivingEntityMaxClimbAngle = 54.f;
                const float s_defaultLivingEntityMaxJumpAngle = 54.f;
                const float s_defaultLivingEntityMaxVelocityGround = 10.f;
                const bool s_defaultLivingEntityCollideWithTerrain = true;
                const bool s_defaultLivingEntityCollideWithStatic = true;
                const bool s_defaultLivingEntityCollideWithRigid = true;
                const bool s_defaultLivingEntityCollideWithSleepingRigid = true;
                const bool s_defaultLivingEntityCollideWithLiving = true;
                const bool s_defaultLivingEntityCollideWithIndependent = true;

                //! mass (in kg)
                float m_mass = s_defaultLivingEntityMass;

                //! inertia koefficient, the more it is, the less inertia is; 0 means no inertia
                float m_inertia = s_defaultLivingEntityInertia;

                //! inertia on acceleration
                float m_inertiaAcceleration = s_defaultLivingEntityInertiaAcceleration;

                //! forcefully turns on inertia for that duration after receiving an impulse
                float m_timeImpulseRecover = s_defaultTimeImpulseRecover;

                //! air control coefficient 0..1, 1 - special value (total control of movement)
                float m_airControl = s_defaultLivingEntityAirControl;

                //! standard air resistance
                float m_airResistance = s_defaultLivingEntityAirResistance;

                //! True: use custom gravity. False: use world gravity.
                bool m_useCustomGravity = s_defaultLivingEntityUseCustomGravity;

                //! Custom gravity vector for this object (ignored when m_useCustomGravity is false)
                AZ::Vector3 m_gravity = s_defaultLivingEntityGravity;

                //! vertical camera shake speed after landings
                float m_nodSpeed = s_defaultLivingEntityNodSpeed;

                //! Setting false disables all simulation for the character,
                //! apart from moving along the requested velocity
                bool m_isActive = s_defaultLivingEntityIsActive;

                //! When true, if the living entity is not active, the ground collider,
                //! if any, will be explicitly released during the simulation step.
                bool m_releaseGroundColliderWhenNotActive = s_defaultLivingEntityReleaseGroundColliderWhenNotActive;

                //! whether entity is swimming (is not bound to ground plane)
                bool m_isSwimming = s_defaultLivingEntityIsSwimming;

                //! surface identifier for collisions
                int m_surfaceIndex = s_defaultLivingEntitySurfaceIndex;

                //! player starts falling when slope is steeper than this (angle is in degrees)
                float m_minFallAngle = s_defaultLivingEntityMinFallAngle;

                //! if surface slope is more than this angle, player starts sliding (angle is in degrees)
                float m_minSlideAngle = s_defaultLivingEntityMinSlideAngle;

                //! player cannot climb surface which slope is steeper than this angle (angle is in degrees)
                float m_maxClimbAngle = s_defaultLivingEntityMaxClimbAngle;

                //! player is not allowed to jump towards ground if this angle is exceeded (angle is in degrees)
                float m_maxJumpAngle = s_defaultLivingEntityMaxJumpAngle;

                //! player cannot stand on surfaces that are moving faster than this
                float m_maxVelocityGround = s_defaultLivingEntityMaxVelocityGround;

                // physics-entity types to collider against
                bool m_collideWithTerrain = s_defaultLivingEntityCollideWithTerrain;
                bool m_collideWithStatic = s_defaultLivingEntityCollideWithStatic;
                bool m_collideWithRigid = s_defaultLivingEntityCollideWithSleepingRigid;
                bool m_collideWithSleepingRigid = s_defaultLivingEntityCollideWithSleepingRigid;
                bool m_collideWithLiving = s_defaultLivingEntityCollideWithLiving;
                bool m_collideWithIndependent = s_defaultLivingEntityCollideWithIndependent;

                //! Whether or not to record collisions on this entity.
                bool m_recordCollisions = false;

                //! Maximum number of collisions to be recorded per frame.
                int m_maxRecordedCollisions = 1;
            };

            PlayerDimensions m_dimensions;
            PlayerDynamics m_dynamics;
        };

        //////////////////////////////////////////////////////////////////////////

        AZ_COMPONENT(CharacterPhysicsComponent, "{D707D6C5-3EFA-4275-82EB-A954F845D324}");
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
            provided.push_back(AZ_CRC("CharacterPhysicsService", 0x3cd4f075));
            provided.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PhysicsService", 0xa7350d22));
            incompatible.push_back(AZ_CRC("CharacterPhysicsService", 0x3cd4f075));
        }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

    private:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponentRequests
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() override;
        void AddImpulse(const AZ::Vector3& impulse) override;
        void AddImpulseAtPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldSpacePoint) override;
        AZ::Vector3 GetVelocity() override;
        void SetVelocity(const AZ::Vector3& velocity) override;
        AZ::Vector3 GetAcceleration() override;
        AZ::Vector3 GetAngularVelocity() override;
        float GetMass() override;
        void SetMass(float mass) override;
        AZ::Aabb GetAabb() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // CryPhysicsComponentRequests
        IPhysicalEntity* GetPhysicalEntity() override;
        void GetPhysicsParameters(pe_params& outParameters) override;
        void SetPhysicsParameters(const pe_params& parameters) override;
        void GetPhysicsStatus(pe_status& outStatus) override;
        void ApplyPhysicsAction(const pe_action& action, bool threadSafe) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EntityPhysicsEvents

        //! When object moves in physics simulation,
        //! update its Transform in component-land.
        void OnPostStep(const EntityPhysicsEvents::PostStep& event) override;
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformNotifications
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;
        void OnParentChanged(AZ::EntityId oldParent, AZ::EntityId newParent) override;
        //////////////////////////////////////////////////////////////////////////

        bool ConfigurePhysicalEntity();

        ////////////////////////////////////////////////////////////////////////
        // CharacterPhysicsRequestBus
        void RequestVelocity(const AZ::Vector3& velocity, int jump) override;
        bool IsCryCharacterControllerPresent() const override { return true; }
        ////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        CryPlayerPhysicsConfiguration m_configuration;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Non reflected data
        AZStd::shared_ptr<IPhysicalEntity> m_physicalEntity;
        bool m_isApplyingPhysicsToEntityTransform = false;
        SProximityElement* m_proximityTriggerProxy = nullptr;
        AZ::Transform m_previousEntityTransform = AZ::Transform::CreateIdentity();
        //////////////////////////////////////////////////////////////////////////
    };
} // namespace LmbrCentral
