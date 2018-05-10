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
#include "precompiled.h"

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Nodes/Variable/VariableNodeBus.h>

#include <ScriptCanvas/Core/GraphBus.h>

#include <Editor/GraphCanvas/Components/VariableNodeDescriptorComponent.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Core/PureData.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////////////////////
    // VariableNodeDescriptorStringDataInterface
    //////////////////////////////////////////////

    VariableNodeDescriptorComponent::VariableNodeDescriptorStringDataInterface::VariableNodeDescriptorStringDataInterface(const AZ::EntityId& busId)
        : m_busId(busId)
    {
        VariableNodeDescriptorNotificationBus::Handler::BusConnect(m_busId);
    }

    VariableNodeDescriptorComponent::VariableNodeDescriptorStringDataInterface::~VariableNodeDescriptorStringDataInterface()
    {
    }

    AZStd::string VariableNodeDescriptorComponent::VariableNodeDescriptorStringDataInterface::GetString() const
    {
        AZStd::string variableName;
        GraphCanvas::VariableRequestBus::EventResult(variableName, m_busId, &GraphCanvas::VariableRequests::GetVariableName);

        return variableName;
    }

    void VariableNodeDescriptorComponent::VariableNodeDescriptorStringDataInterface::SetString(const AZStd::string& newValue)
    {
        GraphCanvas::VariableRequestBus::Event(m_busId, &GraphCanvas::VariableRequests::SetVariableName, newValue);
    }

    void VariableNodeDescriptorComponent::VariableNodeDescriptorStringDataInterface::OnNameChanged()
    {
        SignalValueChanged();
    }

    ////////////////////////////////////
    // VariableNodeDescriptorComponent
    ////////////////////////////////////

    void VariableNodeDescriptorComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<VariableNodeDescriptorComponent, NodeDescriptorComponent>()
                ->Version(1)
                ->Field("DisplayName", &VariableNodeDescriptorComponent::m_displayName)
                ;
        }
    }

    VariableNodeDescriptorComponent::VariableNodeDescriptorComponent()
        : NodeDescriptorComponent(NodeDescriptorType::VariableNode)
    {
    }

    void VariableNodeDescriptorComponent::Activate()
    {
        VariableNodeDescriptorRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::NodeNotificationBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::VariableRequestBus::Handler::BusConnect(GetEntityId());
    }

    void VariableNodeDescriptorComponent::Deactivate()
    {
        GraphCanvas::SceneVariableRequestBus::Handler::BusDisconnect();
        GraphCanvas::VariableRequestBus::Handler::BusDisconnect();
        GraphCanvas::NodeNotificationBus::Handler::BusDisconnect();
        VariableNodeDescriptorRequestBus::Handler::BusDisconnect();
    }

    AZ::EntityId VariableNodeDescriptorComponent::GetVariableId() const
    {
        return GetEntityId();
    }

    AZStd::string VariableNodeDescriptorComponent::GetVariableName() const
    {
        return m_displayName;
    }

    void VariableNodeDescriptorComponent::SetVariableName(const AZStd::string& value)
    {
        if (value.empty())
        {
            return;
        }

        AZStd::string proposedVariableName = value;
        if (proposedVariableName.at(0) != '#')
        {
            proposedVariableName.insert(0, "#");
        }

        AZ::EntityId sceneId;
        GraphCanvas::SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);

        AZ::EntityId currentVariableId = GetEntityId();

        bool isValidName = true;

        // This isn't used here, but on 2013 there is a weird issue trying to resolve GetVariablename
        GraphCanvas::SceneVariableRequestBus::EnumerateHandlersId(sceneId, [&isValidName, &proposedVariableName, &currentVariableId](GraphCanvas::SceneVariableRequests* sceneVariable)
        {
            AZ::EntityId variableId = sceneVariable->GetVariableId();

            if (variableId != currentVariableId)
            {
                AZStd::string variableName;
                GraphCanvas::VariableRequestBus::EventResult(variableName, variableId, &GraphCanvas::VariableRequestBus::Events::GetVariableName);

                isValidName = (variableName.compare(proposedVariableName) != 0);
            }

            return isValidName;
        });

        if (isValidName)
        {
            bool postUndoPoint = m_displayName != proposedVariableName && !m_displayName.empty();

            m_displayName = proposedVariableName;

            if (postUndoPoint)
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, sceneId);
            }

            GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, m_displayName);
            GraphCanvas::VariableNotificationBus::Event(GetEntityId(), &GraphCanvas::VariableNotifications::OnNameChanged);
        }
        else
        {
            AZ_Error("Variable Node", "Variable Name %s is already in use.\n", proposedVariableName.c_str());
        }
    }

    AZ::Uuid VariableNodeDescriptorComponent::GetVariableDataType() const
    {
        ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::Type::Invalid();
        ScriptCanvas::NodeRequestBus::EventResult(dataType, m_variableSourceEndpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlotDataType, m_variableSourceEndpoint.GetSlotId());

        if (dataType.IsValid())
        {
            return ScriptCanvas::Data::ToAZType(dataType);
        }
        else
        {
            return AZ::Uuid();
        }
    }


    GraphCanvas::StringDataInterface* VariableNodeDescriptorComponent::CreateNameInterface()
    {
        return aznew VariableNodeDescriptorStringDataInterface(GetEntityId());
    }

    ScriptCanvas::Endpoint VariableNodeDescriptorComponent::GetReadEndpoint() const
    {
        return m_variableSourceEndpoint;
    }

    ScriptCanvas::Endpoint VariableNodeDescriptorComponent::GetWriteEndpoint() const
    {
        return m_variableWriteEndpoint;
    }

    void VariableNodeDescriptorComponent::AddConnection(const GraphCanvas::Endpoint& endpoint, const AZ::EntityId& scConnectionId)
    {
        m_slotIdToConnectionId[endpoint.GetSlotId()] = scConnectionId;
    }

    void VariableNodeDescriptorComponent::RemoveConnection(const GraphCanvas::Endpoint& endpoint)
    {
        m_slotIdToConnectionId.erase(endpoint.GetSlotId());
    }

    AZ::EntityId VariableNodeDescriptorComponent::FindConnection(const GraphCanvas::Endpoint& endpoint)
    {
        return m_slotIdToConnectionId[endpoint.GetSlotId()];
    }

    void VariableNodeDescriptorComponent::OnAddedToScene(const AZ::EntityId& sceneId)
    {
        GraphCanvas::SceneVariableRequestBus::Handler::BusConnect(sceneId);

        AZStd::any* userData = nullptr;
        GraphCanvas::NodeRequestBus::EventResult(userData, GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId variableNodeId = (userData && userData->is<AZ::EntityId>()) ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();

        ScriptCanvas::SlotId variableSlotId;
        ScriptCanvas::NodeRequestBus::EventResult(variableSlotId, variableNodeId, &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_getThis);

        m_variableSourceEndpoint = ScriptCanvas::Endpoint(variableNodeId, variableSlotId);

        ScriptCanvas::SlotId variableWriteSlotId;
        ScriptCanvas::NodeRequestBus::EventResult(variableWriteSlotId, variableNodeId, &ScriptCanvas::NodeRequests::GetSlotId, ScriptCanvas::PureData::k_setThis);

        m_variableWriteEndpoint = ScriptCanvas::Endpoint(variableNodeId, variableWriteSlotId);

        // If we don't have a name. Generate one.
        while (m_displayName.empty())
        {
            AZ::u32 counter = 0;
            VariableNodeSceneRequestBus::EventResult(counter, sceneId, &VariableNodeSceneRequests::GetNewVariableCounter);

            SetVariableName(AZStd::string::format("Variable %i", counter));

            if (!m_displayName.empty())
            {
                GraphCanvas::SceneVariableNotificationBus::Event(sceneId, &GraphCanvas::SceneVariableNotifications::OnVariableCreated, GetEntityId());
            }
        }

        VariableNodeDescriptorNotificationBus::Event(GetEntityId(), &VariableNodeDescriptorNotifications::OnNameChanged);
        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetTitle, m_displayName);

        ActivateVariable();
    }

    void VariableNodeDescriptorComponent::OnRemovedFromScene(const AZ::EntityId& sceneId)
    {
        GraphCanvas::SceneVariableRequestBus::Handler::BusDisconnect(sceneId);
        GraphCanvas::VariableNotificationBus::Event(GetEntityId(), &GraphCanvas::VariableNotifications::OnVariableDestroyed);
        GraphCanvas::SceneVariableNotificationBus::Event(sceneId, &GraphCanvas::SceneVariableNotifications::OnVariableDestroyed, GetEntityId());
    }

    void VariableNodeDescriptorComponent::OnNodeDeserialized(const GraphCanvas::SceneSerialization&)
    {
        // When we are deserialized(from a paste)
        // We want to clear our display name to generate a new variable name.
        m_displayName.clear();
    }

    void VariableNodeDescriptorComponent::ActivateVariable()
    {
        GraphCanvas::VariableRequestBus::Handler::BusConnect(GetEntityId());
        GraphCanvas::VariableNotificationBus::Event(GetEntityId(), &GraphCanvas::VariableNotifications::OnVariableActivated);

        UpdateTitleDisplay();
    }

    void VariableNodeDescriptorComponent::UpdateTitleDisplay()
    {
        AZ::Uuid dataType;
        GraphCanvas::VariableRequestBus::EventResult(dataType, GetEntityId(), &GraphCanvas::VariableRequests::GetVariableDataType);

        GraphCanvas::NodeTitleRequestBus::Event(GetEntityId(), &GraphCanvas::NodeTitleRequests::SetDataPaletteOverride, dataType);
    }
}