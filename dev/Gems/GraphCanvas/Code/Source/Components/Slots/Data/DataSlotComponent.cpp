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

#include <Components/Slots/Data/DataSlotComponent.h>

#include <Components/Connections/ConnectionComponent.h>
#include <Components/Connections/DataConnections/DataConnectionComponent.h>
#include <Components/Slots/Data/DataSlotConnectionPin.h>
#include <Components/Slots/Data/DataSlotLayoutComponent.h>
#include <Components/Slots/SlotConnectionFilterComponent.h>
#include <Components/StylingComponent.h>

#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilters.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/DataConnectionFilters.h>
#include <GraphCanvas/Components/StyleBus.h>


namespace GraphCanvas
{
    //////////////////////
    // DataSlotComponent
    //////////////////////

    void DataSlotComponent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<DataSlotComponent, SlotComponent>()
                ->Version(3)
                ->Field("TypeId", &DataSlotComponent::m_dataTypeId)
                ->Field("DataSlotType", &DataSlotComponent::m_dataSlotType)
                ->Field("VariableId", &DataSlotComponent::m_variableId)
                ->Field("FixedType", &DataSlotComponent::m_fixedType)
                ->Field("CopiedVariableId", &DataSlotComponent::m_copiedVariableId)
                ->Field("ContainedTypeIds", &DataSlotComponent::m_containedTypeIds)
            ;
        }
    }

    AZ::Entity* DataSlotComponent::CreateDataSlot(const AZ::EntityId& nodeId, const DataSlotConfiguration& dataSlotConfiguration)
    {
        AZ::Entity* entity = SlotComponent::CreateCoreSlotEntity();

        DataSlotComponent* dataSlot = aznew DataSlotComponent(dataSlotConfiguration);

        if (!entity->AddComponent(dataSlot))
        {
            delete dataSlot;
            delete entity;
            return nullptr;
        }

        entity->CreateComponent<DataSlotLayoutComponent>();
        entity->CreateComponent<StylingComponent>(Styling::Elements::DataSlot, nodeId, "");

        SlotConnectionFilterComponent* connectionFilter = entity->CreateComponent<SlotConnectionFilterComponent>();

        SlotTypeFilter* slotTypeFilter = aznew SlotTypeFilter(ConnectionFilterType::Include);
        slotTypeFilter->AddSlotType(SlotTypes::DataSlot);

        connectionFilter->AddFilter(slotTypeFilter);

        ConnectionTypeFilter* connectionTypeFilter = aznew ConnectionTypeFilter(ConnectionFilterType::Include);

        switch (dataSlot->GetConnectionType())
        {
            case ConnectionType::CT_Input:
                connectionTypeFilter->AddConnectionType(CT_Output);
                break;
            case ConnectionType::CT_Output:
                connectionTypeFilter->AddConnectionType(CT_Input);
                break;
            default:
                break;
        };

        connectionFilter->AddFilter(connectionTypeFilter);

        connectionFilter->AddFilter(aznew DataSlotTypeFilter());

        return entity;
    }
    
    DataSlotComponent::DataSlotComponent()
        : SlotComponent(SlotTypes::DataSlot)
        , m_fixedType(false)
        , m_dataSlotType(DataSlotType::Value)
        , m_dataTypeId(AZ::Uuid::CreateNull())
        , m_previousDataSlotType(DataSlotType::Unknown)
    {
        if (m_slotConfiguration.m_slotGroup == SlotGroups::Invalid)
        {
            m_slotConfiguration.m_slotGroup = SlotGroups::DataGroup;
        }
    }

    DataSlotComponent::DataSlotComponent(const DataSlotConfiguration& dataSlotConfiguration)
        : SlotComponent(SlotTypes::DataSlot, dataSlotConfiguration)
        , m_fixedType(true)
        , m_dataSlotType(dataSlotConfiguration.m_dataSlotType)
        , m_dataTypeId(dataSlotConfiguration.m_typeId)
        , m_containedTypeIds(dataSlotConfiguration.m_containerTypeIds)
        , m_previousDataSlotType(DataSlotType::Unknown)
    {
        if (m_slotConfiguration.m_slotGroup == SlotGroups::Invalid)
        {
            if (m_dataSlotType == DataSlotType::Variable)
            {
                m_slotConfiguration.m_slotGroup = SlotGroups::VariableSourceGroup;
            }
            else if (m_dataSlotType == DataSlotType::Reference)
            {
                m_slotConfiguration.m_slotGroup == SlotGroups::VariableReferenceGroup;
            }
            else
            {
                m_slotConfiguration.m_slotGroup = SlotGroups::DataGroup;
            }
        }

        if (m_dataSlotType == DataSlotType::Variable && m_slotConfiguration.m_connectionType != ConnectionType::CT_Output)
        {
            m_slotConfiguration.m_connectionType = ConnectionType::CT_Output;
        }
    }

    DataSlotComponent::~DataSlotComponent()
    {
    }
    
    void DataSlotComponent::Init()
    {
        SlotComponent::Init();
    }
    
    void DataSlotComponent::Activate()
    {
        SlotComponent::Activate();
        
        DataSlotRequestBus::Handler::BusConnect(GetEntityId());

        if (m_variableId.IsValid() && m_dataSlotType == DataSlotType::Reference)
        {
        }
    }
    
    void DataSlotComponent::Deactivate()
    {
        SlotComponent::Deactivate();
        
        DataSlotRequestBus::Handler::BusDisconnect();

        if (m_variableId.IsValid() && m_dataSlotType == DataSlotType::Reference)
        {
        }
    }

    void DataSlotComponent::OnSceneMemberAboutToSerialize(GraphSerialization&)
    {
        m_copiedVariableId = static_cast<AZ::u64>(m_variableId);
    }

    void DataSlotComponent::DisplayProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        const bool updateDisplay = false;
        RestoreDisplay(updateDisplay);

        DataSlotType slotType = DataSlotType::Unknown;
        DataSlotRequestBus::EventResult(slotType, endpoint.GetSlotId(), &DataSlotRequests::GetDataSlotType);

        if (slotType == DataSlotType::Variable)
        {
            m_previousDataSlotType = m_dataSlotType;
            m_cachedVariableId = m_variableId;

            DataSlotRequestBus::EventResult(m_variableId, endpoint.GetSlotId(), &DataSlotRequests::GetVariableId);
            m_dataSlotType = DataSlotType::Reference;

            if (m_previousDataSlotType != m_dataSlotType)
            {
                NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, false);
                DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
                SetConnectionDisplayState(RootGraphicsItemDisplayState::Deletion);
            }
        }
        else if (slotType == DataSlotType::Value)
        {
            m_displayedConnection = connectionId;

            m_connections.emplace_back(m_displayedConnection);

            m_previousDataSlotType = m_dataSlotType;
            m_cachedVariableId = m_variableId;

            m_variableId.SetInvalid();
            m_dataSlotType = DataSlotType::Value;

            if (m_previousDataSlotType != m_dataSlotType)
            {
                DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
            }

            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
        }
        else if (slotType == DataSlotType::Container)
        {
            m_displayedConnection = connectionId;

            m_connections.emplace_back(m_displayedConnection);

            m_previousDataSlotType = m_dataSlotType;
            m_cachedVariableId = m_variableId;

            m_variableId.SetInvalid();
            m_dataSlotType = DataSlotType::Container;

            if (m_previousDataSlotType != m_dataSlotType)
            {
                DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
            }

            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
        }

        UpdateDisplay();
    }

    void DataSlotComponent::RemoveProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        RestoreDisplay(true);
    }

    void DataSlotComponent::AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        SlotComponent::AddConnectionId(connectionId, endpoint);

        if (m_dataSlotType == DataSlotType::Value)
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, true);
        }
        else if (m_dataSlotType == DataSlotType::Reference)
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, false);
        }
        else
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
        }
    }

    void DataSlotComponent::RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint)
    {
        SlotComponent::RemoveConnectionId(connectionId, endpoint);

        NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
    }

    void DataSlotComponent::SetNode(const AZ::EntityId& nodeId)
    {
        SlotComponent::SetNode(nodeId);
    }

    SlotConfiguration* DataSlotComponent::CloneSlotConfiguration() const
    {
        DataSlotConfiguration* slotConfiguration = aznew DataSlotConfiguration();

        slotConfiguration->m_dataSlotType = GetDataSlotType();

        slotConfiguration->m_typeId = GetDataTypeId();

        slotConfiguration->m_containerTypeIds = m_containedTypeIds;

        PopulateSlotConfiguration((*slotConfiguration));

        return slotConfiguration;
    }

    bool DataSlotComponent::AssignVariable(const AZ::EntityId& variableId)
    {
        RestoreDisplay();

        if (m_dataSlotType == DataSlotType::Value)
        {
            if (!ConvertToReference())
            {
                return false;
            }
        }

        if (m_variableId != variableId)
        {
            Endpoint endpoint(GetNode(), GetEntityId());

            if (m_dataSlotType != DataSlotType::Variable)
            {
            }
            
            m_variableId = variableId;

            // If we are the owner of the variable. We don't want to signal things out.
            if (m_dataSlotType != DataSlotType::Variable)
            {
                DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnVariableAssigned, m_variableId);

                if (m_variableId.IsValid())
                {
                }
            }

            UpdateDisplay();
        }

        return true;
    }

    void DataSlotComponent::UpdateDisplay()
    {
        DataSlotLayoutRequestBus::Event(GetEntityId(), &DataSlotLayoutRequests::UpdateDisplay);
    }

    void DataSlotComponent::RestoreDisplay(bool updateDisplay)
    {
        if (m_previousDataSlotType != DataSlotType::Unknown)
        {
            bool typeChanged = m_dataSlotType != m_previousDataSlotType;

            m_dataSlotType = m_previousDataSlotType;
            m_variableId = m_cachedVariableId;

            if (m_displayedConnection.IsValid())
            {
                for (int i = static_cast<int>(m_connections.size()) - 1; i >= 0; ++i)
                {
                    if (m_connections[i] == m_displayedConnection)
                    {
                        m_connections.erase(m_connections.begin() + i);
                        break;
                    }
                }
            }

            if (updateDisplay)
            {
                UpdatePropertyDisplayState();

                if (typeChanged)
                {
                    DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
                }

                UpdateDisplay();
            }

            m_previousDataSlotType = DataSlotType::Unknown;
            m_cachedVariableId.SetInvalid();
            m_displayedConnection.SetInvalid();
        }
    }

    void DataSlotComponent::OnFinalizeDisplay()
    {
        UpdatePropertyDisplayState();
    }

    void DataSlotComponent::UpdatePropertyDisplayState()
    {
        if (m_dataSlotType == DataSlotType::Reference)
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, false);
        }
        else if (m_dataSlotType == DataSlotType::Value)
        {
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());
        }
    }

    AZ::EntityId DataSlotComponent::GetVariableId() const
    {
        return m_variableId;
    }

    bool DataSlotComponent::ConvertToReference()
    {
        if (CanConvertToReference())
        {
            ClearConnections();
            m_dataSlotType = DataSlotType::Reference;

            DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
        }

        return m_dataSlotType == DataSlotType::Reference;
    }

    bool DataSlotComponent::CanConvertToReference() const
    {
        return !m_fixedType && m_dataSlotType == DataSlotType::Value;
    }

    bool DataSlotComponent::ConvertToValue()
    {
        if (CanConvertToValue())
        {
            m_dataSlotType = DataSlotType::Value;

            DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDataSlotTypeChanged, m_dataSlotType);
            NodePropertyRequestBus::Event(GetEntityId(), &NodePropertyRequests::SetDisabled, HasConnections());

            if (m_variableId.IsValid())
            {
                m_variableId.SetInvalid();
            }
        }

        return m_dataSlotType == DataSlotType::Value;
    }

    bool DataSlotComponent::CanConvertToValue() const
    {
        return !m_fixedType && m_dataSlotType == DataSlotType::Reference;
    }

    DataSlotType DataSlotComponent::GetDataSlotType() const
    {
        return m_dataSlotType;
    }

    AZ::Uuid DataSlotComponent::GetDataTypeId() const
    {
        return m_dataTypeId;
    }

    void DataSlotComponent::SetDataTypeId(AZ::Uuid typeId)
    {
        if (m_dataTypeId != typeId)
        {
            m_dataTypeId = typeId;
            UpdateDisplay();
        }
    }

    const Styling::StyleHelper* DataSlotComponent::GetDataColorPalette() const
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, sceneId, &SceneRequests::GetEditorId);

        const Styling::StyleHelper* stylingHelper = nullptr;
        StyleManagerRequestBus::EventResult(stylingHelper, editorId, &StyleManagerRequests::FindDataColorPalette, m_dataTypeId);

        return stylingHelper;
    }

    size_t DataSlotComponent::GetContainedTypesCount() const
    {
        return m_containedTypeIds.size();
    }

    AZ::Uuid DataSlotComponent::GetContainedTypeId(size_t index) const
    {
        return m_containedTypeIds[index];
    }

    const GraphCanvas::Styling::StyleHelper* DataSlotComponent::GetContainedTypeColorPalette(size_t index) const
    {
        AZ::Uuid dataTypeId = m_containedTypeIds[index];

        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        EditorId editorId;
        SceneRequestBus::EventResult(editorId, sceneId, &SceneRequests::GetEditorId);

        const Styling::StyleHelper* stylingHelper = nullptr;
        StyleManagerRequestBus::EventResult(stylingHelper, editorId, &StyleManagerRequests::FindDataColorPalette, dataTypeId);

        return stylingHelper;
    }

    void DataSlotComponent::SetDataAndContainedTypeIds(AZ::Uuid typeId, const AZStd::vector<AZ::Uuid>& typeIds)
    {
        if (m_dataTypeId != typeId)
        {
            m_dataTypeId = typeId;
            m_containedTypeIds = typeIds;

            DataSlotNotificationBus::Event(GetEntityId(), &DataSlotNotifications::OnDisplayTypeChanged, m_dataTypeId, m_containedTypeIds);
            
            UpdatePropertyDisplayState();
        }
    }

    AZ::Entity* DataSlotComponent::ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection) const
    {
        const AZStd::string k_connectionSubStyle = ".varFlow";

        // Create this Connection's entity.
        return DataConnectionComponent::CreateDataConnection(sourceEndpoint, targetEndpoint, createModelConnection, k_connectionSubStyle);
    }
}