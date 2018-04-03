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

#include "PhysicsComponent.h"

namespace LmbrCentral
{
    /**
     * Configuration data for ParticlePhysicsComponent.
     */
    struct ParticlePhysicsConfiguration
    {
        AZ_CLASS_ALLOCATOR(ParticlePhysicsConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(ParticlePhysicsConfiguration, "{FA5938D3-BD8F-4106-8177-C17F944D8B31}");
        static void Reflect(AZ::ReflectContext* context);

		//	Collider types
		int GetColTypes() const;

        //! Whether the entity is initially enabled in the physics simulation.
        //! Entities can be enabled later via PhysicsComponentRequests::Enable(true).
        bool m_enabledInitially = true;

        //! Total mass of entity, in kg
        float m_mass = 1.f;

        //! Radius of the particle, in m
        float m_radius = 0.05f;

        //! The thickness to use when sliding or lying flat
        float m_thicknessWhenLying = 0.05f;

        //! Indicates whether this component can interact with proximity triggers
        bool m_interactsWithTriggers = true;

        //! Indicates whether this component collides with character capsules (if set to false, the component will collide with the physics proxy instead)
        bool m_collideWithCharacterCapsule = true;

        // Entity types to collide with
        bool m_collideWithTerrain = true;
        bool m_collideWithStatic = true;
        bool m_collideWithRigid = true;
        bool m_collideWithSleepingRigid = true;
        bool m_collideWithLiving = true;
        bool m_collideWithIndependent = true;
        bool m_collideWithParticles = true;

        //! Stop moving as soon as it hits something
        bool m_stopOnFirstContact = false;

        //! Align the particle's forward vector with the current trajectory
        bool m_orientToTrajectory = true;

        //! Allow the particle to spin
        bool m_allowSpin = true;

        //! Minimum speed below which the particle will stop bouncing
        float m_bounceSpeedThreshold = 1.0f;

        //! Minimum speed below which the particle will come to rest
        float m_sleepSpeedThreshold = 0.02f;

        //! If the particle is allowed to roll (otherwise it will only slide)
        bool m_allowRolling = true;

        //! Aligns the entity with the roll axis when rolling
        bool m_alignRollAxis = false;

        //! The local axis to align with the roll axis
        AZ::Vector3 m_rollAxis = AZ::Vector3(0, 0, 0);

        //! Apply an impulse to anything it collides with
        bool m_applyHitImpulse = true;

        //! Resistance applied when in air
        float m_airResistance = 0.0f;
        
        //! Resistance applied when in water
        float m_waterResistance = 0.5f;

        //! Surface index to use for collisions
        int m_surfaceIndex = 0;

        //! Enable custom gravity. Otherwise uses the world gravity
        bool m_useCustomGravity = false;

        //! Custom gravity direction and magnitude
        AZ::Vector3 m_customGravity = AZ::Vector3(0, 0, 0);
    };

    /*!
     * Physics component for a Particle movable object.
     */
    class ParticlePhysicsComponent
        : public PhysicsComponent
    {
    public:
        AZ_COMPONENT(ParticlePhysicsComponent, "{84658098-1AB2-4317-8391-1D3024325F96}", PhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        ParticlePhysicsComponent() = default;
        explicit ParticlePhysicsComponent(const ParticlePhysicsConfiguration& configuration);
        ~ParticlePhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponent
        void ConfigurePhysicalEntity() override;
        void ConfigureCollisionGeometry() override;
        pe_type GetPhysicsType() const override { return PE_PARTICLE; }
        bool CanInteractWithProximityTriggers() const override { return m_configuration.m_interactsWithTriggers; }
        bool CanCollideWithCharacterCapsule() const override { return m_configuration.m_collideWithCharacterCapsule; }
        bool IsEnabledInitially() const override { return m_configuration.m_enabledInitially; }
        ////////////////////////////////////////////////////////////////////////

        const ParticlePhysicsConfiguration& GetConfiguration() const { return m_configuration; }

    protected:

        ParticlePhysicsConfiguration m_configuration;
    };
} // namespace LmbrCentral
