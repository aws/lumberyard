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
    struct EditorStaticPhysicsConfiguration
        : public StaticPhysicsConfiguration
    {
        AZ_CLASS_ALLOCATOR(EditorStaticPhysicsConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorStaticPhysicsConfiguration, "{3A2ADB05-DB14-4ED5-9F16-C63E075401F1}", StaticPhysicsConfiguration);
        static void Reflect(AZ::ReflectContext* context);
    };

    /**
     * In-editor physics component for a solid object that cannot move.
     */
    class EditorStaticPhysicsComponent
        : public EditorPhysicsComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorStaticPhysicsComponent, "{C8D8C366-F7B7-42F6-8B86-E58FFF4AF984}", EditorPhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorStaticPhysicsComponent() = default;
        explicit EditorStaticPhysicsComponent(const EditorStaticPhysicsConfiguration& configuration);

        ~EditorStaticPhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        const EditorStaticPhysicsConfiguration& GetConfiguration() const { return m_configuration; }

    private:

        EditorStaticPhysicsConfiguration m_configuration;
    };
} // namespace LmbrCentral