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
#include "StdAfx.h"
#include "EditorNavigationSeedComponent.h"

// Cry pathfinding system
#include <IAISystem.h>
#include <INavigationSystem.h>
#include <MathConversion.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/TransformBus.h>

namespace LmbrCentral
{
    void EditorNavigationSeedComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorNavigationSeedComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Agent Type", &EditorNavigationSeedComponent::m_agentType);

            AZ::EditContext* edit = serializeContext->GetEditContext();
            if (edit)
            {
                edit->Class<EditorNavigationSeedComponent>("Navigation Seed", "Determines reachable navigation nodes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    // Disabled for v1.11
                    //->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c)) 
                    ->Attribute(AZ::Edit::Attributes::Category, "AI")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Navigation.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Navigation.png")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(0, &EditorNavigationSeedComponent::m_agentType, "Agent Type", "Describes the type of the Entity for navigation purposes.")
                        ->Attribute("ChangeNotify", &EditorNavigationSeedComponent::OnAgentTypeChanged);
            }
        }
    }

    EditorNavigationSeedComponent::EditorNavigationSeedComponent()
        : m_agentType("MediumSizedCharacters")
    {
    }

    void EditorNavigationSeedComponent::TriggerReachaibilityRecalculation() const
    {
        // We have to call the legacy method to account for Legacy seeds that might be out there
        gEnv->pAISystem->GetNavigationSystem()->CalculateAccessibility();
    }

    AZ::u32 EditorNavigationSeedComponent::OnAgentTypeChanged() const
    {
        TriggerReachaibilityRecalculation();
        return AZ_CRC("RefreshNone", 0x98a5045b);
    }

    void EditorNavigationSeedComponent::RecalculateReachabilityAroundSelf()
    {
        AZ::Vector3 translation;
        EBUS_EVENT_ID_RESULT(translation, m_entity->GetId(), AZ::TransformBus, GetWorldTranslation);

        auto navigationSystem = gEnv->pAISystem->GetNavigationSystem();
        auto agentType = navigationSystem->GetAgentTypeID(m_agentType.c_str());
        navigationSystem->ComputeAccessibility(AZVec3ToLYVec3(translation), agentType);
    }

    void EditorNavigationSeedComponent::OnTransformChanged(const AZ::Transform&, const AZ::Transform&)
    {
        TriggerReachaibilityRecalculation();
    }

    void EditorNavigationSeedComponent::Activate()
    {
        Base::Activate();

        NavigationSeedRequestsBus::Handler::BusConnect();
        AZ::TransformNotificationBus::Handler::BusConnect(m_entity->GetId());

        TriggerReachaibilityRecalculation();
    }

    void EditorNavigationSeedComponent::Deactivate()
    {
        Base::Deactivate();

        NavigationSeedRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect(m_entity->GetId());
    }
} // LmbrCentral