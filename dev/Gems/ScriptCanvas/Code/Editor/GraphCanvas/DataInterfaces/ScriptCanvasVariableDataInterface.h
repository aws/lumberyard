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

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/NodePropertyDisplay/VariableDataInterface.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Slots/Data/DataSlotBus.h>

#include "Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h"

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasVariableDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::VariableReferenceDataInterface>
        , public GraphCanvas::DataSlotNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasVariableDataInterface, AZ::SystemAllocator, 0);
        
        ScriptCanvasVariableDataInterface(const AZ::Uuid& dataType, const AZ::EntityId& graphCanvasNodeId, const AZ::EntityId& graphCanvasSlotId, const AZ::EntityId& scriptCanvasNodeId, const ScriptCanvas::SlotId& scriptCanvasSlotId)
            : ScriptCanvasDataInterface(scriptCanvasNodeId, scriptCanvasSlotId)
            , m_dataType(dataType)
            , m_graphCanvasNodeId(graphCanvasNodeId)
            , m_graphCanvasSlotId(graphCanvasSlotId)
        {
            GraphCanvas::DataSlotNotificationBus::Handler::BusConnect(m_graphCanvasSlotId);
        }
        
        ~ScriptCanvasVariableDataInterface()
        {
            GraphCanvas::DataSlotNotificationBus::Handler::BusDisconnect();
        }

        // GraphCanvas::VariableReferenceDataInterface
        AZ::Uuid GetVariableDataType() const override
        {
            return m_dataType;
        }
        
        AZ::EntityId GetVariableReference() const override
        {
            AZ::EntityId variableId;
            GraphCanvas::DataSlotRequestBus::EventResult(variableId, m_graphCanvasSlotId, &GraphCanvas::DataSlotRequests::GetVariableId);
            return variableId;
        }

        void AssignVariableReference(const AZ::EntityId& variableId) override
        {
            GraphCanvas::DataSlotRequestBus::Event(m_graphCanvasSlotId, &GraphCanvas::DataSlotRequests::AssignVariable, variableId);
            
            PostUndoPoint();
        }
        ////

        // GraphCanvas::DataSlotNotificationBus
        void OnVariableAssigned(const AZ::EntityId& variableAssigned)
        {
            SignalValueChanged();
        }
        ////
        
    private:

        AZ::Uuid     m_dataType;
        AZ::EntityId m_graphCanvasNodeId;
        AZ::EntityId m_graphCanvasSlotId;
    };
}