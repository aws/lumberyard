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

// Description : Manages prefab events


#include "StdAfx.h"
#include "PrefabEvents.h"

#include "PrefabItem.h"
#include "Objects/PrefabObject.h"
#include "HyperGraph/FlowGraphManager.h"
#include "HyperGraph/FlowGraphNode.h"
#include "HyperGraph/FlowGraphVariables.h"
#include <IGameFramework.h>
#include "Objects/EntityObject.h"

static TFlowNodeId s_prefabInstanceNodeId = InvalidFlowNodeTypeId;
static TFlowNodeId s_prefabEventSourceNodeId = InvalidFlowNodeTypeId;

//////////////////////////////////////////////////////////////////////////
// CPrefabEvents implementation.
//////////////////////////////////////////////////////////////////////////
CPrefabEvents::CPrefabEvents()
    : m_bCurrentlySettingPrefab(false)
{
}

//////////////////////////////////////////////////////////////////////////
CPrefabEvents::~CPrefabEvents()
{
    Deinit();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode)
{
    if (pNode)
    {
        switch (event)
        {
        case EHG_NODE_ADD:
        {
            CFlowNode* pNodeCasted = (CFlowNode*)pNode;
            CRY_ASSERT(pNodeCasted != NULL);
            const TFlowNodeId typeId = pNodeCasted->GetTypeId();

            if (typeId == s_prefabEventSourceNodeId)
            {
                UpdatePrefabEventSourceNode(event, pNodeCasted);
            }
            else if (typeId == s_prefabInstanceNodeId)
            {
                UpdatePrefabInstanceNode(event, pNodeCasted);
            }
        }
        break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::OnHyperGraphEvent(IHyperNode* pINode, EHyperGraphEvent event)
{
    if (event == EHG_NODE_PROPERTIES_CHANGED && pINode)   // Only node change are handled here, add and remove are from OnHyperGraphManagerEvent
    {
        CFlowNode* pNodeCasted = (CFlowNode*)pINode;
        CRY_ASSERT(pNodeCasted != NULL);
        TFlowNodeId typeId = pNodeCasted->GetTypeId();

        if (pNodeCasted->GetTypeId() == s_prefabInstanceNodeId)
        {
            UpdatePrefabInstanceNode(event, pNodeCasted);
        }
        else if (pNodeCasted->GetTypeId() == s_prefabEventSourceNodeId)
        {
            UpdatePrefabEventSourceNode(event, pNodeCasted);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::Init()
{
    CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager();
    CRY_ASSERT(pFlowGraphManager != NULL);
    if (pFlowGraphManager)
    {
        pFlowGraphManager->AddListener(this);
        s_prefabInstanceNodeId = gEnv->pFlowSystem->GetTypeId("Prefab:Instance");
        s_prefabEventSourceNodeId = gEnv->pFlowSystem->GetTypeId("Prefab:EventSource");

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::Deinit()
{
    CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager();
    CRY_ASSERT(pFlowGraphManager != NULL);
    if (pFlowGraphManager)
    {
        pFlowGraphManager->RemoveListener(this);
    }

    s_prefabInstanceNodeId = InvalidFlowNodeTypeId;
    s_prefabEventSourceNodeId = InvalidFlowNodeTypeId;

    RemoveAllEventData();
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::AddPrefabInstanceNodeFromSelection(CFlowNode* pNode, CPrefabObject* pPrefabObj)
{
    CRY_ASSERT(pPrefabObj != NULL);
    CRY_ASSERT(pNode != NULL);
    CPrefabItem* pPrefabItem = pPrefabObj->GetPrefab();
    CRY_ASSERT(pPrefabItem != NULL);

    const QString& prefabName = pPrefabItem->GetFullName();
    const QString& prefabInstanceName = pPrefabObj->GetName();

    // Ensure doesn't already exist
    const QString* pPrefabName = NULL;
    const QString* pPrefabInstanceName = NULL;
    SPrefabEventInstanceEventsData* pInstanceEventsData = NULL;
    const bool bResult = GetEventInstanceNodeData(pNode, &pPrefabName, &pPrefabInstanceName, &pInstanceEventsData);
    CRY_ASSERT(!bResult);

    if (!bResult)
    {
        IHyperGraph* pGraph = pNode->GetGraph();
        CRY_ASSERT(pGraph != NULL);
        if (pGraph)
        {
            pGraph->AddListener(this);   // Need to register with graph for events to receive change events
        }

        TPrefabInstancesData& instancesData = m_prefabEventData[ prefabName ]; // Creates new entry if doesn't exist
        SPrefabEventInstanceEventsData& instanceEventsData = instancesData[ prefabInstanceName ]; // Creates new entry if doesn't exist

        SetPrefabIdentifiersToEventNode(pNode, prefabName, prefabInstanceName);
        UpdatePrefabEventsOnInstance(instanceEventsData, pNode);
        instanceEventsData.m_eventInstanceNodes.insert(pNode);

        return true;
    }
    else
    {
        Warning("CPrefabEvents::AddPrefabInstanceNode: Node already added for prefab: %s", prefabName.toUtf8().constData());
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::SetCurrentlySettingPrefab(const bool bCurrentlySetting)
{
    m_bCurrentlySettingPrefab = bCurrentlySetting;
    if (!bCurrentlySetting)   // Need to check if event source nodes were attempted to be added during cloning (Only after cloning is prefab object + instance name set)
    {
        if (m_delayedAddingEventSourceNodes.size() > 0)
        {
            TPrefabEventNodeContainer::iterator nodeContainerIter = m_delayedAddingEventSourceNodes.begin();
            const TPrefabEventNodeContainer::const_iterator nodeContainerIterEnd = m_delayedAddingEventSourceNodes.end();
            while (nodeContainerIter != nodeContainerIterEnd)
            {
                CFlowNode* pNode = *nodeContainerIter;
                CRY_ASSERT(pNode != NULL);

                const bool bResult = AddPrefabEventSourceNode(pNode, SPrefabPortVisibility());
                if (!bResult)
                {
                    Warning("CPrefabEvents::SetCurrentlyCloningPrefab: Failed adding cloned event source node");
                }

                ++nodeContainerIter;
            }

            m_delayedAddingEventSourceNodes.clear();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::RemoveAllEventData()
{
    m_prefabEventData.clear();
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::OnFlowNodeRemoval(CFlowNode* pNode)
{
    CRY_ASSERT(pNode != NULL);

    // Too late to determine by node type, just check if have an association to ptr and remove
    UpdatePrefabInstanceNode(EHG_NODE_DELETE, pNode);
    UpdatePrefabEventSourceNode(EHG_NODE_DELETE, pNode);
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::OnPrefabObjectNameChange(CPrefabObject* pPrefabObj, const QString& oldName, const QString& newName)
{
    CRY_ASSERT(pPrefabObj != NULL);

    CPrefabItem* pPrefabItem = pPrefabObj->GetPrefab();
    if (pPrefabItem)
    {
        const QString& prefabName = pPrefabItem->GetFullName();
        TPrefabNameToInstancesData::iterator allPrefabDataIter = m_prefabEventData.find(prefabName);
        if (allPrefabDataIter != m_prefabEventData.end())
        {
            TPrefabInstancesData& instancesData = allPrefabDataIter->second;
            TPrefabInstancesData::iterator instanceNameDataIter = instancesData.find(oldName);
            if (instanceNameDataIter != instancesData.end())
            {
                instancesData[ newName ] = instanceNameDataIter->second; // Copy into new name mapping
                instancesData.erase(instanceNameDataIter);   // Remove data in old mapping
                instanceNameDataIter = instancesData.find(newName);   // Reused iterator to point to data in new name mapping

                // Now go through all the nodes switching names
                SPrefabEventInstanceEventsData& instanceEventsData = instanceNameDataIter->second;

                // First event instance ones
                TPrefabEventNodeContainer& eventInstanceNodes = instanceEventsData.m_eventInstanceNodes;
                TPrefabEventNodeContainer::iterator eventInstanceNodesIter = eventInstanceNodes.begin();
                const TPrefabEventNodeContainer::iterator eventInstanceNodesIterEnd = eventInstanceNodes.end();
                while (eventInstanceNodesIter != eventInstanceNodesIterEnd)
                {
                    CFlowNode* pNode = *eventInstanceNodesIter;
                    CRY_ASSERT(pNode != NULL);

                    SetPrefabIdentifiersToEventNode(pNode, prefabName, newName);

                    ++eventInstanceNodesIter;
                }

                // Now go through all the event source nodes
                TPrefabEventSourceData& eventsSourceData = instanceEventsData.m_eventSourceData;
                TPrefabEventSourceData::iterator eventDataIter = eventsSourceData.begin();
                const TPrefabEventSourceData::const_iterator eventDataIterEnd = eventsSourceData.end();
                while (eventDataIter != eventDataIterEnd)
                {
                    SPrefabEventSourceData& eventData = eventDataIter->second;
                    TPrefabEventNodeContainer& eventSourceNodes = eventData.m_eventSourceNodes;
                    TPrefabEventNodeContainer::iterator eventSourceNodeIter = eventSourceNodes.begin();
                    const TPrefabEventNodeContainer::iterator eventSourceNodeIterEnd = eventSourceNodes.end();
                    while (eventSourceNodeIter != eventSourceNodeIterEnd)
                    {
                        CFlowNode* pNode = *eventSourceNodeIter;
                        CRY_ASSERT(pNode != NULL);

                        SetPrefabIdentifiersToEventNode(pNode, prefabName, newName);

                        ++eventSourceNodeIter;
                    }

                    ++eventDataIter;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::GetPrefabPortVisibility(CFlowNode* pNode, SPrefabPortVisibility& result)
{
    for (CHyperNode::Ports::iterator it = pNode->GetInputs()->begin(); it != pNode->GetInputs()->end(); ++it)
    {
        QString pName = it->GetName();
        if (QString::compare(pName, "PrefabName") == 0)
        {
            result.bPrefabNamePortVisible = it->bVisible;
        }
        else if (QString::compare(pName, "InstanceName") == 0)
        {
            result.bInstanceNamePortVisible = it->bVisible;
        }
        else if (QString::compare(pName, "EventId") == 0)
        {
            result.bEventIdPortVisible = it->bVisible;
        }
        else if (QString::compare(pName, "EventIndex") == 0)
        {
            result.bEventIndexPortVisible = it->bVisible;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::UpdatePrefabEventSourceNode(const EHyperGraphEvent event, CFlowNode* pNode)
{
    CRY_ASSERT(pNode != NULL);

    const QString* pPrefabName = NULL;
    const QString* pPrefabInstanceName = NULL;
    const QString* pEventName = NULL;
    TPrefabInstancesData* pInstancesData = NULL;
    SPrefabEventInstanceEventsData* pInstanceEventsData = NULL;
    SPrefabEventSourceData* pEventSourceData = NULL;
    bool bResult = GetEventSourceNodeData(pNode,
            &pPrefabName,
            &pPrefabInstanceName,
            &pEventName,
            &pInstancesData,
            &pInstanceEventsData,
            &pEventSourceData);

    switch (event)
    {
    case EHG_NODE_ADD:
    {
        if (!bResult)
        {
            IHyperGraph* pGraph = pNode->GetGraph();
            CRY_ASSERT(pGraph != NULL);
            if (pGraph)
            {
                pGraph->AddListener(this);       // Need to register with graph for events to receive change events
            }

            if (m_bCurrentlySettingPrefab)       // After cloning is completely and prefab object + instance name assigned, these can be processed
            {
                m_delayedAddingEventSourceNodes.insert(pNode);
                return;
            }

            bResult = AddPrefabEventSourceNode(pNode, SPrefabPortVisibility());
            CRY_ASSERT(bResult);
            if (!bResult)
            {
                Warning("CPrefabEvents::UpdatePrefabEventSourceNode: Failed to add prefab event source node");
            }
        }
        else
        {
            Warning("CPrefabEvents::UpdatePrefabEventSourceNode: can't add node, already added");
        }
    }
    break;
    case EHG_NODE_PROPERTIES_CHANGED:
    {
        if (bResult)
        {
            // Need to check what changed
            QString prefabName;
            QString prefabInstanceName;
            QString eventName;

            GetPrefabIdentifiersFromStoredData(pNode, prefabName, prefabInstanceName);
            GetPrefabEventNameFromEventSourceNode(pNode, eventName);

            if ((prefabName.compare(*pPrefabName) != 0) ||
                (prefabInstanceName.compare(*pPrefabInstanceName) != 0) ||
                (eventName.compare(*pEventName) != 0))
            {
                SPrefabPortVisibility portVisibility;
                GetPrefabPortVisibility(pNode, portVisibility);
                RemovePrefabEventSourceNode(*pPrefabName, *pInstancesData, *pInstanceEventsData, *pEventSourceData, *pEventName, pNode, true);
                bResult = AddPrefabEventSourceNode(pNode, portVisibility);
                CRY_ASSERT(bResult);
            }
        }
    }
    break;
    case EHG_NODE_DELETE:
    {
        if (bResult)
        {
            RemovePrefabEventSourceNode(*pPrefabName, *pInstancesData, *pInstanceEventsData, *pEventSourceData, *pEventName, pNode, false);
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::UpdatePrefabInstanceNode(const EHyperGraphEvent event, CFlowNode* pNode)
{
    CRY_ASSERT(pNode != NULL);

    const QString* pPrefabName = NULL;
    const QString* pPrefabInstanceName = NULL;
    SPrefabEventInstanceEventsData* pInstanceEventsData = NULL;
    bool bResult = GetEventInstanceNodeData(pNode, &pPrefabName, &pPrefabInstanceName, &pInstanceEventsData);

    switch (event)
    {
    // Add is handled externally from AddPrefabInstanceNode
    case EHG_NODE_ADD:
    {
        if (!bResult)       // Wasn't added from selection ( Serialized or cloned )
        {
            IHyperGraph* pGraph = pNode->GetGraph();
            CRY_ASSERT(pGraph != NULL);
            if (pGraph)
            {
                pGraph->AddListener(this);       // Need to register with graph for events to receive change events
            }

            bResult = AddPrefabInstanceNodeFromStoredData(pNode);
            CRY_ASSERT(bResult);
        }
    }
    break;

    case EHG_NODE_PROPERTIES_CHANGED:
    {
        // Remove and re-add from node data
        if (bResult)
        {
            RemovePrefabInstanceNode(pInstanceEventsData, pNode, true);
            AddPrefabInstanceNodeFromStoredData(pNode);
        }
    }
    break;
    case EHG_NODE_DELETE:
    {
        if (bResult)
        {
            RemovePrefabInstanceNode(pInstanceEventsData, pNode, false);
        }
    }
    break;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::AddPrefabInstanceNodeFromStoredData(CFlowNode* pNode)
{
    const QString* pPrefabName = NULL;
    const QString* pPrefabInstanceName = NULL;
    SPrefabEventInstanceEventsData* pInstanceEventsData = NULL;

    QString prefabName;
    QString prefabInstanceName;
    bool bResult = GetPrefabIdentifiersFromStoredData(pNode, prefabName, prefabInstanceName);
    CRY_ASSERT(bResult);
    if (bResult)
    {
        TPrefabInstancesData& instancesData = m_prefabEventData[ prefabName ]; // Creates new entry if doesn't exist
        SPrefabEventInstanceEventsData& instanceEventsData = instancesData[ prefabInstanceName ]; // Creates new entry if doesn't exist
        instanceEventsData.m_eventInstanceNodes.insert(pNode);

        SetPrefabIdentifiersToEventNode(pNode, prefabName, prefabInstanceName);   // Updates node ports readability

        // Flownode event eFE_Initialize causes custom events to be registered. Currently no way to only do that
        // only when gEnv->bEditorGame == true as when entering editor, bEditorGame = true assignment happens later than it should
        // For now clear events to be safe won't be leaving a dangling pointer
        // Needs to be inside here which is called when adding a node since initialize is called from flownode serialization
        UnregisterPrefabInstanceEvents(pNode);

        UpdatePrefabEventsOnInstance(instanceEventsData, pNode);
        // It's possible during serializing that not all source events have been added at this point and instance node isn't complete
        // That's ok since source node contains event index and updates the instances when they're added

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::AddPrefabEventSourceNode(CFlowNode* pNode, const SPrefabPortVisibility& portVisibility)
{
    CRY_ASSERT(pNode != NULL);

    // Need to check what changed
    QString prefabName;
    QString prefabInstanceName;
    QString eventName;

    bool bResult = GetPrefabIdentifiersFromObject(pNode, prefabName, prefabInstanceName);
    CRY_ASSERT(bResult);
    if (bResult)
    {
        SetPrefabIdentifiersToEventNode(pNode, prefabName, prefabInstanceName);
        SetPrefabEventNodePortVisibility(pNode, "PrefabName", portVisibility.bPrefabNamePortVisible, true);
        SetPrefabEventNodePortVisibility(pNode, "InstanceName", portVisibility.bInstanceNamePortVisible, true);
    }
    else
    {
        Warning("CPrefabEvents::AddPrefabEventSourceNode: Failed to find prefab data, node should only be created inside a prefab");
        return false;
    }

    GetPrefabEventNameFromEventSourceNode(pNode, eventName);
    TPrefabInstancesData& instancesData = m_prefabEventData[ prefabName ]; // Creates new entry if doesn't exist
    SPrefabEventInstanceEventsData& instanceEventsData = instancesData[ prefabInstanceName ]; // Creates new entry if doesn't exist
    SPrefabEventSourceData& eventData = instanceEventsData.m_eventSourceData[ eventName ]; // Creates new entry if doesn't exist

    eventData.m_eventSourceNodes.insert(pNode);   // Keep track of node regardless if has an empty event or not

    TCustomEventId& eventId = eventData.m_eventId;
    int iEventIndex = -1;
    // First attempt to get saved event index
    GetPrefabEventIndexFromEventSourceNode(pNode, iEventIndex);

    if (eventId == CUSTOMEVENTID_INVALID)   // Try to create event id
    {
        if (!eventName.isEmpty())   // Has an event name so assign an id
        {
            // Determine port position on instance
            if (iEventIndex == -1)   // Didn't exist so need to attempt to get a free one
            {
                SPrefabEventSourceData* pFoundSourceData = NULL;
                const QString* pFoundEventName = NULL;
                for (int i = 1; i <= CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
                {
                    bResult = GetPrefabEventSourceData(instanceEventsData.m_eventSourceData, i, &pFoundSourceData, &pFoundEventName);
                    if (!bResult)
                    {
                        iEventIndex = i;
                        break;
                    }
                }
            }

            if (iEventIndex != -1)
            {
                ICustomEventManager* pCustomEventManager = GetISystem()->GetIGame()->GetIGameFramework()->GetICustomEventManager();
                CRY_ASSERT(pCustomEventManager != NULL);
                eventData.m_eventId = pCustomEventManager->GetNextCustomEventId();
                eventData.m_iEventIndex = iEventIndex;

                // Need to update all instances
                UpdatePrefabInstanceNodeEvents(instancesData);
            }
            else // Unable to find a port position for instances, must have exceeded amount of unique events
            {
                Warning("CPrefabEvents::AddPrefabEventSourceNode: Unable to add unique event, at max limit of: %d", CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE);
            }
        }
    }

    // Flownode event eFE_Initialize causes custom events to be registered. Currently no way to only do that
    // only when gEnv->bEditorGame == true as when entering editor, bEditorGame = true assignment happens later than it should
    // For now clear events to be safe won't be leaving a dangling pointer
    // Needs to be inside here which is called when adding a node since initialize is called from flownode serialization
    UnregisterPrefabEventSourceEvent(pNode);
    SetPrefabEventIdToEventSourceNode(pNode, eventId);

    SetPrefabEventIndexToEventSourceNode(pNode, iEventIndex);

    SetPrefabEventNodePortVisibility(pNode, "EventId", portVisibility.bEventIdPortVisible, true);
    SetPrefabEventNodePortVisibility(pNode, "EventIndex", portVisibility.bEventIndexPortVisible, true);

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::RemovePrefabEventSourceNode(const QString& prefabName, TPrefabInstancesData& instancesData, SPrefabEventInstanceEventsData& instanceEventsData, SPrefabEventSourceData& eventData, const QString& eventName, CFlowNode* pNode, bool bRemoveDataOnNode)
{
    CRY_ASSERT(pNode);

    if (bRemoveDataOnNode) // Won't do this when node is being destroyed
    {
        SetPrefabEventIndexToEventSourceNode(pNode, -1);   // Want to redetermine event index

        // Flownode event eFE_Initialize causes custom events to be registered. Currently no way to only do that
        // only when gEnv->bEditorGame == true as when entering editor, bEditorGame = true assignment happens later than it should
        // For now clear events to be safe won't be leaving a dangling pointer
        UnregisterPrefabEventSourceEvent(pNode);
    }

    TPrefabEventNodeContainer& nodeContainer = eventData.m_eventSourceNodes;
    TPrefabEventNodeContainer::iterator nodeContainerIter = nodeContainer.find(pNode);
    CRY_ASSERT(nodeContainerIter != nodeContainer.end());
    if (nodeContainerIter != nodeContainer.end())
    {
        nodeContainer.erase(nodeContainerIter);

        // If just removed last event source, need to remove event and update all instances
        if (nodeContainer.size() == 0)
        {
            TPrefabEventSourceData& eventSourcesData = instanceEventsData.m_eventSourceData;
            TPrefabEventSourceData::iterator eventSourcesDataIter = eventSourcesData.find(eventName);
            CRY_ASSERT(eventSourcesDataIter != eventSourcesData.end());
            if (eventSourcesDataIter != eventSourcesData.end())
            {
                eventSourcesData.erase(eventSourcesDataIter);

                if (instanceEventsData.m_eventInstanceNodes.size() > 0)
                {
                    // Need to update all instances
                    UpdatePrefabInstanceNodeEvents(instancesData);
                }
            }
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::UnregisterPrefabEventSourceEvent(CFlowNode* pNode)
{
    TCustomEventId curEventId = CUSTOMEVENTID_INVALID;
    GetPrefabEventIdFromEventSourceNode(pNode, curEventId);
    if (curEventId != CUSTOMEVENTID_INVALID)
    {
        SetPrefabEventIdToEventSourceNode(pNode, -1);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::RemovePrefabInstanceNode(SPrefabEventInstanceEventsData* pInstanceEventsData, CFlowNode* pNode, bool bRemoveDataOnNode)
{
    CRY_ASSERT(pInstanceEventsData != NULL);
    CRY_ASSERT(pNode != NULL);

    // Perform remove from container
    TPrefabEventNodeContainer& nodeContainer = pInstanceEventsData->m_eventInstanceNodes;
    TPrefabEventNodeContainer::iterator iter = nodeContainer.find(pNode);
    CRY_ASSERT(iter != nodeContainer.end());
    if (iter != nodeContainer.end())
    {
        nodeContainer.erase(iter);

        if (bRemoveDataOnNode) // Won't do this when node is being destroyed
        {
            // Flownode event eFE_Initialize causes custom events to be registered. Currently no way to only do that
            // only when gEnv->bEditorGame == true as when entering editor, bEditorGame = true assignment happens later than it should
            // For now clear events to be safe won't be leaving a dangling pointer
            UnregisterPrefabInstanceEvents(pNode);
        }

        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::UnregisterPrefabInstanceEvents(CFlowNode* pNode)
{
    // Flownode event eFE_Initialize causes custom events to be registered. Currently no way to only do that
    // only when gEnv->bEditorGame == true as when entering editor, bEditorGame = true assignment happens later than it should
    // For now clear events to be safe won't be leaving a dangling pointer
    QString eventName;

    for (int i = 1; i <= CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
    {
        eventName = QString("Event%1").arg(i);

        CHyperNodePort* pEventTriggerInputPort = pNode->FindPort(eventName, true);
        CHyperNodePort* pEventTriggerOutputPort = pNode->FindPort(eventName, false);
        CHyperNodePort* pEventNamePort = pNode->FindPort(eventName + "Name", true);
        CHyperNodePort* pEventIdPort = pNode->FindPort(eventName + "Id", true);

        CRY_ASSERT(pEventTriggerInputPort && pEventTriggerOutputPort && pEventNamePort && pEventIdPort);
        if (pEventTriggerInputPort && pEventTriggerOutputPort && pEventNamePort && pEventIdPort)
        {
            int iCustomEventId = (int)CUSTOMEVENTID_INVALID;
            pEventIdPort->pVar->Get(iCustomEventId);

            // Get rid of the event id set on node itself incase this node is going to be reused from a change
            pEventIdPort->pVar->Set((int)CUSTOMEVENTID_INVALID);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetEventInstanceNodeData(CFlowNode* pNode, const QString** ppPrefabName, const QString** ppPrefabInstanceName, SPrefabEventInstanceEventsData** ppInstanceEventsData)
{
    CRY_ASSERT(pNode != NULL);
    CRY_ASSERT(ppPrefabName != NULL);
    CRY_ASSERT(ppPrefabInstanceName != NULL);
    CRY_ASSERT(ppInstanceEventsData != NULL);

    TPrefabNameToInstancesData::iterator allDataIter = m_prefabEventData.begin();
    const TPrefabNameToInstancesData::const_iterator allDataIterEnd = m_prefabEventData.end();
    while (allDataIter != allDataIterEnd)
    {
        TPrefabInstancesData& instancesData = allDataIter->second;

        // Search assigned events + containers first
        TPrefabInstancesData::iterator instancesDataIter = instancesData.begin();
        const TPrefabInstancesData::const_iterator instancesDataIterEnd = instancesData.end();
        while (instancesDataIter != instancesDataIterEnd)
        {
            SPrefabEventInstanceEventsData& instanceData = instancesDataIter->second;

            TPrefabEventNodeContainer* pNodeContainer = &instanceData.m_eventInstanceNodes;
            if (pNodeContainer->find(pNode) != pNodeContainer->end())
            {
                *ppPrefabName = &allDataIter->first;
                *ppPrefabInstanceName = &instancesDataIter->first;
                *ppInstanceEventsData = &instanceData;
                return true;
            }

            ++instancesDataIter;
        }

        ++allDataIter;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetEventSourceNodeData(CFlowNode* pNode,
    const QString** ppPrefabName,
    const QString** ppPrefabInstanceName,
    const QString** ppEventName,
    TPrefabInstancesData** ppInstancesData,
    SPrefabEventInstanceEventsData** ppInstanceEventsData,
    SPrefabEventSourceData** ppEventSourceData)
{
    CRY_ASSERT(pNode != NULL);
    CRY_ASSERT(ppPrefabName != NULL);
    CRY_ASSERT(ppPrefabInstanceName != NULL);
    CRY_ASSERT(ppEventName != NULL);
    CRY_ASSERT(ppInstancesData != NULL);
    CRY_ASSERT(ppInstanceEventsData != NULL);
    CRY_ASSERT(ppEventSourceData != NULL);

    TPrefabNameToInstancesData::iterator allDataIter = m_prefabEventData.begin();
    const TPrefabNameToInstancesData::const_iterator allDataIterEnd = m_prefabEventData.end();
    while (allDataIter != allDataIterEnd)
    {
        TPrefabInstancesData& instancesData = allDataIter->second;

        // Search assigned events + containers first
        TPrefabInstancesData::iterator instancesDataIter = instancesData.begin();
        const TPrefabInstancesData::const_iterator instancesDataIterEnd = instancesData.end();
        while (instancesDataIter != instancesDataIterEnd)
        {
            SPrefabEventInstanceEventsData& instanceData = instancesDataIter->second;

            TPrefabEventSourceData::iterator sourceEventDataIter = instanceData.m_eventSourceData.begin();
            const TPrefabEventSourceData::const_iterator sourceEventDataIterEnd = instanceData.m_eventSourceData.end();

            while (sourceEventDataIter != sourceEventDataIterEnd)
            {
                SPrefabEventSourceData& sourceData = sourceEventDataIter->second;
                TPrefabEventNodeContainer* pNodeContainer = &sourceData.m_eventSourceNodes;
                if (pNodeContainer->find(pNode) != pNodeContainer->end())
                {
                    *ppInstancesData = &instancesData;
                    *ppPrefabName = &allDataIter->first;
                    *ppPrefabInstanceName = &instancesDataIter->first;
                    *ppInstanceEventsData = &instanceData;
                    *ppEventName = &sourceEventDataIter->first;
                    *ppEventSourceData = &sourceData;
                    return true;
                }

                ++sourceEventDataIter;
            }

            ++instancesDataIter;
        }

        ++allDataIter;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetPrefabIdentifiersFromStoredData(CFlowNode* pNode, QString& prefabName, QString& prefabInstanceName)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPort = pNode->FindPort("PrefabName", true);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        pPort->pVar->Get(prefabName);

        if (prefabName.isEmpty())
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    pPort = pNode->FindPort("InstanceName", true);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        pPort->pVar->Get(prefabInstanceName);

        if (prefabInstanceName.isEmpty())
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetPrefabIdentifiersFromObject(CFlowNode* pNode, QString& prefabName, QString& prefabInstanceName)
{
    CRY_ASSERT(pNode != NULL);

    // Determine prefab object from graph holding the node
    IFlowGraph* pGraph = pNode->GetIFlowGraph();
    CRY_ASSERT(pGraph != NULL);
    if (pGraph)
    {
        const EntityId graphEntityId = pGraph->GetGraphEntity(0);
        CRY_ASSERT(graphEntityId != 0);
        if (graphEntityId != 0)
        {
            // Get editor object that represents the entity
            CEntityObject* pEntityObject = CEntityObject::FindFromEntityId(graphEntityId);
            CRY_ASSERT(pEntityObject != NULL);
            if (pEntityObject != NULL)
            {
                if (CPrefabObject* pFoundPrefabObject = pEntityObject->GetPrefab())
                {
                    CPrefabItem* pPrefabItem = pFoundPrefabObject->GetPrefab();
                    CRY_ASSERT(pPrefabItem != NULL);
                    if (pPrefabItem)
                    {
                        prefabName = pPrefabItem->GetFullName();
                        prefabInstanceName = pFoundPrefabObject->GetName();
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::SetPrefabIdentifiersToEventNode(CFlowNode* pNode, const QString& prefabName, const QString& prefabInstanceName)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPrefabNamePort = pNode->FindPort("PrefabName", true);
    CHyperNodePort* pPrefabInstancePort = pNode->FindPort("InstanceName", true);
    CRY_ASSERT(pPrefabNamePort && pPrefabInstancePort);
    if (pPrefabNamePort && pPrefabInstancePort)
    {
        pPrefabNamePort->pVar->Set(prefabName);
        pPrefabNamePort->pVar->SetHumanName("PREFAB");
        pPrefabInstancePort->pVar->Set(prefabInstanceName);
        pPrefabInstancePort->pVar->SetHumanName("INSTANCE");
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::SetPrefabEventIdToEventSourceNode(CFlowNode* pNode, const TCustomEventId eventId)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPort = pNode->FindPort("EventId", true);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        pPort->pVar->Set((int)eventId);
        pPort->bVisible = false;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::SetPrefabEventIndexToEventSourceNode(CFlowNode* pNode, const int iEventIndex)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPort = pNode->FindPort("EventIndex", true);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        pPort->pVar->Set(iEventIndex);
        pPort->bVisible = false;
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetPrefabEventIndexFromEventSourceNode(CFlowNode* pNode, int& iEventIndex)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPort = pNode->FindPort("EventIndex", true);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        pPort->pVar->Get(iEventIndex);
        return true;
    }

    iEventIndex = -1;
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetPrefabEventNameFromEventSourceNode(CFlowNode* pNode, QString& eventName)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPort = pNode->FindPort("EventName", true);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        pPort->pVar->Get(eventName);
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetPrefabEventIdFromEventSourceNode(CFlowNode* pNode, TCustomEventId& eventId)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPort = pNode->FindPort("EventId", true);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        int iEventIdVal;
        pPort->pVar->Get(iEventIdVal);
        eventId = (TCustomEventId)iEventIdVal;
        return true;
    }

    eventId = CUSTOMEVENTID_INVALID;
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::UpdatePrefabInstanceNodeEvents(TPrefabInstancesData& instancesData)
{
    TPrefabInstancesData::iterator instancesDataIter = instancesData.begin();
    const TPrefabInstancesData::const_iterator instancesDataIterEnd = instancesData.end();
    while (instancesDataIter != instancesDataIterEnd)
    {
        SPrefabEventInstanceEventsData& instanceEventsData = instancesDataIter->second;

        TPrefabEventNodeContainer& instancesNodeContainer = instanceEventsData.m_eventInstanceNodes;
        TPrefabEventNodeContainer::iterator instanceNodeContainerIter = instancesNodeContainer.begin();
        const TPrefabEventNodeContainer::const_iterator instanceNodeContainerIterEnd = instancesNodeContainer.end();
        while (instanceNodeContainerIter != instanceNodeContainerIterEnd)   // Go through each instance node
        {
            CFlowNode* pNode = *instanceNodeContainerIter;
            CRY_ASSERT(pNode != NULL);

            UpdatePrefabEventsOnInstance(instanceEventsData, pNode);

            ++instanceNodeContainerIter;
        }

        ++instancesDataIter;
    }
}

//////////////////////////////////////////////////////////////////////////
void CPrefabEvents::UpdatePrefabEventsOnInstance(SPrefabEventInstanceEventsData& instanceEventsData, CFlowNode* pInstanceNode)
{
    TPrefabEventSourceData& eventsSourceData = instanceEventsData.m_eventSourceData;

    QString eventName;

    // Go through all the events, setting and showing if it exists, or hide the ports
    for (int i = 1; i <= CUSTOMEVENTS_PREFABS_MAXNPERINSTANCE; i++)
    {
        eventName = QString("Event%1").arg(i);

        CHyperNodePort* pEventTriggerInputPort = pInstanceNode->FindPort(eventName, true);
        CHyperNodePort* pEventTriggerOutputPort = pInstanceNode->FindPort(eventName, false);
        CHyperNodePort* pEventNamePort = pInstanceNode->FindPort(eventName + "Name", true);
        CHyperNodePort* pEventIdPort = pInstanceNode->FindPort(eventName + "Id", true);

        CRY_ASSERT(pEventTriggerInputPort && pEventTriggerOutputPort && pEventNamePort && pEventIdPort);
        if (pEventTriggerInputPort && pEventTriggerOutputPort && pEventNamePort && pEventIdPort)
        {
            SPrefabEventSourceData* pEventSourceData = NULL;
            const QString* pEventName = NULL;
            const bool bResult = GetPrefabEventSourceData(eventsSourceData, i, &pEventSourceData, &pEventName);
            bool bVisibility = false;
            if (bResult)
            {
                bVisibility = true;
                pEventNamePort->pVar->Set(*pEventName);
                pEventIdPort->pVar->Set((int)pEventSourceData->m_eventId);

                // Set human port names so event name is visible instead of event#
                const QString& eventName = *pEventName;
                pEventTriggerInputPort->pVar->SetHumanName(eventName);
                pEventTriggerOutputPort->pVar->SetHumanName(eventName);
            }
            else
            {
                pEventNamePort->pVar->Set("");
                pEventIdPort->pVar->Set((int)CUSTOMEVENTID_INVALID);
                pEventTriggerInputPort->pVar->SetHumanName("InvalidEvent");
                pEventTriggerOutputPort->pVar->SetHumanName("InvalidEvent");
            }

            pEventTriggerInputPort->bVisible = bVisibility;
            pEventTriggerOutputPort->bVisible = bVisibility;
            pEventNamePort->bVisible = false;
            pEventIdPort->bVisible = false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::GetPrefabEventSourceData(TPrefabEventSourceData&   eventsSourceData, const int iEventIndex, SPrefabEventSourceData** ppSourceData, const QString** ppEventName)
{
    TPrefabEventSourceData::iterator iter = eventsSourceData.begin();
    const TPrefabEventSourceData::const_iterator iterEnd = eventsSourceData.end();
    while (iter != iterEnd)
    {
        SPrefabEventSourceData& sourceData = iter->second;
        if (sourceData.m_iEventIndex == iEventIndex)
        {
            *ppSourceData = &sourceData;
            *ppEventName = &iter->first;
            return true;
        }

        ++iter;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPrefabEvents::SetPrefabEventNodePortVisibility(CFlowNode* pNode, const QString& portName, const bool bVisible, const bool bInputPort)
{
    CRY_ASSERT(pNode != NULL);
    CHyperNodePort* pPort = pNode->FindPort(portName, bInputPort);
    CRY_ASSERT(pPort != NULL);
    if (pPort)
    {
        pPort->bVisible = bVisible;
        return true;
    }

    return false;
}
