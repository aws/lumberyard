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


#ifndef CRYINCLUDE_EDITOR_PREFABS_PREFABEVENTS_H
#define CRYINCLUDE_EDITOR_PREFABS_PREFABEVENTS_H
#pragma once

#include <HyperGraph/IHyperGraph.h>
#include <ICustomEvents.h>

class CPrefabObject;
class CFlowNode;

/** Manages prefab events.
*/
class CPrefabEvents
    : public IHyperGraphManagerListener
    , public IHyperGraphListener
{
public:
    CPrefabEvents();
    ~CPrefabEvents();

    // IHyperGraphManagerListener
    virtual void OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode);
    // ~IHyperGraphManagerListener

    // IHyperGraphListener
    virtual void OnHyperGraphEvent(IHyperNode* pNode, EHyperGraphEvent event);
    // ~IHyperGraphListener

    // Initialization code to handle prefab events
    bool Init();

    // Deinitialization code for prefab events
    void Deinit();

    // Adds prefab instance node when selecting a prefab and clicking add selected
    bool AddPrefabInstanceNodeFromSelection(CFlowNode* pNode, CPrefabObject* pPrefabObj);

    // Sets currently setting prefab flag (When cloning, serializing, updating prefabs), used to delay adding of prefab event source nodes since prefab name + instance can't be determined yet
    void SetCurrentlySettingPrefab(const bool bCurrentlySetting);

    // Removes all data associated to prefab events
    void RemoveAllEventData();

    // Handle flow node removal
    void OnFlowNodeRemoval(CFlowNode* pNode);

    // Handle prefab object name change
    void OnPrefabObjectNameChange(CPrefabObject* pPrefabObj, const QString& oldName, const QString& newName);

