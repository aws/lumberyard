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
#include "Util/FlowSystem/FlowGraphContext.h"

namespace LmbrAWS
{
    FlowGraphContext::FlowGraphContext(IFlowGraphPtr flowGraph, TFlowNodeId flowNodeId)
        : m_flowGraph(flowGraph)
        , m_flowNodeId(flowNodeId)
    {
    }

    IFlowGraphPtr FlowGraphContext::GetFlowGraph() const
    {
        return m_flowGraph;
    }

    IFlowNode* FlowGraphContext::GetFlowNode() const
    {
        IFlowGraph* flowGraph = GetFlowGraph();
        IFlowNodeData* flowNodeData = (flowGraph != nullptr) ? flowGraph->GetNodeData(m_flowNodeId) : nullptr;
        IFlowNode* flowNode = (flowNodeData != nullptr) ? flowNodeData->GetNode() : nullptr;
        return flowNode;
    }
}