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

#include <Components/Connections/DataConnections/DataConnectionComponent.h>

#include <Components/Connections/DataConnections/DataConnectionVisualComponent.h>
#include <Components/StylingComponent.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>

namespace GraphCanvas
{
    ////////////////////////////
    // DataConnectionComponent
    ////////////////////////////

    void DataConnectionComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DataConnectionComponent, ConnectionComponent>()
                ->Version(1)
            ;
        }
    }

    AZ::Entity* DataConnectionComponent::CreateDataConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection, const AZStd::string& substyle)
    {
        // Create this Connection's entity.
        AZ::Entity* entity = aznew AZ::Entity("Connection");

        entity->CreateComponent<DataConnectionComponent>(sourceEndpoint, targetEndpoint, createModelConnection);
        entity->CreateComponent<StylingComponent>(Styling::Elements::Connection, AZ::EntityId(), substyle);
        entity->CreateComponent<DataConnectionVisualComponent>();

        entity->Init();
        entity->Activate();

        return entity;
    }
    
    DataConnectionComponent::DataConnectionComponent(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint, bool createModelConnection)
        : ConnectionComponent(sourceEndpoint, targetEndpoint, createModelConnection)
    {
    }

    void DataConnectionComponent::Activate()
    {
        ConnectionComponent::Activate();
        
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void DataConnectionComponent::Deactivate()
    {
        SceneMemberNotificationBus::Handler::BusDisconnect();

        ConnectionComponent::Deactivate();
    }
    
    bool DataConnectionComponent::OnConnectionMoveComplete(const QPointF& scenePos, const QPoint& screenPos)
    {
        bool retVal = false;
        // If we are missing an endpoint, default to the normal behavior
        if (!m_sourceEndpoint.IsValid() || !m_targetEndpoint.IsValid())
        {
            retVal = ConnectionComponent::OnConnectionMoveComplete(scenePos, screenPos);
        }
        else
        {
            DataSlotType sourceSlotType = DataSlotType::Unknown;
            DataSlotRequestBus::EventResult(sourceSlotType, GetSourceSlotId(), &DataSlotRequests::GetDataSlotType);

            bool converted = false;

            if (sourceSlotType == DataSlotType::Variable)
            {
                // Temporary dirtiness.
                // When dragging from a input to an output, we put the connection into the output
                // to make the display look right.
                //
                // But converting a DataSlot to a Reference, clears off all connections(including us).
                // To fix this, just remove ourselves from the target before converting to a reference.
                if (m_dragContext == DragContext::MoveSource)
                {
                    SlotNotificationBus::Event(GetTargetSlotId(), &SlotNotifications::OnDisconnectedFrom, GetEntityId(), GetSourceEndpoint());
                }
                
                DataSlotRequestBus::EventResult(converted, GetTargetSlotId(), &DataSlotRequests::ConvertToReference);
            }
            else if (sourceSlotType == DataSlotType::Value)
            {
                DataSlotRequestBus::EventResult(converted, GetTargetSlotId(), &DataSlotRequests::ConvertToValue);
            }

            if (converted)
            {
                DataSlotType targetSlotType = DataSlotType::Unknown;
                DataSlotRequestBus::EventResult(targetSlotType, GetTargetSlotId(), &DataSlotRequests::GetDataSlotType);

                if (targetSlotType == DataSlotType::Reference)
                {
                    AZ::EntityId variableId;
                    DataSlotRequestBus::EventResult(variableId, GetSourceSlotId(), &DataSlotRequests::GetVariableId);

                    DataSlotRequestBus::Event(GetTargetSlotId(), &DataSlotRequests::AssignVariable, variableId);
                }
                else if (targetSlotType == DataSlotType::Value)
                {
                    retVal = ConnectionComponent::OnConnectionMoveComplete(scenePos, screenPos);
                }
            }
        }
        
        return retVal;
    }
}