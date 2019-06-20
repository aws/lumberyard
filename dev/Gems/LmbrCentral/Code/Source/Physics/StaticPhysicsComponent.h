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
     *  Physics component for a solid object that cannot move.
     */
    class StaticPhysicsComponent
        : public PhysicsComponent
    {
    public:
        AZ_COMPONENT(StaticPhysicsComponent, AzFramework::StaticPhysicsComponentTypeId, PhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            PhysicsComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("StaticPhysicsService", 0x20ca6e80));
            provided.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
        }

        StaticPhysicsComponent() = default;
        ~StaticPhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponent
        void ConfigurePhysicalEntity() override;
        void ConfigureCollisionGeometry() override {};
        pe_type GetPhysicsType() const override { return PE_STATIC; }
        bool CanInteractWithProximityTriggers() const override { return false; }
        bool IsEnabledInitially() const override { return m_configuration.m_enabledInitially; }
        ////////////////////////////////////////////////////////////////////////

    protected:

        ////////////////////////////////////////////////////////////////////////
        // PhysicsComponent
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        ////////////////////////////////////////////////////////////////////////

        AzFramework::StaticPhysicsConfig m_configuration;
    };
} // namespace LmbrCentral