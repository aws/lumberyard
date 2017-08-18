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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMANAGER_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMANAGER_H
#pragma once

#include "HyperGraphManager.h"
#include "FlowGraphMigrationHelper.h"
#include <FlowGraphInformation.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

struct IAIAction;
struct IFlowNode;
struct IFlowNodeData;
struct IFlowGraph;
struct IFlowGraphModule;
struct SInputPortConfig;

class CEntityObject;
class CFlowGraph;
class CFlowNode;
class CPrefabObject;

//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CFlowGraphManager
    : public CHyperGraphManager
    , public FlowGraphNotificationBus::Handler
    , AzToolsFramework::EditorEntityContextNotificationBus::Handler
{
public:
    //////////////////////////////////////////////////////////////////////////
    // CHyperGraphManager implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual ~CFlowGraphManager();

    virtual void Init();
    virtual void ReloadClasses();
    virtual void ReloadNodeConfig(IFlowNodeData* pFlowNodeData, CFlowNode* pNode);

    CHyperNode* CreateNode(CHyperGraph* pGraph, const char* sNodeClass, HyperNodeID nodeId, const QPointF& pos, CBaseObject* pObj = NULL, bool bAllowMissing = false) override;
    CHyperGraph* CreateGraph() override;
    //////////////////////////////////////////////////////////////////////////

    void OpenFlowGraphView(IFlowGraph* pIFlowGraph);

    // Unregisters flowgraphs and sets view to 0
    void UnregisterAndResetView(CFlowGraph* pFlowGraph, bool bInitFlowGraph = true);

    // Create graph for specific entity.
    CFlowGraph* CreateGraphForEntity(CEntityObject* pEntity, const char* sGroupName = "");

    // Create graph for specific AI Action.
    CFlowGraph* CreateGraphForAction(IAIAction* pAction);

    CFlowGraph* FindGraphForAction(IAIAction* pAction);

    // Deletes all graphs created for AI Actions
    void FreeGraphsForActions();

    // Create graph for specific Custom Action.
    CFlowGraph* CreateGraphForCustomAction(ICustomAction* pCustomAction);

    // Create graph for specific Material FX Graphs.
    CFlowGraph* CreateGraphForMatFX(IFlowGraphPtr pFG, const QString& filename);

    // Finds graph for custom action
    CFlowGraph* FindGraphForCustomAction(ICustomAction* pCustomAction);

    // Deletes all graphs created for Custom Actions
    void FreeGraphsForCustomActions();
    CFlowGraph* CreateGraphForModule(IFlowGraphModule* pModule);
    CFlowGraph* FindGraph(IFlowGraphPtr pFG);

    void OnEnteringGameMode(bool inGame = true);

    //////////////////////////////////////////////////////////////////////////
    // Create special nodes.
    CFlowNode* CreateSelectedEntityNode(CFlowGraph* pFlowGraph, CBaseObject* pSelObj);
    CFlowNode* CreateEntityNode(CFlowGraph* pFlowGraph, CEntityObject* pEntity);
    CFlowNode* CreatePrefabInstanceNode(CFlowGraph* pFlowGraph, CPrefabObject* pPrefabObj);

    int GetFlowGraphCount() const { return m_graphs.size(); }
    CFlowGraph* GetFlowGraph(int nIndex) { return m_graphs[nIndex]; };
    void GetAvailableGroups(std::set<QString>& outGroups, bool bActionGraphs = false);

    CFlowGraphMigrationHelper& GetMigrationHelper()
    {
        return m_migrationHelper;
    }

    void UpdateLayerName(const QString& oldName, const QString& name);

    //////////////////////////////////////////////////////////////////////////
    // FlowGraphInformationBus implementation
    void ComponentFlowGraphAdded(IFlowGraph* graph, const AZ::EntityId& id, const char* flowGraphName) override;
    void ComponentFlowGraphLoaded(IFlowGraph* graph, const AZ::EntityId& id, const AZStd::string& flowGraphName, const AZStd::string& flowGraphXML, bool isTemporaryGraph) override;
    void ComponentFlowGraphRemoved(IFlowGraph* graph) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework::EditorEntityContextNotificationBus::Handler
    void OnEntityStreamLoadSuccess() override;
    //////////////////////////////////////////////////////////////////////////

private:
    friend class CFlowGraph;
    friend class CUndoFlowGraph;
    void RegisterGraph(CFlowGraph* pGraph);
    void UnregisterGraph(CFlowGraph* pGraph);

    void LoadAssociatedBitmap(class CFlowNode* pFlowNode);

    void GetNodeConfig(IFlowNodeData* pSrcNode, CFlowNode* pFlowNode);
    IVariable* MakeInVar(const SInputPortConfig* pConfig, uint32 portId, CFlowNode* pFlowNode);
    IVariable* MakeSimpleVarFromFlowType(int type);

private:
    // All graphs currently created.
    std::vector<CFlowGraph*> m_graphs;
    CFlowGraphMigrationHelper m_migrationHelper;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMANAGER_H
