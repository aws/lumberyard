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

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <PhysXRigidBody.h>
#include <PhysXRigidBodyComponent.h>

namespace PhysX
{
    /**
    * Configuration data for EditorPhysXRigidBodyComponent.
    */
    struct EditorPhysXRigidBodyConfiguration
        : public PhysXRigidBodyConfiguration
    {
        AZ_CLASS_ALLOCATOR(EditorPhysXRigidBodyConfiguration, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(EditorPhysXRigidBodyConfiguration, "{27297024-5A99-4C58-8614-4EF18137CE69}", PhysXRigidBodyConfiguration);
        static void Reflect(AZ::ReflectContext* context);
    };

    /**
    * Class for in-editor PhysX rigid physics components.
    */
    class EditorPhysXRigidBodyComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorPhysXRigidBodyComponent, "{F2478E6B-001A-4006-9D7E-DCB5A6B041DD}", AzToolsFramework::Components::EditorComponentBase);
        static void Reflect(AZ::ReflectContext* context);

        EditorPhysXRigidBodyComponent() = default;
        explicit EditorPhysXRigidBodyComponent(const EditorPhysXRigidBodyConfiguration& configuration);
        ~EditorPhysXRigidBodyComponent() = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("PhysXRigidBodyService", 0x1d4c64a8));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            dependent.push_back(AZ_CRC("PhysXColliderService", 0x4ff43f7c));
        }

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;
        ////////////////////////////////////////////////////////////////////////

        const EditorPhysXRigidBodyConfiguration& GetConfiguration() const { return m_config; }

    private:
        EditorPhysXRigidBodyConfiguration m_config;
    };
} // namespace PhysX
