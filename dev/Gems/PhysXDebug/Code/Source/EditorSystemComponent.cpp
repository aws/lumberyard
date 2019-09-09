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

#include "PhysXDebug_precompiled.h"
#include "EditorSystemComponent.h"
#include <PhysX/SystemComponentBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <IEditor.h>

namespace PhysXDebug
{
    void EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    // This will be called when the IEditor instance is ready
    void EditorSystemComponent::NotifyRegisterViews()
    {
        RegisterForEditorEvents();
    }

    void EditorSystemComponent::OnCrySystemShutdown(ISystem&)
    {
        UnregisterForEditorEvents();
    }

    void EditorSystemComponent::OnEditorNotifyEvent(const EEditorNotifyEvent editorEvent)
    {
        switch (editorEvent)
        {
        case eNotify_OnEndNewScene:
        case eNotify_OnEndLoad:
            AutoConnectPVD();
            break;
        default:
            // Intentionally left blank.
            break;
        }
    }

    void EditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::Components::EditorComponentBase::Activate();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        PhysX::ConfigurationNotificationBus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        PhysX::ConfigurationNotificationBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void EditorSystemComponent::RegisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->RegisterNotifyListener(this);
        }
    }

    void EditorSystemComponent::UnregisterForEditorEvents()
    {
        IEditor* editor = nullptr;
        AzToolsFramework::EditorRequests::Bus::BroadcastResult(editor, &AzToolsFramework::EditorRequests::GetEditor);
        if (editor)
        {
            editor->UnregisterNotifyListener(this);
        }
    }

    void EditorSystemComponent::OnConfigurationRefreshed(const PhysX::Configuration& configuration)
    {
        if(configuration.m_settings.IsAutoConnectionEditorMode())
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::ConnectToPvd);
        }
        else
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::DisconnectFromPvd);
        }
    }

    void EditorSystemComponent::OnStartPlayInEditorBegin()
    {
        PhysX::Configuration configuration;
        PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);
        if(configuration.m_settings.IsAutoConnectionGameMode())
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::ConnectToPvd);
        }
    }
    void EditorSystemComponent::OnStopPlayInEditor()
    {
        PhysX::Configuration configuration;
        PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);

        if(configuration.m_settings.IsAutoConnectionGameMode())
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::DisconnectFromPvd);
        }

        if(configuration.m_settings.IsAutoConnectionEditorMode() && configuration.m_settings.m_pvdReconnect)
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::ConnectToPvd);
        }
    }

    void EditorSystemComponent::AutoConnectPVD()
    {
        PhysX::Configuration configuration;
        PhysX::ConfigurationRequestBus::BroadcastResult(configuration, &PhysX::ConfigurationRequests::GetConfiguration);

        if(configuration.m_settings.IsAutoConnectionEditorMode())
        {
            PhysX::SystemRequestsBus::Broadcast(&PhysX::SystemRequests::ConnectToPvd);
        }
    }
}