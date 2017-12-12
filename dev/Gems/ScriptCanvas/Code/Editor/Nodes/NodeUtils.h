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
#include <AzCore/Component/EntityId.h>

#include <ScriptCanvas/Bus/NodeIdPair.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace ScriptCanvas
{
    class Slot;
}

namespace ScriptCanvasEditor
{
    namespace Nodes
    {
        NodeIdPair CreateNode(const AZ::Uuid& classData, AZ::EntityId graphId, const AZStd::string& styleOverride);
        NodeIdPair CreateEntityNode(const AZ::EntityId& sourceId, AZ::EntityId graphId);
        
        NodeIdPair CreateEbusEventNode(const AZStd::string& busName, const AZStd::string& eventName, AZ::EntityId graphId);
        NodeIdPair CreateEbusWrapperNode(const AZStd::string& busName, const AZ::EntityId& graphId);

        NodeIdPair CreateGetVariableNode(const AZ::EntityId& variableId);
        NodeIdPair CreateSetVariableNode(const AZ::EntityId& variableId, const AZ::EntityId& graphId);

        NodeIdPair CreateVariablePrimitiveNode(const AZ::Uuid& primitiveId, AZ::EntityId graphId);
        NodeIdPair CreateVariableObjectNode(const AZStd::string& className, AZ::EntityId graphId);

        NodeIdPair CreateObjectMethodNode(const AZStd::string& className, const AZStd::string& methodName, AZ::EntityId graphId);
        NodeIdPair CreateObjectOrObjectMethodNode(const AZStd::string& className, const AZStd::string& methodName, AZ::EntityId graphId);

        // SlotGroup will control how elements are groupped.
        // Invalid will cause the slots to put themselves into whatever category they belong to by default.
        AZ::EntityId CreateGraphCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::Slot& slot, GraphCanvas::SlotGroup group = GraphCanvas::SlotGroups::Invalid);
        AZ::EntityId CreateForcedDataTypeGraphCanvasSlot(const AZ::EntityId& graphCanvasNodeId, const ScriptCanvas::Slot& slot, const AZ::Uuid& dataType, GraphCanvas::SlotGroup slotGroup = GraphCanvas::SlotGroups::Invalid);
        AZ::EntityId CreateGraphCanvasPropertySlot(const AZ::EntityId& graphCanvasNodeId, const AZ::Crc32& propertyId, const GraphCanvas::SlotConfiguration& slotConfiguration);
        AZ::EntityId CreateGraphCanvasVariableSourceSlot(const AZ::EntityId& graphCanavasNodeId, const AZ::Uuid& dataType, const GraphCanvas::SlotConfiguration& slotConfiguration);
    }
}