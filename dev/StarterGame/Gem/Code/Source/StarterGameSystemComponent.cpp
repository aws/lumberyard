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

#include <StarterGameSystemComponent.h>

#include <GameStates/StarterGameStateMainMenu.h>
#include <GameStates/StarterGameStateOptionsMenu.h>
#include <GameStates/StarterGameStateLevelPaused.h>
#include <GameStates/StarterGameStateLevelRunning.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/RTTI/BehaviorContext.h>

#include <platform_impl.h>

namespace StarterGame
{
    void StarterGameSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            PlayerData::Reflect(*serialize);

            serialize->Class<StarterGameSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<StarterGameSystemComponent>("StarterGame", "Starter Game system component configuration settings")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->EBus<PlayerDataRequestsBus>("PlayerData", "PlayerDataRequestsBus")
                ->Attribute(AZ::Script::Attributes::Category, GAME_NAME)
                ->Event("AddCoins", &PlayerDataRequestsBus::Events::AddCoins, { {{"nbCoins", ""}} })
                ->Event("AddItemToInventory", &PlayerDataRequestsBus::Events::AddItemToInventory, { {{"ItemType", ""}} })
                ->Event("ReduceHealth", &PlayerDataRequestsBus::Events::ReduceHealth, { {{"NbPoints", ""}} })
                ->Event("AddHealth", &PlayerDataRequestsBus::Events::AddHealth, { {{"NbPoints", ""}} })
                ->Event("GetHealth", &PlayerDataRequestsBus::Events::GetHealth)
                ;
            behaviorContext->Class<StarterGameSystemComponent>()->RequestBus("PlayerDataRequestsBus");
        }
    }

    void StarterGameSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("StarterGameService"));
    }

    void StarterGameSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("StarterGameService"));
    }

    void StarterGameSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void StarterGameSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void StarterGameSystemComponent::Init()
    {
    }

    void StarterGameSystemComponent::Activate()
    {
        GameState::GameStateRequests::AddGameStateFactoryOverrideForType<GameStateSamples::GameStateMainMenu>([]()
        {
            return AZStd::make_shared<StarterGameStateMainMenu>();
        });
        GameState::GameStateRequests::AddGameStateFactoryOverrideForType<GameStateSamples::GameStateOptionsMenu>([]()
        {
            return AZStd::make_shared<StarterGameStateOptionsMenu>();
        });
        GameState::GameStateRequests::AddGameStateFactoryOverrideForType<GameStateSamples::GameStateLevelPaused>([]()
        {
            return AZStd::make_shared<StarterGameStateLevelPaused>();
        });
        GameState::GameStateRequests::AddGameStateFactoryOverrideForType<GameStateSamples::GameStateLevelRunning>([]()
        {
            return AZStd::make_shared<StarterGameStateLevelRunning>();
        });
        m_Game.reset(new StarterGameGame());

    }

    void StarterGameSystemComponent::Deactivate()
    {
        m_Game.reset();
        GameState::GameStateRequests::RemoveGameStateFactoryOverrideForType<GameStateSamples::GameStateLevelRunning>();
        GameState::GameStateRequests::RemoveGameStateFactoryOverrideForType<GameStateSamples::GameStateMainMenu>();
    }
}
