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
     * Configuration data for RigidPhysicsComponent.
     */
    struct RigidPhysicsConfiguration
    {
        AZ_CLASS_ALLOCATOR(RigidPhysicsConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(RigidPhysicsConfiguration, "{4D4211C2-4539-444F-A8AC-B0C8417AA579}");
        static void Reflect(AZ::ReflectContext* context);

        //! Whether total mass is specified, or calculated at spawn time based on density and volume
        enum class MassOrDensity : uint32_t
        {
            Mass,
            Density,
        };

        bool UseMass() const
        {
            return m_specifyMassOrDensity == MassOrDensity::Mass;
        }

        bool UseDensity() const
        {
            return m_specifyMassOrDensity == MassOrDensity::Density;
        }

        //! Whether the entity is initially enabled in the physics simulation.
        //! Entities can be enabled later via PhysicsComponentRequests::Enable(true).
        bool m_enabledInitially = true;

        //! Whether total mass is specified, or calculated at spawn time based on density and volume
        MassOrDensity m_specifyMassOrDensity = MassOrDensity::Density;

        //! Total mass of entity, in kg
        float m_mass = 10.f; // 10kg would be a cube of water with 10cm sides.

        //! Density of the entity, in kg per cubic meter.
        //! The total mass of entity will be calculated at spawn,
        //! based on density and the volume of its geometry.
        //! Mass = Density * Volume
        float m_density = 500.f; // 1000 kg/m^3 is density of water. Oak is 600 kg/m^3.

        //! Whether the entity is initially considered at-rest in the physics simulation.
        //! True: Entity remains still until agitated.
        //! False: Entity will be fall if nothing is below it.
        bool m_atRestInitially = true;
        
        //! Entity will physically react to collisions, rather than only report them.
        bool m_enableCollisionResponse = true;

        //! Indicates whether this component can interact with proximity triggers
        bool m_interactsWithTriggers = true;

        //! Damping value applied while in water.
        float m_buoyancyDamping = 0.f;

        //! Water density strength.
        float m_buoyancyDensity = 1.f;

        //! Water resistance strength.
        float m_buoyancyResistance = 1.f;

        //! Movement damping.
        float m_simulationDamping = 0.f;

        //! Minimum energy under which object will sleep.
        float m_simulationMinEnergy = 0.002f;

        //! Whether or not to record collisions on this entity.
        bool m_recordCollisions = true;

        //! Maximum number of collisions to be recorded per frame.
        int m_maxRecordedCollisions = 1;
    };

    /*!
     * Physics component for a rigid movable object.
     */
    class RigidPhysicsComponent
        : public PhysicsComponent
    {
    public:
        AZ_COMPONENT(RigidPhysicsComponent, "{BF2ED241-6364-4D78-8008-498EF2A2659C}", PhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        RigidPhysicsComponent() = default;
        explicit RigidPhysicsComponent(const RigidPhysicsConfiguration& configuration);
        ~RigidPhysicsComponent() override = default;

        const RigidPhysicsConfiguration& GetConfiguration() const { return m_configuration; }

    protected:
        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponent
        void ConfigurePhysicalEntity() override;
        void ConfigureCollisionGeometry() override;
        pe_type GetPhysicsType() const override { return PE_RIGID; }
        bool CanInteractWithProximityTriggers() const override { return m_configuration.m_interactsWithTriggers; }
        bool IsEnabledInitially() const override { return m_configuration.m_enabledInitially; }
        ////////////////////////////////////////////////////////////////////////

        void Activate() override;

        RigidPhysicsConfiguration m_configuration;
    };
} // namespace LmbrCentral
