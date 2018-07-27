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


#include "CryLegacy_precompiled.h"

#include "FlowPrefabNodes.h"

#include "CustomEvents/CustomEventsManager.h"

#define PREFAB_EVENTS_INSTANCE_NUMINPUTPORTSPEREVENT 3

//////////////////////////////////////////////////////////////////////////
// Prefab:EventSource node
// This node is placed inside a prefab flowgraph to create / handle a custom event
//////////////////////////////////////////////////////////////////////////
CFlowNode_PrefabEventSource::~CFlowNode_PrefabEventSource()
{
    if (m_eventId != CUSTOMEVENTID_INVALID && m_customEventListenerRegistered)
    {
        ICustomEventManager* pCustomEventManager = gEnv->pGame->GetIGameFramework()->GetICustomEventManager();
        CRY_ASSERT(pCustomEventManager != NULL);
        pCustomEventManager->UnregisterEventListener(this, m_eventId);
    }
}

void CFlowNode_PrefabEventSource::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_ports[] =
    {
        InputPortConfig<string> ("PrefabName", _HELP("Prefab name")),
        InputPortConfig<string> ("InstanceName", _HELP("Prefab instance name")),
        InputPortConfig<string> ("EventName", _HELP("Name of event associated to this prefab")),
        InputPortConfig_Void        ("FireEvent", _HELP("Fires associated event")),
        InputPortConfig<int>        ("EventId", 0, _HELP("Event id storage")),
        InputPortConfig<int>        ("EventIndex", -1, _HELP("Event id storage")),
        {0}
    };

    static const SOutputPortConfig out_ports[] =
    {
        OutputPortConfig_Void   ("EventFired", _HELP("Triggered when associated event is fired")),
        {0}
    };

    config.pInputPorts = in_ports;
    config.pOutputPorts = out_ports;
    config.sDescription = _HELP("Event node that must be placed inside a prefab for it to be handled in an instance");
    config.SetCategory(EFLN_APPROVED);
}

void CFlowNode_PrefabEventSource::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    ICustomEventManager* pCustomEventManager = gEnv->pGame->GetIGameFramework()->GetICustomEventManager();
    CRY_ASSERT(pCustomEventManager != NULL);

    if (event == eFE_Activate)
    {
        if (IsPortActive(pActInfo, EIP_FireEvent))
        {
            if (m_eventId != CUSTOMEVENTID_INVALID)
            {
                pCustomEventManager->FireEvent(m_eventId);
            }

            // Output port is activated from event
        }
    }
    else if (event == eFE_Initialize)
    {
        m_actInfo = *pActInfo;

        const TCustomEventId eventId = (TCustomEventId)GetPortInt(pActInfo, EIP_EventId);
        if (eventId != CUSTOMEVENTID_INVALID)
        {
            m_customEventListenerRegistered = pCustomEventManager->RegisterEventListener(this, eventId);
        }
        m_eventId = eventId;
    }
}

void CFlowNode_PrefabEventSource::OnCustomEvent(const TCustomEventId eventId)
{
    if (m_eventId == eventId)
    {
        ActivateOutput(&m_actInfo, EOP_EventFired, true);
    }
}

