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

#ifndef CRYINCLUDE_CRYCOMMON_FLOWGRAPHINFORMATION_EBUS_H
#define CRYINCLUDE_CRYCOMMON_FLOWGRAPHINFORMATION_EBUS_H
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>

#include <IFlowSystem.h>

class CEntityObject;
class CFlowGraph;
struct IFlowGraph;

class FlowGraphNotificationEventGroup
    : public AZ::EBusTraits
{
public:

    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

    virtual void ComponentFlowGraphAdded(IFlowGraph* graph, const AZ::EntityId& id, const char* flowGraphName) = 0;
    virtual void ComponentFlowGraphLoaded(IFlowGraph* graph, const AZ::EntityId& id, const AZStd::string& flowGraphName, const AZStd::string& flowGraphXML, bool isTemporaryGraph) = 0;
    virtual void ComponentFlowGraphRemoved(IFlowGraph* graph) = 0;
};

typedef AZ::EBus<FlowGraphNotificationEventGroup> FlowGraphNotificationBus;

/*
* This editor-side bus is used for FlowGraph integration with the UI.
*/
class FlowGraphEditorRequests
    : public AZ::EBusTraits
{
public:

    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
    typedef FlowEntityId BusIdType;

    virtual void OpenFlowGraphView(IFlowGraph* flowGraph) = 0;
    virtual IFlowGraph* AddFlowGraph(const AZStd::string& flowGraphName) = 0;
    virtual void RemoveFlowGraph(IFlowGraph* flowGraph, bool shouldPromptUser) = 0;
    virtual void GetFlowGraphs(AZStd::vector<AZStd::pair<AZStd::string, IFlowGraph*> >& flowGraphs) = 0;
};

typedef AZ::EBus<FlowGraphEditorRequests> FlowGraphEditorRequestsBus;


class ComponentFlowgraphRuntimeNotifications
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    typedef TFlowGraphId BusIdType;

    /*
    * Indicates that a node in a given flowgraph was activated
    */
    virtual void OnFlowNodeActivated(IFlowGraphPtr pFlowGraph, TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value) = 0;
};

typedef AZ::EBus<ComponentFlowgraphRuntimeNotifications> ComponentFlowgraphRuntimeNotificationBus;

class ComponentHyperGraphRequests
    : public AZ::EBusTraits
{
public:
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    typedef AZ::Uuid BusIdType;

    /*
    * Fetches the flowgraph that is used by the Component entity while in runtime
    */
    virtual IFlowGraph* GetComponentRuntimeFlowgraph() = 0;

    /*
    * Fetches the flowgraph that is used by the Component entities while in editor
    */
    virtual IFlowGraph* GetLegacyRuntimeFlowgraph() = 0;

    /*
    * Fetches the raw HyperGraph pointer
    */
    virtual CFlowGraph* GetHyperGraph() = 0;

    /*
    * Binds and Unbinds this HyperGraph to a Runtime Component Flowgraph
    */
    virtual void BindToFlowgraph(IFlowGraphPtr pFlowGraph) = 0;
    virtual void UnBindFromFlowgraph() = 0;
};

typedef AZ::EBus<ComponentHyperGraphRequests> ComponentHyperGraphRequestsBus;

#endif