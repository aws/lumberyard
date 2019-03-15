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

#include <AzCore/Component/Component.h>
#include <PhysXCharacters/SystemBus.h>
#ifdef PHYSX_CHARACTERS_EDITOR
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#endif // ifdef PHYSX_CHARACTERS_EDITOR

namespace PhysXCharacters
{
    class CharacterController;

    class SystemComponent
        : public AZ::Component
        , public SystemRequestBus::Handler
        , public Physics::SystemNotificationBus::Handler
        , public Physics::CharacterSystemRequestBus::Handler
#ifdef PHYSX_CHARACTERS_EDITOR
        , public AzToolsFramework::EditorEntityContextNotificationBus::Handler
#endif // ifdef PHYSX_CHARACTERS_EDITOR
    {
    public:
        AZ_COMPONENT(SystemComponent, "{A3D46E1B-1879-4295-A49B-9D8291F07F01}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // SystemRequestBus
        physx::PxControllerManager* GetControllerManager(const Physics::World& world) override;

        // Physics::SystemNotificationBus
        virtual void OnPreWorldDestroy(Physics::World* world) override;

        // Physics::CharacterSystemRequestBus
        virtual AZStd::unique_ptr<Physics::Character> CreateCharacter(const Physics::CharacterConfiguration& characterConfig,
            const Physics::ShapeConfiguration& shapeConfig, Physics::World& world) override;
        virtual void UpdateCharacters(const Physics::World& world, float deltaTime) override;

#ifdef PHYSX_CHARACTERS_EDITOR
        // AzToolsFramework::EditorEntityContextNotificationBus
        void OnStopPlayInEditor() override;
#endif // ifdef PHYSX_CHARACTERS_EDITOR

    private:
        AZStd::vector<AZStd::pair<const Physics::World*, physx::PxControllerManager*>> m_controllerManagers;
    };
} // namespace PhysXCharacters
