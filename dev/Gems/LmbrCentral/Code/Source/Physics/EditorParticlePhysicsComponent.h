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
#include "ParticlePhysicsComponent.h"

namespace LmbrCentral
{
    /**
     * Configuration data for EditorParticlePhysicsComponent.
     */
    struct EditorParticlePhysicsConfiguration
        : public ParticlePhysicsConfiguration
    {
        AZ_CLASS_ALLOCATOR(EditorParticlePhysicsConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorParticlePhysicsConfiguration, "{99DB247A-9F25-4753-BF82-EAC383FD248D}", ParticlePhysicsConfiguration);
        static void Reflect(AZ::ReflectContext* context);
    };

    /**
     * In-editor physics component for a Particle movable object.
     */
    class EditorParticlePhysicsComponent
        : public EditorPhysicsComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorParticlePhysicsComponent, "{B693BBE6-AA46-41DF-964B-C75300378992}", EditorPhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorParticlePhysicsComponent() = default;
        explicit EditorParticlePhysicsComponent(const EditorParticlePhysicsConfiguration& configuration);
        ~EditorParticlePhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        const EditorParticlePhysicsConfiguration& GetConfiguration() const { return m_configuration; }

    private:

        EditorParticlePhysicsConfiguration m_configuration;
    };

} // namespace LmbrCentral