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

#include <PhysXCharacters_precompiled.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <API/CharacterController.h>
#include <API/Ragdoll.h>
#include <API/Utils.h>
#include <System/SystemComponent.h>
#include <PhysX/SystemComponentBus.h>

namespace PhysXCharacters
{
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("PhysXCharacters", "Provides functionality for simulating characters using PhysX")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("PhysXCharactersService", 0xd8051ca6));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("PhysXCharactersService", 0xd8051ca6));
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("PhysXService", 0x75beae2d));
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    // AZ::Component interface implementation
    void SystemComponent::Init()
    {
    }

    void SystemComponent::Activate()
    {
        SystemRequestBus::Handler::BusConnect();
        Physics::SystemNotificationBus::Handler::BusConnect();
        Physics::CharacterSystemRequestBus::Handler::BusConnect();
#ifdef PHYSX_CHARACTERS_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
#endif // ifdef PHYSX_CHARACTERS_EDITOR
    }

    void SystemComponent::Deactivate()
    {
#ifdef PHYSX_CHARACTERS_EDITOR
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
#endif // ifdef PHYSX_CHARACTERS_EDITOR
        Physics::CharacterSystemRequestBus::Handler::BusDisconnect();
        Physics::SystemNotificationBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();

        for (auto worldManagerPair : m_controllerManagers)
        {
            worldManagerPair.second->release();
        }
        m_controllerManagers.clear();
    }

#ifdef PHYSX_CHARACTERS_EDITOR
    // AzToolsFramework::EditorEntityContextNotificationBus
    void SystemComponent::OnStopPlayInEditor()
    {
    }
#endif // ifdef PHYSX_CHARACTERS_EDITOR

    // SystemRequestBus
    physx::PxControllerManager* SystemComponent::GetControllerManager(const Physics::World& world)
    {
        for (auto worldManagerPair : m_controllerManagers)
        {
            if (worldManagerPair.first == &world)
            {
                return worldManagerPair.second;
            }
        }
        
        physx::PxScene* scene = static_cast<physx::PxScene*>(world.GetNativePointer());
        
        physx::PxControllerManager* manager = nullptr;
        PhysX::SystemRequestsBus::BroadcastResult(manager, &PhysX::SystemRequests::CreateControllerManager, scene);
        
        if (manager)
        {
            manager->setOverlapRecoveryModule(true);
            m_controllerManagers.push_back(AZStd::make_pair(&world, manager));
        }
        else
        {
            AZ_Error("PhysX Character Controller System", false, "Unable to create a Controller Manager.");
        }
        
        return manager;
    }

    // Physics::SystemNotificationBus
    void SystemComponent::OnPreWorldDestroy(Physics::World* world)
    {
        for (size_t worldManagerPairIndex = 0; worldManagerPairIndex < m_controllerManagers.size();)
        {
            if (m_controllerManagers[worldManagerPairIndex].first == world)
            {
                PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::ReleaseControllerManager, 
                    m_controllerManagers[worldManagerPairIndex].second);

                m_controllerManagers[worldManagerPairIndex] = m_controllerManagers.back();
                m_controllerManagers.pop_back();
            }

            else
            {
                worldManagerPairIndex++;
            }
        }
    }

    // Physics::CharacterSystemRequestBus
    AZStd::unique_ptr<Physics::Character> SystemComponent::CreateCharacter(const Physics::CharacterConfiguration&
        characterConfig, const Physics::ShapeConfiguration& shapeConfig, Physics::World& world)
    {
        return Utils::CreateCharacterController(characterConfig, shapeConfig, world);
    }

    void SystemComponent::UpdateCharacters(const Physics::World& world, float deltaTime)
    {
        physx::PxControllerManager* manager = GetControllerManager(world);
        if (manager)
        {
            manager->computeInteractions(deltaTime);
        }
    }
} // namespace PhysXCharacters
