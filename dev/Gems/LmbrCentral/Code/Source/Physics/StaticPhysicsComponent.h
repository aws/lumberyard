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
    /*!
     * Configuration data for StaticPhysicsComponent.
     */
    struct StaticPhysicsConfiguration
    {
        AZ_CLASS_ALLOCATOR(StaticPhysicsConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(StaticPhysicsConfiguration, "{2129576B-A548-4F3E-A2A1-87851BF48838}");
        static void Reflect(AZ::ReflectContext* context);

        //! Whether the entity is initially enabled in the physics simulation.
        //! Entities can be enabled later via PhysicsComponentRequests::Enable(true).
        bool m_enabledInitially = true;
    };

    /*!
     *  Physics component for a solid object that cannot move.
     */
    class StaticPhysicsComponent
        : public PhysicsComponent
    {
    public:
        AZ_COMPONENT(StaticPhysicsComponent, "{95D89791-6397-41BC-AAC5-95282C8AD9D4}", PhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        StaticPhysicsComponent() = default;
        explicit StaticPhysicsComponent(const StaticPhysicsConfiguration& configuration);
        ~StaticPhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponent
        void ConfigurePhysicalEntity() override;
        void ConfigureCollisionGeometry() override {};
        pe_type GetPhysicsType() const override { return PE_STATIC; }
        bool CanInteractWithProximityTriggers() const override { return false; }
        bool IsEnabledInitially() const override { return m_configuration.m_enabledInitially; }
        ////////////////////////////////////////////////////////////////////////

        const StaticPhysicsConfiguration& GetConfiguration() const { return m_configuration; }

    protected:

        StaticPhysicsConfiguration m_configuration;
    };
} // namespace LmbrCentral