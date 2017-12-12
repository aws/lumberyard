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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHMANAGER_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHMANAGER_H
#pragma once

#include "HyperGraphNode.h"

#include <QStringList>

struct IHyperGraph;

class CHyperGraph;
class CHyperNode;

//////////////////////////////////////////////////////////////////////////
// Manages collection of hyper node classes that can be created for hyper graphs.
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CHyperGraphManager
{
public:
    typedef Functor1wRet<CHyperNode*, bool> NodeFilterFunctor;

    virtual ~CHyperGraphManager() {}

    //////////////////////////////////////////////////////////////////////////
    // Initialize graph manager.
    // Must be called after full game initialization.
    virtual void Init();

    // Reload prototype node classes.
    virtual void ReloadClasses() = 0;

    void GetPrototypes(QStringList& prototypes, bool bForUI, NodeFilterFunctor filterFunc = NodeFilterFunctor());
    void GetPrototypesEx(std::vector<THyperNodePtr>& prototypes, bool bForUI, NodeFilterFunctor filterFunc = NodeFilterFunctor());

    virtual CHyperNode* GetPrototypeByTag(const char* sNodeClass, bool checkUIName);

    virtual CHyperGraph* CreateGraph() = 0;
    virtual CHyperNode* CreateNode(CHyperGraph* pGraph, const char* sNodeClass, HyperNodeID nodeId, const QPointF& pos = QPointF(0.0f, 0.0f), CBaseObject* pObj = NULL, bool bAllowMissing = false);
    //////////////////////////////////////////////////////////////////////////

    virtual void AddListener(IHyperGraphManagerListener* pListener);
    virtual void RemoveListener(IHyperGraphManagerListener* pListener);

    // Opens view of the specified hyper graph.
    void OpenView(IHyperGraph* pGraph);

    void SendNotifyEvent(EHyperGraphEvent event, IHyperGraph* pGraph = 0, IHyperNode* pNode = 0);
    // Call this function to stop the listeners from getting events from the manager (e.g. sometimes we want to surpress the events and call send them at a later stage)
    void DisableNotifyListeners(bool disable) { m_notifyListenersDisabled = disable; }
    // Main function to enable/disable event processing in GUI controls displaying flowgraph upon FG changes
    void SetGUIControlsProcessEvents(bool active, bool refreshTreeCtrList);
    // Function to set a current hypergraph in the active view
    void SetCurrentViewedGraph(CHyperGraph* pGraph);

protected:
    typedef std::map<QString, THyperNodePtr> NodesPrototypesMap;
    NodesPrototypesMap m_prototypes;

    typedef std::list<IHyperGraphManagerListener*> Listeners;
    Listeners m_listeners;
    bool m_notifyListenersDisabled;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHMANAGER_H
