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
#include <precompiled.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <ScriptCanvas/Components/EditorUtils.h>

#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/GraphCanvas/MappingBus.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Utils/NodeUtils.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteModel.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>

namespace ScriptCanvasEditor
{
    //////////////////////////
    // NodeIdentifierFactory
    //////////////////////////

    ScriptCanvas::NodeTypeIdentifier NodeIdentifierFactory::ConstructNodeIdentifier(const GraphCanvas::GraphCanvasTreeItem* treeItem)
    {
        ScriptCanvas::NodeTypeIdentifier resultHash = 0;
        if (auto getVariableMethodTreeItem = azrtti_cast<const GetVariableNodePaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructGetVariableNodeIdentifier(getVariableMethodTreeItem->GetVariableId());
        }
        else if (auto setVariableMethodTreeItem = azrtti_cast<const SetVariableNodePaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructSetVariableNodeIdentifier(setVariableMethodTreeItem->GetVariableId());
        }
        else if (auto classMethodTreeItem = azrtti_cast<const ClassMethodEventPaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructMethodNodeIdentifier(classMethodTreeItem->GetClassMethodName(), classMethodTreeItem->GetMethodName());
        }
        else if (auto customNodeTreeItem = azrtti_cast<const CustomNodePaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructCustomNodeIdentifier(customNodeTreeItem->GetTypeId());
        }
        else if (auto sendEbusEventTreeItem = azrtti_cast<const EBusSendEventPaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructEBusEventSenderIdentifier(sendEbusEventTreeItem->GetBusId(), sendEbusEventTreeItem->GetEventId());
        }
        else if (auto handleEbusEventTreeItem = azrtti_cast<const EBusHandleEventPaletteTreeItem*>(treeItem))
        {
            resultHash = ScriptCanvas::NodeUtils::ConstructEBusEventReceiverIdentifier(handleEbusEventTreeItem->GetBusId(), handleEbusEventTreeItem->GetEventId());
        }

        return resultHash;
    }

    //////////////////////////
    // GraphStatisticsHelper
    //////////////////////////

    void GraphStatisticsHelper::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<GraphStatisticsHelper>()
                ->Version(1)
                ->Field("InstanceCounter", &GraphStatisticsHelper::m_nodeIdentifierCount)
            ;
        }
    }

    void GraphStatisticsHelper::PopulateStatisticData(const Graph* editorGraph)
    {
        // Opportunistically use this time to refresh out node count array.
        m_nodeIdentifierCount.clear();

        for (auto* nodeEntity : editorGraph->GetNodeEntities())
        {
            auto nodeComponent = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity);

            if (nodeComponent)
            {
                if (auto ebusHandlerNode  = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(nodeComponent))
                {
                    GraphCanvas::NodeId graphCanvasNodeId;
                    SceneMemberMappingRequestBus::EventResult(graphCanvasNodeId, nodeEntity->GetId(), &SceneMemberMappingRequests::GetGraphCanvasEntityId);                    

                    const ScriptCanvas::Nodes::Core::EBusEventHandler::EventMap& events = ebusHandlerNode->GetEvents();

                    ScriptCanvas::EBusBusId busId = ebusHandlerNode->GetEBusId();

                    for (const auto& eventPair : events)
                    {
                        bool hasEvent = false;
                        EBusHandlerNodeDescriptorRequestBus::EventResult(hasEvent, graphCanvasNodeId, &EBusHandlerNodeDescriptorRequests::ContainsEvent, eventPair.second.m_eventId);

                        // In case we are populating from an uncreated scene if we don't have a valid graph canvas node id
                        // just accept everything. We can overreport on unknown data for now.
                        if (hasEvent || !graphCanvasNodeId.IsValid())
                        {
                            RegisterNodeType(ScriptCanvas::NodeUtils::ConstructEBusEventReceiverIdentifier(busId, eventPair.second.m_eventId));
                        }
                    }
                }
                else
                {
                    ScriptCanvas::NodeTypeIdentifier nodeType = nodeComponent->GetNodeType();

                    // Fallback in case something isn't initialized for whatever reason
                    if (nodeType == 0)
                    {
                        nodeType = ScriptCanvas::NodeUtils::ConstructNodeType(nodeComponent);
                    }

                    RegisterNodeType(nodeType);
                }
            }
        }
    }

    void GraphStatisticsHelper::RegisterNodeType(const ScriptCanvas::NodeTypeIdentifier& nodeTypeIdentifier)
    {
        auto nodeIter = m_nodeIdentifierCount.find(nodeTypeIdentifier);

        if (nodeIter == m_nodeIdentifierCount.end())
        {
            m_nodeIdentifierCount.emplace(AZStd::make_pair(nodeTypeIdentifier, 1));
        }
        else
        {
            nodeIter->second++;
        }
    }
}