protected:
    // Container to hold prefab instance or event source nodes
    typedef std::set<CFlowNode*> TPrefabEventNodeContainer;

    // Prefab event data associated with a single source event
    struct SPrefabEventSourceData
    {
        SPrefabEventSourceData()
            : m_iEventIndex(-1)
            , m_eventId(CUSTOMEVENTID_INVALID)
        {
        }

        int                                             m_iEventIndex; // To remember port order on instances
        TCustomEventId                      m_eventId; // Assigned event id for custom event
        TPrefabEventNodeContainer m_eventSourceNodes;
    };

    // Event name to event source data inside an instance
    typedef std::map<QString, SPrefabEventSourceData> TPrefabEventSourceData;

    // Prefab data associated to an instance
    struct SPrefabEventInstanceEventsData
    {
        TPrefabEventSourceData      m_eventSourceData;
        TPrefabEventNodeContainer m_eventInstanceNodes;
    };

    // Instance name to instance data
    typedef std::map<QString, SPrefabEventInstanceEventsData> TPrefabInstancesData;

    // Utility to collect visibility flags
    struct SPrefabPortVisibility
    {
        bool bEventIdPortVisible;
        bool bEventIndexPortVisible;
        bool bPrefabNamePortVisible;
        bool bInstanceNamePortVisible;

        SPrefabPortVisibility()
            : bEventIdPortVisible(false)
            , bEventIndexPortVisible(false)
            , bPrefabNamePortVisible(false)
            , bInstanceNamePortVisible(false) {}
    };

    // Retrieve the current port visibility information of a prefab node
    // This information can be used to restore visibility information when re-adding
    void GetPrefabPortVisibility(CFlowNode* pNode, SPrefabPortVisibility& result);

    // Handles add, change, removing of prefab event source nodes
    void UpdatePrefabEventSourceNode(const EHyperGraphEvent event, CFlowNode* pNode);

    // Handles add, change, removing of prefab event instance nodes
    void UpdatePrefabInstanceNode(const EHyperGraphEvent event, CFlowNode* pNode);

    // Adds prefab instance node from stored data (After initial creation from selection was done)
    bool AddPrefabInstanceNodeFromStoredData(CFlowNode* pNode);

    // Adds prefab source node
    bool AddPrefabEventSourceNode(CFlowNode* pNode, const SPrefabPortVisibility& portVisibility);

    // Removes prefab event source node
    bool RemovePrefabEventSourceNode(const QString& prefabName, TPrefabInstancesData& instancesData, SPrefabEventInstanceEventsData& instanceEventsData, SPrefabEventSourceData& eventData, const QString& eventName, CFlowNode* pNode, bool bRemoveDataOnNode);

    // Unregisters custom event associated to the prefab event source node
    void UnregisterPrefabEventSourceEvent(CFlowNode* pNode);

    // Removes prefab instance node
    bool RemovePrefabInstanceNode(SPrefabEventInstanceEventsData* pInstanceEventsData, CFlowNode* pNode, bool bRemoveDataOnNode);

    // Unregisters custom events associated to the prefab event instance node
    void UnregisterPrefabInstanceEvents(CFlowNode* pNode);

    // Gets event instance node data if it exists
    bool GetEventInstanceNodeData(CFlowNode* pNode, const QString** ppPrefabName, const QString** ppPrefabInstanceName, SPrefabEventInstanceEventsData** ppInstanceEventsData);

    // Gets event source node data if it exists
    bool GetEventSourceNodeData(CFlowNode* pNode,
        const QString** ppPrefabName,
        const QString** ppPrefabInstanceName,
        const QString** ppEventName,
        TPrefabInstancesData** ppInstancesData,
        SPrefabEventInstanceEventsData** ppInstanceEventsData,
        SPrefabEventSourceData** ppEventSourceData);

    // Gets prefab identifiers from stored data
    bool GetPrefabIdentifiersFromStoredData(CFlowNode* pNode, QString& prefabName, QString& prefabInstanceName);

    // Attempts to get prefab identifiers from owning object
    bool GetPrefabIdentifiersFromObject(CFlowNode* pNode, QString& prefabName, QString& prefabInstanceName);

    // Sets prefab identifiers to stored data
    bool SetPrefabIdentifiersToEventNode(CFlowNode* pNode, const QString& prefabName, const QString& prefabInstanceName);

    // Retrieves associated event name
    bool GetPrefabEventNameFromEventSourceNode(CFlowNode* pNode, QString& eventName);

    // Assigns custom event id to event source node
    bool SetPrefabEventIdToEventSourceNode(CFlowNode* pNode, const TCustomEventId eventId);

    // Retrieves custom event id from event source node
    bool GetPrefabEventIdFromEventSourceNode(CFlowNode* pNode, TCustomEventId& eventId);

    // Assigns custom event index (Used to preserve port order on instances) to event source node
    bool SetPrefabEventIndexToEventSourceNode(CFlowNode* pNode, const int iEventIndex);

    // Retrieves custom event index (Used to preserve port order on instances) from event source node
    bool GetPrefabEventIndexFromEventSourceNode(CFlowNode* pNode, int& iEventIndex);

    // Updates all prefab instance nodes associated to a particular prefab instance to represent current events from event sources
    void UpdatePrefabInstanceNodeEvents(TPrefabInstancesData& instancesData);

    // Updates a prefab instance node to represent current events from event sources
    void UpdatePrefabEventsOnInstance(SPrefabEventInstanceEventsData& instanceEventsData, CFlowNode* pInstanceNode);

    // Attempts to retrieve associated event source data corresponding to an event index
    bool GetPrefabEventSourceData(TPrefabEventSourceData&  eventsSourceData, const int iEventIndex, SPrefabEventSourceData** ppSourceData, const QString** ppEventName);

    // Sets prefab event node port visibility (Only needs to reduce clutter when looking at nodes in editor)
    bool SetPrefabEventNodePortVisibility(CFlowNode* pNode, const QString& portName, const bool bVisible, const bool bInputPort);

    typedef std::map<QString, TPrefabInstancesData> TPrefabNameToInstancesData;

    // Mapping of all prefabs that have instances with events
    TPrefabNameToInstancesData  m_prefabEventData;

    // Used to delay adding of event source nodes until setting prefab is complete (Only then can determine prefab + instance names)
    TPrefabEventNodeContainer       m_delayedAddingEventSourceNodes;

    // Whether currently setting prefab or not (Prefab event source nodes created when this is true are put into m_delayedAddingEventSourceNodes)
    bool                                                m_bCurrentlySettingPrefab;
};

#endif // CRYINCLUDE_EDITOR_PREFABS_PREFABEVENTS_H
