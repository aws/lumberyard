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
#include "StaticPhysicsComponent.h"

namespace LmbrCentral
{
    /**
     * Configuration data for EditorStaticPhysicsComponent.
     */
    struct EditorStaticPhysicsConfig
        : public AzFramework::StaticPhysicsConfig
    {
        AZ_CLASS_ALLOCATOR(EditorStaticPhysicsConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(EditorStaticPhysicsConfig, "{8309995F-A628-57DA-AAFE-2E04A257EC40}", AzFramework::StaticPhysicsConfig);
        static void Reflect(AZ::ReflectContext* context);

        // currently, there's no difference between EditorStaticPhysicsConfig and its base class.
    };

    /**
     * In-editor physics component for a solid object that cannot move.
     */
    class EditorStaticPhysicsComponent
        : public EditorPhysicsComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorStaticPhysicsComponent, AzFramework::EditorStaticPhysicsComponentTypeId, EditorPhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);
        
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            EditorPhysicsComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC("StaticPhysicsService", 0x20ca6e80));
        }

        EditorStaticPhysicsComponent() = default;
        ~EditorStaticPhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        ////////////////////////////////////////////////////////////////////////
        // Component
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;
        ////////////////////////////////////////////////////////////////////////

        EditorStaticPhysicsConfig m_configuration;
    };
} // namespace LmbrCentral