//////////////////////////////////////////////////////////////////////////
// Prefab:Instance node
// This node represents a prefab instance (Currently used to handle events specific to a prefab instance)
//////////////////////////////////////////////////////////////////////////
CFlowNode_PrefabInstance::CFlowNode_PrefabInstance(SActivationInfo* pActInfo)
{
    m_customEventListenerRegistered = false;
    for (int i = 0; i < CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
    {
        m_eventIds.push_back(CUSTOMEVENTID_INVALID);
    }
}

CFlowNode_PrefabInstance::~CFlowNode_PrefabInstance()
{
    ICustomEventManager* pCustomEventManager = gEnv->pGame->GetIGameFramework()->GetICustomEventManager();
    CRY_ASSERT(pCustomEventManager != NULL);

    for (int i = 0; i < CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
    {
        TCustomEventId& eventId = m_eventIds[i];
        if (eventId != CUSTOMEVENTID_INVALID && m_customEventListenerRegistered)
        {
            pCustomEventManager->UnregisterEventListener(this, eventId);
            eventId = CUSTOMEVENTID_INVALID;
        }
    }
}

void CFlowNode_PrefabInstance::GetConfiguration(SFlowNodeConfig& config)
{
    static const SInputPortConfig in_ports[] =
    {
        InputPortConfig<string> ("PrefabName", _HELP("Prefab name")),
        InputPortConfig<string> ("InstanceName", _HELP("Prefab instance name")),
        InputPortConfig_Void        ("Event1", _HELP("Event trigger port")),
        InputPortConfig<int>        ("Event1Id", _HELP("Event id storage")),
        InputPortConfig<string> ("Event1Name", _HELP("Event name storage")),
        InputPortConfig_Void        ("Event2", _HELP("Event trigger port")),
        InputPortConfig<int>        ("Event2Id", _HELP("Event id storage")),
        InputPortConfig<string> ("Event2Name", _HELP("Event name storage")),
        InputPortConfig_Void        ("Event3", _HELP("Event trigger port")),
        InputPortConfig<int>        ("Event3Id", _HELP("Event id storage")),
        InputPortConfig<string> ("Event3Name", _HELP("Event name storage")),
        InputPortConfig_Void        ("Event4", _HELP("Event trigger port")),
        InputPortConfig<int>        ("Event4Id", _HELP("Event id storage")),
        InputPortConfig<string> ("Event4Name", _HELP("Event name storage")),
        InputPortConfig_Void        ("Event5", _HELP("Event trigger port")),
        InputPortConfig<int>        ("Event5Id", _HELP("Event id storage")),
        InputPortConfig<string> ("Event5Name", _HELP("Event name storage")),
        InputPortConfig_Void        ("Event6", _HELP("Event trigger port")),
        InputPortConfig<int>        ("Event6Id", _HELP("Event id storage")),
        InputPortConfig<string> ("Event6Name", _HELP("Event name storage")),
        {0}
    };

    static const SOutputPortConfig out_ports[] =
    {
        OutputPortConfig_Void   ("Event1", _HELP("Triggered when associated event is fired")),
        OutputPortConfig_Void   ("Event2", _HELP("Triggered when associated event is fired")),
        OutputPortConfig_Void   ("Event3", _HELP("Triggered when associated event is fired")),
        OutputPortConfig_Void   ("Event4", _HELP("Triggered when associated event is fired")),
        OutputPortConfig_Void   ("Event5", _HELP("Triggered when associated event is fired")),
        OutputPortConfig_Void   ("Event6", _HELP("Triggered when associated event is fired")),
        {0}
    };

    config.pInputPorts = in_ports;
    config.pOutputPorts = out_ports;
    config.sDescription = _HELP("Prefab instance node to handle events specific to an instance");
    config.SetCategory(EFLN_APPROVED | EFLN_HIDE_UI);
}

void CFlowNode_PrefabInstance::ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
{
    ICustomEventManager* pCustomEventManager = gEnv->pGame->GetIGameFramework()->GetICustomEventManager();
    CRY_ASSERT(pCustomEventManager != NULL);

    if (event == eFE_Activate)
    {
        int iCurInputPort = EIP_Event1;
        for (int i = 0; i < CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
        {
            if (IsPortActive(pActInfo, iCurInputPort))
            {
                const TCustomEventId eventId = m_eventIds[i];
                if (m_eventIds[i] != CUSTOMEVENTID_INVALID)
                {
                    pCustomEventManager->FireEvent(eventId);
                }

                // Output port is activated from event
            }

            iCurInputPort += PREFAB_EVENTS_INSTANCE_NUMINPUTPORTSPEREVENT;
        }
    }
    else if (event == eFE_Initialize)
    {
        m_actInfo = *pActInfo;

        int iCurPort = EIP_Event1Id;
        for (int i = 0; i < CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
        {
            const TCustomEventId eventId = (TCustomEventId)GetPortInt(pActInfo, iCurPort);
            if (eventId != CUSTOMEVENTID_INVALID)
            {
                m_eventIds[i] = eventId;
                m_customEventListenerRegistered = pCustomEventManager->RegisterEventListener(this, eventId);
            }

            iCurPort += PREFAB_EVENTS_INSTANCE_NUMINPUTPORTSPEREVENT;
        }
    }
}

void CFlowNode_PrefabInstance::OnCustomEvent(const TCustomEventId eventId)
{
    // Need to determine which output port corresponds to event id
    int iOutputPort = EOP_Event1;
    for (int i = 0; i < CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
    {
        const TCustomEventId curEventId = m_eventIds[i];
        if (curEventId == eventId)
        {
            ActivateOutput(&m_actInfo, iOutputPort, true);
            break;
        }

        iOutputPort++;
    }
}

REGISTER_FLOW_NODE("Prefab:EventSource", CFlowNode_PrefabEventSource)
REGISTER_FLOW_NODE("Prefab:Instance", CFlowNode_PrefabInstance)
