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

#include "../../CryEngine/CryAction/FlowSystem/FlowData.h"

#include "Include/EditorCoreAPI.h"
#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraph.h"
#include "HyperGraph/FlowGraphNode.h"
#include "../../../Editor/IEditor.h"
#include "../../../Editor/IconManager.h"

#include <LmbrCentral/Components/FlowGraph/FlowGraphSerialization.h>

#include <QtCore/QRectF>

namespace LmbrCentral
{
    //=========================================================================
    // BuildSerializedFlowGraph
    //=========================================================================
    void BuildSerializedFlowGraph(IFlowGraphPtr flowGraph, SerializedFlowGraph& graphData)
    {
        graphData = SerializedFlowGraph();

        if (!flowGraph)
        {
            return;
        }

        IFlowGraph* graph = flowGraph.get();
        CFlowGraph* hyperGraph = static_cast<CFlowGraph*>(GetIEditor()->GetFlowGraphManager()->FindGraph(graph));
        if (!hyperGraph)
        {
            return;
        }

        graphData.m_name = hyperGraph->GetName().toUtf8().constData();
        graphData.m_description = hyperGraph->GetDescription().toUtf8().constData();
        graphData.m_group = hyperGraph->GetGroupName().toUtf8().constData();
        graphData.m_isEnabled = hyperGraph->IsEnabled();

        switch (hyperGraph->GetMPType())
        {
        case CFlowGraph::eMPT_ServerOnly:
        {
            graphData.m_networkType = FlowGraphNetworkType::ServerOnly;
        }
        break;
        case CFlowGraph::eMPT_ClientOnly:
        {
            graphData.m_networkType = FlowGraphNetworkType::ClientOnly;
        }
        break;
        case CFlowGraph::eMPT_ClientServer:
        {
            graphData.m_networkType = FlowGraphNetworkType::ServerOnly;
        }
        break;
        }

        IHyperGraphEnumerator* nodeIter = hyperGraph->GetNodesEnumerator();
        for (IHyperNode* hyperNodeInterface = nodeIter->GetFirst(); hyperNodeInterface; hyperNodeInterface = nodeIter->GetNext())
        {
            CHyperNode* hyperNode = static_cast<CHyperNode*>(hyperNodeInterface);

            graphData.m_nodes.push_back();
            SerializedFlowGraph::Node& nodeData = graphData.m_nodes.back();

            nodeData.m_name = hyperNode->GetName().toUtf8().constData();
            nodeData.m_class = hyperNode->GetClassName().toUtf8().constData();
            nodeData.m_position.Set(hyperNode->GetPos().x(), hyperNode->GetPos().y());
            nodeData.m_flags = hyperNode->GetFlags();

            const QRectF& sizeRect(hyperNode->GetRect());
            nodeData.m_size.Set(sizeRect.right() - sizeRect.left(), sizeRect.bottom() - sizeRect.top());

            const QRectF* borderRect = hyperNode->GetResizeBorderRect();
            if (borderRect)
            {
                nodeData.m_borderRect.Set(borderRect->right() - borderRect->left(), borderRect->bottom() - borderRect->top());
            }

            const HyperNodeID nodeId = hyperNode->GetId();
            const HyperNodeID flowNodeId = hyperNode->GetFlowNodeId();
            CFlowData* flowData = flowNodeId != InvalidFlowNodeId ? static_cast<CFlowData*>(graph->GetNodeData(flowNodeId)) : nullptr;

            nodeData.m_id = nodeId;
            nodeData.m_isGraphEntity = hyperNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY);
            nodeData.m_entityId = flowData ? AZ::EntityId(flowData->GetEntityId().GetId()) : AZ::EntityId();
            if (static_cast<AZ::u64>(nodeData.m_entityId) == 0)
            {
                nodeData.m_entityId.SetInvalid();
            }

            const CHyperNode::Ports* inputPorts = hyperNode->GetInputs();
            if (inputPorts)
            {
                for (size_t inputIndex = 0, inputCount = inputPorts->size(); inputIndex < inputCount; ++inputIndex)
                {
                    const CHyperNodePort& port = (*inputPorts)[inputIndex];

                    if (!port.bVisible)
                    {
                        nodeData.m_inputHideMask |= (1 << inputIndex);
                    }

                    if (port.pVar)
                    {
                        const IVariable::EType type = port.pVar->GetType();
                        switch (type)
                        {
                        case IVariable::UNKNOWN:
                        case IVariable::FLOW_CUSTOM_DATA:
                        {
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Unknown,
                                aznew SerializedFlowGraph::InputValueVoid());
                        }
                        break;
                        case IVariable::INT:
                        {
                            auto* value = aznew SerializedFlowGraph::InputValueInt();
                            port.pVar->Get(value->m_value);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Int, value);
                        }
                        break;
                        case IVariable::BOOL:
                        {
                            auto* value = aznew SerializedFlowGraph::InputValueBool();
                            port.pVar->Get(value->m_value);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Bool, value);
                        }
                        break;
                        case IVariable::FLOAT:
                        {
                            auto* value = aznew SerializedFlowGraph::InputValueFloat();
                            port.pVar->Get(value->m_value);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Float, value);
                        }
                        break;
                        case IVariable::VECTOR2:
                        {
                            Vec2 temp;
                            port.pVar->Get(temp);
                            auto* value = aznew SerializedFlowGraph::InputValueVec2(temp);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Vector2, value);
                        }
                        break;
                        case IVariable::VECTOR:
                        {
                            Vec3 temp;
                            port.pVar->Get(temp);
                            auto* value = aznew SerializedFlowGraph::InputValueVec3(temp);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Vector3, value);
                        }
                        break;
                        case IVariable::VECTOR4:
                        {
                            Vec4 temp;
                            port.pVar->Get(temp);
                            auto* value = aznew SerializedFlowGraph::InputValueVec4(temp);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Vector4, value);
                        }
                        break;
                        case IVariable::QUAT:
                        {
                            Quat temp;
                            port.pVar->Get(temp);
                            auto* value = aznew SerializedFlowGraph::InputValueQuat(temp);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Quat, value);
                        }
                        break;
                        case IVariable::STRING:
                        {
                            auto* value = aznew SerializedFlowGraph::InputValueString();
                            CString temp;
                            port.pVar->Get(temp);
                            value->m_value = temp.GetString();
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::String, value);
                        }
                        break;
                        case IVariable::DOUBLE:
                        {
                            auto* value = aznew SerializedFlowGraph::InputValueDouble();
                            port.pVar->Get(value->m_value);
                            nodeData.m_inputs.emplace_back(port.GetName(), port.GetHumanName(), FlowVariableType::Double, value);
                        }
                        break;
                        }
                    }
                }
            }

            const CHyperNode::Ports* outputPorts = hyperNode->GetOutputs();
            if (outputPorts)
            {
                for (size_t outputIndex = 0, outputCount = outputPorts->size(); outputIndex < outputCount; ++outputIndex)
                {
                    const CHyperNodePort& port = (*outputPorts)[outputIndex];

                    if (!port.bVisible)
                    {
                        nodeData.m_outputHideMask |= (1 << outputIndex);
                    }
                }
            }
        }

        std::vector<CHyperEdge*> edges;
        edges.reserve(4096);
        hyperGraph->GetAllEdges(edges);

        for (CHyperEdge* edge : edges)
        {
            graphData.m_edges.push_back();
            SerializedFlowGraph::Edge& edgeData = graphData.m_edges.back();

            edgeData.m_nodeIn = edge->nodeIn;
            edgeData.m_nodeOut = edge->nodeOut;
            edgeData.m_portIn = edge->portIn.toUtf8().constData();
            edgeData.m_portOut = edge->portOut.toUtf8().constData();
            edgeData.m_isEnabled = edge->enabled;
        }

        edges.resize(0);

        for (size_t tokenIndex = 0, tokenCount = graph->GetGraphTokenCount(); tokenIndex < tokenCount; ++tokenIndex)
        {
            graphData.m_graphTokens.push_back();
            SerializedFlowGraph::GraphToken& tokenData = graphData.m_graphTokens.back();

            const IFlowGraph::SGraphToken* token = graph->GetGraphToken(tokenIndex);
            AZ_Assert(token, "Failed to retrieve graph token at index %zu", tokenIndex);
            tokenData.m_name = token->name.c_str();
            tokenData.m_type = token->type;
        }
    }
} // namespace LmbrCentral
