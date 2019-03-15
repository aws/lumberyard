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

#include <PhysX_precompiled.h>
#include "EditorSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/SystemBus.h>
#include <PhysX/ConfigurationBus.h>
#include <Editor/ConfigStringLineEditCtrl.h>

namespace PhysX
{
    const float EditorSystemComponent::s_minEditorWorldUpdateInterval = 0.1f;
    const float EditorSystemComponent::s_fixedDeltaTime = 0.1f;

    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(1);
        }
    }

    void EditorSystemComponent::Activate()
    {
        Physics::EditorWorldBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();

        PhysX::Configuration configuration;
        PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
        Physics::WorldConfiguration editorWorldConfiguration = configuration.m_worldConfiguration;
        editorWorldConfiguration.m_fixedTimeStep = 0.0f;

        Physics::SystemRequestBus::BroadcastResult(m_editorWorld, &Physics::SystemRequests::CreateWorldCustom,
            AZ_CRC("EditorWorld", 0x8d93f191), editorWorldConfiguration);

        PhysX::RegisterConfigStringLineEditHandler(); // Register custom unique string line edit control
    }

    void EditorSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        Physics::EditorWorldBus::Handler::BusDisconnect();
        m_editorWorld = nullptr;
    }

    AZStd::shared_ptr<Physics::World> EditorSystemComponent::GetEditorWorld()
    {
        return m_editorWorld;
    }

    void EditorSystemComponent::MarkEditorWorldDirty()
    {
        m_editorWorldDirty = true;
    }

    void EditorSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        m_intervalCountdown -= deltaTime;

        if (m_editorWorldDirty && m_editorWorld && m_intervalCountdown <= 0.0f)
        {
            m_editorWorld->Update(s_fixedDeltaTime);
            m_intervalCountdown = s_minEditorWorldUpdateInterval;
            m_editorWorldDirty = false;

            Physics::SystemNotificationBus::Broadcast(&Physics::SystemNotifications::OnPostPhysicsUpdate, s_fixedDeltaTime, m_editorWorld.get());
        }
    }
}

