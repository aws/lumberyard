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

#include <AzCore/EBus/EBus.h>

struct IFlowGraph;
class IEntityObjectListener;
class CHyperGraph;

namespace LmbrCentral
{
    class SerializedFlowGraph;
}

namespace AzToolsFramework
{
    /**
     * Bus for querying about sandbox data associated with a given Entity.
     */
    class HyperGraphRequests
        : public AZ::EBusTraits
    {
    public:

        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        virtual ~HyperGraphRequests() {}

        virtual AZStd::string GetHyperGraphName(IFlowGraph* runtimeGraphPtr) = 0;

        virtual void RegisterHyperGraphEntityListener(IFlowGraph* runtimeGraphPtr, IEntityObjectListener* listener) = 0;
        virtual void UnregisterHyperGraphEntityListener(IFlowGraph* runtimeGraphPtr, IEntityObjectListener* listener) = 0;

        virtual void SetHyperGraphEntity(IFlowGraph* runtimeGraphPtr, const AZ::EntityId& id) = 0;
        virtual void SetHyperGraphGroupName(IFlowGraph* runtimeGraphPtr, const char* name) = 0;
        virtual void SetHyperGraphName(IFlowGraph* runtimeGraphPtr, const char* name) = 0;

        virtual void OpenHyperGraphView(IFlowGraph* runtimeGraphPtr) = 0;

        virtual void ReleaseHyperGraph(IFlowGraph* runtimeGraphPtr) = 0;

        virtual void BuildSerializedFlowGraph(IFlowGraph* runtimeGraphPtr, LmbrCentral::SerializedFlowGraph& graphData) = 0;
    };

    using HyperGraphRequestBus = AZ::EBus<HyperGraphRequests>;
} // namespace AzToolsFramework

