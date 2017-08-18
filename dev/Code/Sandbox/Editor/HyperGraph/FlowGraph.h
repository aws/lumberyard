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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPH_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPH_H
#pragma once

#include "HyperGraph.h"

#include <IFlowSystem.h>
#include <IEntitySystem.h>
#include <AzCore/Math/Uuid.h>
#include <FlowGraphInformation.h>

#include "Objects/EntityObject.h" // _smart_ptr requires full definition.

class CFlowNode;
struct IAIAction;
struct ICustomAction;

//////////////////////////////////////////////////////////////////////////
class CFlowEdge
    : public CHyperEdge
{
public:
    SFlowAddress addr_in;
    SFlowAddress addr_out;
};

//////////////////////////////////////////////////////////////////////////
// Specialization of HyperGraph to handle logical Flow Graphs.
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CFlowGraph
    : public CHyperGraph
    , SFlowNodeActivationListener
    , ComponentFlowgraphRuntimeNotificationBus::Handler
    , ComponentHyperGraphRequestsBus::Handler
{
public:
    CFlowGraph(CHyperGraphManager* pManager, const char* sGroupName = "");
    CFlowGraph(CHyperGraphManager* pManager, IFlowGraph* pGameFlowGraph, const char* flowGraphName = nullptr, const char* groupName = nullptr);

    //////////////////////////////////////////////////////////////////////////
    // From CHyperGraph
    //////////////////////////////////////////////////////////////////////////
    IHyperGraph* Clone() override;
    void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar = 0) override;
    virtual void BeginEditing();
    virtual void EndEditing();
    void SetEnabled(bool bEnable) override;
    bool IsFlowGraph() override { return true; }
    bool Migrate(XmlNodeRef& node) override;
    void SetGroupName(const QString& sName) override;
    void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);
    bool CanConnectPorts(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, CHyperEdge* pExistingEdge = NULL) override;
    QString GetType() const { return QStringLiteral("FlowGraph"); }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    IFlowGraph* GetIFlowGraph();
    void SetIFlowGraph(IFlowGraphPtr pFlowGraph);
    void OnEnteringGameMode ();

    // SFlowNodeActivationListener
    virtual bool OnFlowNodeActivation(IFlowGraphPtr pFlowGraph, TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value);
    // ~SFlowNodeActivationListener

    //////////////////////////////////////////////////////////////////////////
    // Assigns current entity of the flow graph.
    // if bAdjustGraphNodes==true assigns this entity to all nodes which have t
    void SetEntity(CEntityObject* pEntity, bool bAdjustGraphNodes = false);
    void SetEntity(const AZ::EntityId& id, bool bAdjustGraphNodes = false);
    CEntityObject* GetEntity() const { return m_pEntity; }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Assigns AI Action to the flow graph.
    void SetAIAction(IAIAction* pAIAction);
    virtual IAIAction* GetAIAction() const { return m_pAIAction; }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Assigns Custom Action to the flow graph.
    void SetCustomAction(ICustomAction* pCustomAction);
    virtual ICustomAction* GetCustomAction() const { return m_pCustomAction; }
    //////////////////////////////////////////////////////////////////////////

    enum EMultiPlayerType
    {
        eMPT_ServerOnly = 0,
        eMPT_ClientOnly,
        eMPT_ClientServer
    };

    void SetMPType(EMultiPlayerType mpType)
    {
        m_mpType = mpType;
    }

    EMultiPlayerType GetMPType()
    {
        return m_mpType;
    }

    virtual void ValidateEdges(CHyperNode* pNode);
    virtual IUndoObject* CreateUndo();
    virtual CFlowNode* FindFlowNode(TFlowNodeId id) const;

    // Extract GUID remapping from the prefab if the FG is part of one
    // This is needed in order to properly deserialize
    void ExtractObjectsPrefabIdToGlobalIdMappingFromPrefab(CObjectArchive& archive);

    inline AZ::Uuid GetHyperGraphId() const
    {
        return m_hypergraphId;
    }

    void SetHyperGraphId(const AZ::Uuid& id);

    //////////////////////////////////////////////////////////////////////////
    // ComponentFlowgraphRuntimeNotificationBus implementation
    void OnFlowNodeActivated(IFlowGraphPtr pFlowGraph, TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value) override;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // ComponentHyperGraphRequestsBus implementation
    inline IFlowGraph* GetComponentRuntimeFlowgraph() override
    {
        return m_componentEntityRuntimeFlowgraph;
    }

    inline IFlowGraph* GetLegacyRuntimeFlowgraph() override
    {
        return m_legacyRuntimeFlowGraph;
    }

    CFlowGraph* GetHyperGraph() override
    {
        return this;
    }

    void BindToFlowgraph(IFlowGraphPtr pFlowGraph) override;
    void UnBindFromFlowgraph() override;
    //////////////////////////////////////////////////////////////////////////

protected:

    virtual ~CFlowGraph();

    virtual void RemoveEdge(CHyperEdge* pEdge);
    virtual CHyperEdge* MakeEdge(CHyperNode* pNodeOut, CHyperNodePort* pPortOut, CHyperNode* pNodeIn, CHyperNodePort* pPortIn, bool bEnabled, bool fromSpecialDrag);
    virtual void OnPostLoad();
    virtual void EnableEdge(CHyperEdge* pEdge, bool bEnable);
    virtual void ClearAll();

    void InitializeFromGame();

protected:

    /*
    * This is the runtime version of the flowgraph for legacy. It is constructed at edit time and stays in memory
    * as long as the editor is running.
    */
    IFlowGraphPtr m_legacyRuntimeFlowGraph;

    /*
    * This is the runtime version of flowgraphs for component entities. It is constructed when the simulation starts
    * and goes away when the simulation has ended.
    * points to nullptr when the game / simulation is not running (at Edit time)
    */
    IFlowGraph* m_componentEntityRuntimeFlowgraph;

    _smart_ptr<CEntityObject> m_pEntity;
    IAIAction* m_pAIAction;
    ICustomAction* m_pCustomAction;

    // When set to true this flow graph is editable.
    bool m_bEditable;

    // MultiPlayer Type of this flowgraph (default ServerOnly)
    EMultiPlayerType m_mpType;

    //! a unique identifier for every HyperGraph that gets created
    AZ::Uuid m_hypergraphId;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPH_H
