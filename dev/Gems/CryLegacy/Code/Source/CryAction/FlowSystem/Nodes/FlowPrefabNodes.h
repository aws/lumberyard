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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Prefab flownode functionality


#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWPREFABNODES_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWPREFABNODES_H
#pragma once

#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <ICustomEvents.h>

//////////////////////////////////////////////////////////////////////////
// Prefab:EventSource node
// This node is placed inside a prefab flowgraph to create / handle a custom event
//////////////////////////////////////////////////////////////////////////
class CFlowNode_PrefabEventSource
    : public CFlowBaseNode<eNCT_Instanced>
    , public ICustomEventListener
{
public:
    enum INPUTS
    {
        EIP_PrefabName = 0,
        EIP_InstanceName,
        EIP_EventName,
        EIP_FireEvent,
        EIP_EventId,
        EIP_EventIndex,
    };

    enum OUTPUTS
    {
        EOP_EventFired = 0,
    };

    CFlowNode_PrefabEventSource(SActivationInfo* pActInfo)
        : m_eventId(CUSTOMEVENTID_INVALID)
        , m_customEventListenerRegistered(false)
    {
    }

    ~CFlowNode_PrefabEventSource();

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_PrefabEventSource(pActInfo); }

    virtual void GetConfiguration(SFlowNodeConfig& config);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);

    // ICustomEventListener
    virtual void OnCustomEvent(const TCustomEventId eventId);
    // ~ICustomEventListener

private:
    SActivationInfo m_actInfo;
    TCustomEventId m_eventId;
    bool m_customEventListenerRegistered;
};

//////////////////////////////////////////////////////////////////////////
// Prefab:Instance node
// This node represents a prefab instance (Currently used to handle events specific to a prefab instance)
//////////////////////////////////////////////////////////////////////////
class CFlowNode_PrefabInstance
    : public CFlowBaseNode<eNCT_Instanced>
    , public ICustomEventListener
{
public:
    enum INPUTS
    {
        EIP_PrefabName = 0,
        EIP_InstanceName,
        EIP_Event1,
        EIP_Event1Id,
        EIP_Event1Name,
        EIP_Event2,
        EIP_Event2Id,
        EIP_Event2Name,
        EIP_Event3,
        EIP_Event3Id,
        EIP_Event3Name,
    };

    enum OUTPUTS
    {
        EOP_Event1 = 0,
        EOP_Event2,
        EOP_Event3,
    };

    CFlowNode_PrefabInstance(SActivationInfo* pActInfo);
    ~CFlowNode_PrefabInstance();

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    virtual IFlowNodePtr Clone(SActivationInfo* pActInfo) { return new CFlowNode_PrefabInstance(pActInfo); }

    virtual void GetConfiguration(SFlowNodeConfig& config);
    virtual void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo);

    // ICustomEventListener
    virtual void OnCustomEvent(const TCustomEventId eventId);
    // ~ICustomEventListener

private:
    CryFixedArray<TCustomEventId, CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE> m_eventIds;
    SActivationInfo m_actInfo;
    bool m_customEventListenerRegistered;
};

#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_NODES_FLOWPREFABNODES_H
