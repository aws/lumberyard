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

#include "EditorPhysicsComponent.h"
#include "RigidPhysicsComponent.h"

namespace LmbrCentral
{
    /**
     * Configuration data for EditorRigidPhysicsComponent.
     */
    struct EditorRigidPhysicsConfig
        : public AzFramework::RigidPhysicsConfig
    {
        AZ_CLASS_ALLOCATOR(EditorRigidPhysicsConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorRigidPhysicsConfig, "{B2FA5441-9B99-5EFA-A606-82752CA23EE8}", AzFramework::RigidPhysicsConfig);
        static void Reflect(AZ::ReflectContext* context);

        // currently, there's no difference between EditorRigidPhysicsConfig and its base class.
    };

    /**
     * In-editor physics component for a rigid movable object.
     */
    class EditorRigidPhysicsComponent
        : public EditorPhysicsComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRigidPhysicsComponent, AzFramework::EditorRigidPhysicsComponentTypeId, EditorPhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorPhysicsComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("RigidPhysicsService", 0xc03b426c));
            provided.push_back(AZ_CRC("LegacyCryPhysicsService", 0xbb370351));
        }

        EditorRigidPhysicsComponent() = default;
        ~EditorRigidPhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        ////////////////////////////////////////////////////////////////////////

    private:

        EditorRigidPhysicsConfig m_configuration;
    };
} // namespace LmbrCentral