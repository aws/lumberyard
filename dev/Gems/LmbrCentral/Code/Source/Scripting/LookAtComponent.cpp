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
#include "LmbrCentral_precompiled.h"
#include "LookAtComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    //=========================================================================
    void LookAtComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LookAtComponent, AZ::Component>()
                ->Version(1)
                ->Field("Target", &LookAtComponent::m_targetId)
                ->Field("ForwardAxis", &LookAtComponent::m_forwardAxis)
                ;
        }
    }

    //=========================================================================
    void LookAtComponent::Activate()
    {
        if (m_targetId.IsValid())
        {
            AZ::EntityBus::Handler::BusConnect(m_targetId);
        }
    }

    //=========================================================================
    void LookAtComponent::Deactivate()
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect();
        AZ::EntityBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void LookAtComponent::OnEntityActivated(const AZ::EntityId& /*entityId*/)
    {
        AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
        AZ::TransformNotificationBus::MultiHandler::BusConnect(m_targetId);
    }

    //=========================================================================
    void LookAtComponent::OnEntityDeactivated(const AZ::EntityId& /*entityId*/)
    {
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(GetEntityId());
        AZ::TransformNotificationBus::MultiHandler::BusDisconnect(m_targetId);
    }

    //=========================================================================
    void LookAtComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& /*world*/)
    {
        // See corresponding function in EditorLookAtComponent for comment.
        AZ::TickBus::Handler::BusConnect();
    }

    //=========================================================================
    void LookAtComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*time*/)
    {
        RecalculateTransform();
        AZ::TickBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void LookAtComponent::RecalculateTransform()
    {
        if (m_targetId.IsValid())
        {
            AZ::TransformNotificationBus::MultiHandler::BusDisconnect(GetEntityId());
            {
                AZ::Transform currentTM = AZ::Transform::CreateIdentity();
                EBUS_EVENT_ID_RESULT(currentTM, GetEntityId(), AZ::TransformBus, GetWorldTM);

                AZ::Transform targetTM = AZ::Transform::CreateIdentity();
                EBUS_EVENT_ID_RESULT(targetTM, m_targetId, AZ::TransformBus, GetWorldTM);

                AZ::Transform lookAtTransform = AzFramework::CreateLookAt(
                    currentTM.GetPosition(),
                    targetTM.GetPosition(),
                    m_forwardAxis
                    );

                EBUS_EVENT_ID(GetEntityId(), AZ::TransformBus, SetWorldTM, lookAtTransform);
            }
            AZ::TransformNotificationBus::MultiHandler::BusConnect(GetEntityId());
        }
    }

} // namespace LmbrCentral
