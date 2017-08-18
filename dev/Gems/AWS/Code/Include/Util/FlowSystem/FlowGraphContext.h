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

#include <Util/Async/AsyncContextWithExtra.h>
#include <IFlowSystem.h>

namespace LmbrAWS
{
    class FlowGraphContext
        : public Aws::Client::AsyncCallerContext
    {
    public:

        FlowGraphContext(IFlowGraphPtr flowGraph, TFlowNodeId flowNodeId);

        TFlowNodeId GetFlowNodeId() const { return m_flowNodeId; }

        IFlowGraphPtr GetFlowGraph() const;
        IFlowNode* GetFlowNode() const;

    private:

        IFlowGraphPtr   m_flowGraph;
        TFlowNodeId     m_flowNodeId;
    };

    template <typename EXTRA_TYPE>
    class FlowGraphContextWithExtra
        : public FlowGraphContext
        , public WithExtra < EXTRA_TYPE >
    {
    public:

        FlowGraphContextWithExtra(EXTRA_TYPE extra, IFlowGraphPtr flowGraph, TFlowNodeId flowNodeId)
            : FlowGraphContext(flowGraph, flowNodeId)
            , WithExtra < EXTRA_TYPE >(extra)
        {}
    };
} //namescape LmbrAWS