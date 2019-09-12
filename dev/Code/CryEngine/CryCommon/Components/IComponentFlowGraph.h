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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTFLOWGRAPH_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTFLOWGRAPH_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>

struct IFlowGraph;

//! Interface for the entity's flow graph component.
//! This component allows the entity to host a reference to a flow graph.
struct IComponentFlowGraph
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentFlowGraph", 0x1306D25045784348, 0xB4F9E98E7C96CD83)

    IComponentFlowGraph() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::FlowGraph; }

    virtual void SetFlowGraph(IFlowGraph* pFlowGraph) = 0;
    virtual IFlowGraph* GetFlowGraph() = 0;

    virtual void AddEventListener(IEntityEventListener* pListener) = 0;
    virtual void RemoveEventListener(IEntityEventListener* pListener) = 0;
    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentFlowGraph);

#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTFLOWGRAPH_H