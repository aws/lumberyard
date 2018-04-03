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
#include "LmbrCentral_precompiled.h"
#include "ParticlePhysicsComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MathConversion.h>

namespace LmbrCentral
{
    void ParticlePhysicsConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ParticlePhysicsConfiguration>()
                ->Version(1)
                ->Field("EnabledInitially", &ParticlePhysicsConfiguration::m_enabledInitially)
                ->Field("Mass", &ParticlePhysicsConfiguration::m_mass)
                ->Field("Radius", &ParticlePhysicsConfiguration::m_radius)
                ->Field("ThicknessWhenLying", &ParticlePhysicsConfiguration::m_thicknessWhenLying)
                ->Field("InteractsWithTriggers", &ParticlePhysicsConfiguration::m_interactsWithTriggers)
                ->Field("CollideWithCharacterCapsule", &ParticlePhysicsConfiguration::m_collideWithCharacterCapsule)
                ->Field("Collide With Terrain", &ParticlePhysicsConfiguration::m_collideWithTerrain)
                ->Field("Collide With Static", &ParticlePhysicsConfiguration::m_collideWithStatic)
                ->Field("Collide With Rigid", &ParticlePhysicsConfiguration::m_collideWithRigid)
                ->Field("Collide With Sleeping Rigid", &ParticlePhysicsConfiguration::m_collideWithSleepingRigid)
                ->Field("Collide With Living", &ParticlePhysicsConfiguration::m_collideWithLiving)
                ->Field("Collide With Independent", &ParticlePhysicsConfiguration::m_collideWithIndependent)
                ->Field("Collide With Particles", &ParticlePhysicsConfiguration::m_collideWithParticles)
                ->Field("StopOnFirstContact", &ParticlePhysicsConfiguration::m_stopOnFirstContact)
                ->Field("OrientToTrajectory", &ParticlePhysicsConfiguration::m_orientToTrajectory)
                ->Field("AllowSpin", &ParticlePhysicsConfiguration::m_allowSpin)
                ->Field("BounceSpeedThreshold", &ParticlePhysicsConfiguration::m_bounceSpeedThreshold)
                ->Field("SleepSpeedThreshold", &ParticlePhysicsConfiguration::m_sleepSpeedThreshold)
                ->Field("AllowRolling", &ParticlePhysicsConfiguration::m_allowRolling)
                ->Field("AlignRollAxis", &ParticlePhysicsConfiguration::m_alignRollAxis)
                ->Field("RollAxis", &ParticlePhysicsConfiguration::m_rollAxis)
                ->Field("ApplyHitImpulse", &ParticlePhysicsConfiguration::m_applyHitImpulse)
                ->Field("AirResistance", &ParticlePhysicsConfiguration::m_airResistance)
                ->Field("WaterResistance", &ParticlePhysicsConfiguration::m_waterResistance)
                ->Field("SurfaceIndex", &ParticlePhysicsConfiguration::m_surfaceIndex)
                ->Field("UseCustomGravity", &ParticlePhysicsConfiguration::m_useCustomGravity)
                ->Field("CustomGravity", &ParticlePhysicsConfiguration::m_customGravity)
                ;
        }
    }

    void ParticlePhysicsComponent::Reflect(AZ::ReflectContext* context)
    {
        ParticlePhysicsConfiguration::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ParticlePhysicsComponent, PhysicsComponent>()
                ->Version(1)
                ->Field("Configuration", &ParticlePhysicsComponent::m_configuration)
                ;
        }
    }

    ParticlePhysicsComponent::ParticlePhysicsComponent(const ParticlePhysicsConfiguration& configuration)
        : m_configuration(configuration)
    {
    }

    void ParticlePhysicsComponent::ConfigureCollisionGeometry()
    {
        // Particle entities do not have colliders as they model a moving sphere
    }

    void ParticlePhysicsComponent::ConfigurePhysicalEntity()
    {
        // Particle parameters
        pe_params_particle particleParams;
        particleParams.flags =
            pef_never_affect_triggers     // don't generate events when moving through triggers
            | particle_traceable          // entity is registered in the entity grid
            | pef_log_collisions          // entity will log collision events
            | pef_log_poststep            // entity will log EventPhysPostStep events
            ;

        if (m_configuration.m_stopOnFirstContact)
        {
            particleParams.flags |= particle_single_contact; // full stop after first contact
        }

        if (!m_configuration.m_allowRolling)
        {
            particleParams.flags |= particle_no_roll; // 'sliding' mode; entity's 'normal' vector axis will be alinged with the ground normal
        }

        if (!m_configuration.m_orientToTrajectory)
        {
            particleParams.flags |= particle_no_path_alignment; // unless set, entity's y axis will be aligned along the movement trajectory
        }

        if (!m_configuration.m_allowSpin)
        {
            particleParams.flags |= particle_no_spin; // disables spinning while flying
        }

        if (!m_configuration.m_collideWithParticles)
        {
            particleParams.flags |= particle_no_self_collisions; // disables collisions with other particles
        }

        if (!m_configuration.m_applyHitImpulse)
        {
            particleParams.flags |= particle_no_impulse; // particle will not add hit impulse (expecting that some other system will)
        }

        particleParams.mass = m_configuration.m_mass;
        particleParams.size = m_configuration.m_radius;
        particleParams.thickness = m_configuration.m_thicknessWhenLying;
        particleParams.collTypes = m_configuration.GetColTypes();

        AZ::Transform transform = GetEntity()->GetTransform()->GetWorldTM();
        particleParams.q0 = AZQuaternionToLYQuaternion(AZ::Quaternion::CreateFromTransform(transform).GetNormalized());
        particleParams.heading = AZVec3ToLYVec3(transform.GetBasisY().GetNormalizedSafe());

        particleParams.velocity = 0.0f;
        particleParams.normal = Vec3(0.0f, 0.0f, 1.0f);

        particleParams.kAirResistance = m_configuration.m_airResistance;
        particleParams.kWaterResistance = m_configuration.m_waterResistance;
        particleParams.surface_idx = m_configuration.m_surfaceIndex;

        if (m_configuration.m_alignRollAxis)
        {
            particleParams.rollAxis = AZVec3ToLYVec3(m_configuration.m_rollAxis);
        }

        if (m_configuration.m_useCustomGravity)
        {
            particleParams.gravity = AZVec3ToLYVec3(m_configuration.m_customGravity);
        }

        particleParams.minBounceVel = m_configuration.m_bounceSpeedThreshold;
        particleParams.minVel = m_configuration.m_sleepSpeedThreshold;

        m_physicalEntity->SetParams(&particleParams, 1);
    }

    int ParticlePhysicsConfiguration::GetColTypes() const
    {
        return (m_collideWithTerrain ? ent_terrain : 0)
            | (m_collideWithStatic ? ent_static : 0)
            | (m_collideWithRigid ? ent_rigid : 0)
            | (m_collideWithSleepingRigid ? ent_sleeping_rigid : 0)
            | (m_collideWithLiving ? ent_living : 0)
            | (m_collideWithIndependent ? ent_independent : 0)
            ;
    }
} // namespace LmbrCentral