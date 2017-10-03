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
     * Physics component for a rigid movable object.
     */
    class RigidPhysicsComponent
        : public PhysicsComponent
    {
    public:
        AZ_COMPONENT(RigidPhysicsComponent, RigidPhysicsComponentTypeId, PhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        RigidPhysicsComponent() = default;
        ~RigidPhysicsComponent() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponent
        void ConfigurePhysicalEntity() override;
        void ConfigureCollisionGeometry() override;
        pe_type GetPhysicsType() const override { return PE_RIGID; }
        bool CanInteractWithProximityTriggers() const override { return m_configuration.m_interactsWithTriggers; }
        bool IsEnabledInitially() const override { return m_configuration.m_enabledInitially; }
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // Component
        void Activate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        ////////////////////////////////////////////////////////////////////////

        RigidPhysicsConfig m_configuration;
    };
} // namespace LmbrCentral
