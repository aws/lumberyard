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
    struct EditorRigidPhysicsConfiguration
        : public RigidPhysicsConfiguration
    {
        AZ_CLASS_ALLOCATOR(EditorRigidPhysicsConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorRigidPhysicsConfiguration, "{AC3B7279-6A22-4764-9F58-329CB0198CE9}", RigidPhysicsConfiguration);
        static void Reflect(AZ::ReflectContext* context);
    };

    /**
     * In-editor physics component for a rigid movable object.
     */
    class EditorRigidPhysicsComponent
        : public EditorPhysicsComponent
    {
    public:
        AZ_EDITOR_COMPONENT(EditorRigidPhysicsComponent, "{BD17E257-BADB-45D7-A8BA-16D6B0BE0881}", EditorPhysicsComponent);
        static void Reflect(AZ::ReflectContext* context);

        EditorRigidPhysicsComponent() = default;
        explicit EditorRigidPhysicsComponent(const EditorRigidPhysicsConfiguration& configuration);
        ~EditorRigidPhysicsComponent() override = default;

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        const EditorRigidPhysicsConfiguration& GetConfiguration() const { return m_configuration; }

    private:

        EditorRigidPhysicsConfiguration m_configuration;
    };

} // namespace LmbrCentral