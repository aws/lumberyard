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

#include "StdAfx.h"

#include <IAIAction.h>

#include "GameEngine.h"
#include "FlowGraph.h"
#include "FlowGraphNode.h"
#include "FlowGraphManager.h"
#include "FlowGraphMigrationHelper.h"
#include "CommentNode.h"
#include "CommentBoxNode.h"
#include "TrackEventNode.h"
#include "MissingNode.h"
#include "Objects/EntityObject.h"
#include "Objects/PrefabObject.h"
#include "HyperGraph/HyperGraphDialog.h"
#include "ErrorReport.h"
#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/API/ComponentEntityObjectBus.h>

#define FG_INTENSIVE_DEBUG
#undef  FG_INTENSIVE_DEBUG



//////////////////////////////////////////////////////////////////////////
// CUndoFlowGraph
//
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Undo object for CFlowGraph.
// Remarks [AlexL]: The CUndoFlowGraph serializes the graph to XML to store
//                  the current state. If it is an entity FlowGraph we use
//                  the entity's GUID to lookup the FG for the entity, because
//                  when an entity is deleted and then re-created from undo,
//                  the FG is also recreated and hence the FG pointer in the Undo
//                  object is garbage! AIAction's may not suffer from this, but
//                  maybe we should reference these by names then or something else...
//                  Currently for simple node changes [Select, PosChange] we create a
//                  full graph XML as well. The Undo hypernode suffers from the same
//                  problem as Graphs, because nodes are deleted and recreated, so pointer
//                  storing is BAD. Maybe use the nodeID [are these surely be the same on recreation?]
//////////////////////////////////////////////////////////////////////////
class CUndoFlowGraph
    : public IUndoObject
{
public:
    CUndoFlowGraph(CFlowGraph* pGraph)
    {
        // Stores the current state of given graph.
        assert(pGraph != 0);
        assert(pGraph->IsFlowGraph());

        m_pGraph = 0;
        m_entityGUID = GuidUtil::NullGuid;

        CEntityObject* pEntity = pGraph->GetEntity();
        if (pEntity)
        {
            m_entityGUID = pEntity->GetId();
            pEntity->SetLayerModified();
        }
        else
        {
            m_pGraph = pGraph;
        }

        m_redo = 0;
        m_undo = XmlHelpers::CreateXmlNode("HyperGraph");
        pGraph->Serialize(m_undo, false);
#ifdef FG_INTENSIVE_DEBUG
        CryLogAlways("CUndoFlowGraph 0x%p:: Serializing from graph 0x%p", this, pGraph);
#endif
    }

protected:
    CFlowGraph* GetGraph()
    {
        if (GuidUtil::IsEmpty(m_entityGUID))
        {
            assert (m_pGraph != 0);
            return m_pGraph;
        }
        CBaseObject* pObj = GetIEditor()->GetObjectManager()->FindObject(m_entityGUID);
        assert (pObj != 0);
        if (qobject_cast<CEntityObject*>(pObj))
        {
            CEntityObject* pEntity = static_cast<CEntityObject*> (pObj);
            XmlNodeRef hypergraphId = m_undo->findChild("HyperGraphId");
            if (hypergraphId)
            {
                auto id = hypergraphId->getAttr("Id");
                CFlowGraph* currentHypergraph;
                EBUS_EVENT_ID_RESULT(currentHypergraph, id, ComponentHyperGraphRequestsBus, GetHyperGraph);
                return currentHypergraph;
            }
        }
        return nullptr;
    }

    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "FlowGraph Undo"; };

    virtual void Undo(bool bUndo)
    {
        CFlowGraph* pGraph = GetGraph();
        assert (pGraph != 0);

        if (bUndo)
        {
#ifdef FG_INTENSIVE_DEBUG
            CryLogAlways("CUndoFlowGraph 0x%p::Undo 1: Serializing to graph 0x%p", this, pGraph);
#endif

            m_redo = XmlHelpers::CreateXmlNode("HyperGraph");
            if (pGraph)
            {
                pGraph->Serialize(m_redo, false);
                GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_OWNER_CHANGE);
            }
        }

#ifdef FG_INTENSIVE_DEBUG
        CryLogAlways("CUndoFlowGraph 0x%p::Undo 2: Serializing to graph 0x%p", this, pGraph);
#endif

        if (pGraph)
        {
            CObjectArchive remappingInfo(GetIEditor()->GetObjectManager(), m_undo, true);
            pGraph->ExtractObjectsPrefabIdToGlobalIdMappingFromPrefab(remappingInfo);

            pGraph->Serialize(m_undo, true, &remappingInfo);
            GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE, pGraph);
            CEntityObject* pEntity = pGraph->GetEntity();
            if (pEntity)
            {
                pEntity->SetLayerModified();
            }

            pGraph->SendNotifyEvent(nullptr, EHG_GRAPH_UNDO_REDO);
            // If we are part of the prefab set this graph to the current viewed one.
            // You can potentially be looking at the same graph but in a different prefab.
            // This undo action could be on the not currently viewed one so enforce the refresh of the view
            if (pEntity && pEntity->IsPartOfPrefab())
            {
                GetIEditor()->GetFlowGraphManager()->SetCurrentViewedGraph(pGraph);
            }
        }
    }
    virtual void Redo()
    {
        if (m_redo)
        {
            CFlowGraph* pGraph = GetGraph();
            assert (pGraph != 0);
            if (pGraph)
            {
#ifdef FG_INTENSIVE_DEBUG
                CryLogAlways("CUndoFlowGraph 0x%p::Redo: Serializing to graph 0x%p", this, pGraph);
#endif
                CObjectArchive remappingInfo(GetIEditor()->GetObjectManager(), m_redo, true);
                ;
                pGraph->ExtractObjectsPrefabIdToGlobalIdMappingFromPrefab(remappingInfo);

                pGraph->Serialize(m_redo, true, &remappingInfo);
                GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE, pGraph);
                CEntityObject* pEntity = pGraph->GetEntity();
                if (pEntity)
                {
                    pEntity->SetLayerModified();
                }

                pGraph->SendNotifyEvent(nullptr, EHG_GRAPH_UNDO_REDO);

                // If we are part of the prefab set this graph to the current viewed one.
                // You can potentially be looking at the same graph but in a different prefab.
                // This undo action could be on the not currently viewed one so enforce the refresh of the view
                if (pEntity && pEntity->IsPartOfPrefab())
                {
                    GetIEditor()->GetFlowGraphManager()->SetCurrentViewedGraph(pGraph);
                }
            }
            else
            {
                CryLogAlways("CUndoFlowGraph 0x%p: Got 0 graph", this);
            }
        }
    }
private:
    GUID m_entityGUID;
    CFlowGraph* m_pGraph;
    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
};




//////////////////////////////////////////////////////////////////////////
// CFlowGraph
//
//////////////////////////////////////////////////////////////////////////
CFlowGraph::CFlowGraph(CHyperGraphManager* pManager, const char* sGroupName)
    : CHyperGraph(pManager)
    , m_componentEntityRuntimeFlowgraph(nullptr)
    , m_bEditable(true)
    , m_pEntity(nullptr)
    , m_pAIAction(nullptr)
    , m_pCustomAction(nullptr)
    , m_legacyRuntimeFlowGraph(nullptr)
    , m_mpType(eMPT_ClientServer)
{
#ifdef FG_INTENSIVE_DEBUG
    CryLogAlways("CFlowGraph: Creating 0x%p", this);
#endif
    SetGroupName(sGroupName);
    IFlowSystem* pFlowSystem = GetIEditor()->GetGameEngine()->GetIFlowSystem();
    if (pFlowSystem)
    {
        SetIFlowGraph(pFlowSystem->CreateFlowGraph());
    }

    SetHyperGraphId(AZ::Uuid::Create());

    GetIEditor()->GetFlowGraphManager()->RegisterGraph(this);
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph::CFlowGraph(CHyperGraphManager* pManager, IFlowGraph* pGameFlowGraph, const char* flowGraphName, const char* groupName)
    : CHyperGraph(pManager)
    , m_componentEntityRuntimeFlowgraph(nullptr)
    , m_bEditable(true)
    , m_pEntity(nullptr)
    , m_pAIAction(nullptr)
    , m_pCustomAction(nullptr)
    , m_mpType(eMPT_ClientServer)
{
    AZ_Assert(pGameFlowGraph, "A game side flowgraph was not supplied");

    if (flowGraphName)
    {
        SetName(flowGraphName);
    }

    if (groupName)
    {
        SetGroupName(groupName);
    }

    SetIFlowGraph(pGameFlowGraph);

    SetHyperGraphId(AZ::Uuid::Create());

    GetIEditor()->GetFlowGraphManager()->RegisterGraph(this);
}

//////////////////////////////////////////////////////////////////////////
CFlowGraph::~CFlowGraph()
{
#ifdef FG_INTENSIVE_DEBUG
    CryLogAlways("CFlowGraph: About to delete 1 0x%p", this);
#endif

    if (m_legacyRuntimeFlowGraph)
    {
        m_legacyRuntimeFlowGraph->RemoveFlowNodeActivationListener(this);
    }

    // first check is the manager still there!
    // could be deleted in which case we don't
    // care about unregistering.
    CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
    if (pManager)
    {
        pManager->UnregisterAndResetView(this);
    }

    ComponentFlowgraphRuntimeNotificationBus::Handler::BusDisconnect();
    ComponentHyperGraphRequestsBus::Handler::BusDisconnect();

#ifdef FG_INTENSIVE_DEBUG
    CryLogAlways("CFlowGraph: About to delete 2 0x%p", this);
#endif
}

//////////////////////////////////////////////////////////////////////////
IFlowGraph* CFlowGraph::GetIFlowGraph()
{
    if (!m_legacyRuntimeFlowGraph)
    {
        IFlowSystem* pFlowSystem = GetIEditor()->GetGameEngine()->GetIFlowSystem();
        if (pFlowSystem)
        {
            SetIFlowGraph(pFlowSystem->CreateFlowGraph());
        }
    }

    return m_legacyRuntimeFlowGraph;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::SetIFlowGraph(IFlowGraphPtr pFlowGraph)
{
    if (m_legacyRuntimeFlowGraph)
    {
        m_legacyRuntimeFlowGraph->RemoveFlowNodeActivationListener(this);
    }

    if (pFlowGraph)
    {
        m_legacyRuntimeFlowGraph = pFlowGraph;
        ComponentFlowgraphRuntimeNotificationBus::Handler::BusDisconnect();
        ComponentFlowgraphRuntimeNotificationBus::Handler::BusConnect(m_legacyRuntimeFlowGraph->GetGraphId());
        m_legacyRuntimeFlowGraph->RegisterFlowNodeActivationListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraph::OnFlowNodeActivation(IFlowGraphPtr pFlowGraph, TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value)
{
    NodesMap::iterator it = m_nodesMap.begin();
    NodesMap::iterator end = m_nodesMap.end();
    for (; it != end; ++it)
    {
        CFlowNode* pNode = static_cast<CFlowNode*>(it->second.get());
        TFlowNodeId id = pNode->GetFlowNodeId();
        if (id == toNode)
        {
            pNode->DebugPortActivation(toPort, value);
            return false;
        }
        //else if(id == srcNode)
        //  pNode->DebugPortActivation(srcPort, "out");
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::OnEnteringGameMode ()
{
    IHyperGraphEnumerator* pEnum = GetNodesEnumerator();
    for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
    {
        CHyperNode* pNode = (CHyperNode*)pINode;
        pNode->OnEnteringGameMode();
    }
    pEnum->Release();
}


//////////////////////////////////////////////////////////////////////////
void CFlowGraph::ClearAll()
{
    GetIFlowGraph()->Clear();
    CHyperGraph::ClearAll();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::RemoveEdge(CHyperEdge* pEdge)
{
    // Remove link in flow system.
    CFlowEdge* pFlowEdge = (CFlowEdge*)pEdge;
    GetIFlowGraph()->UnlinkNodes(pFlowEdge->addr_out, pFlowEdge->addr_in);

    CHyperGraph::RemoveEdge(pEdge);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::EnableEdge(CHyperEdge* pEdge, bool bEnable)
{
    CHyperGraph::EnableEdge(pEdge, bEnable);

    SetModified();

    CFlowEdge* pFlowEdge = (CFlowEdge*)pEdge;

    if (!bEnable)
    {
        GetIFlowGraph()->UnlinkNodes(pFlowEdge->addr_out, pFlowEdge->addr_in);
    }
    else
    {
        GetIFlowGraph()->LinkNodes(pFlowEdge->addr_out, pFlowEdge->addr_in);
    }
}

//////////////////////////////////////////////////////////////////////////
CHyperEdge* CFlowGraph::MakeEdge(CHyperNode* pNodeOut, CHyperNodePort* pPortOut, CHyperNode* pNodeIn, CHyperNodePort* pPortIn, bool bEnabled, bool fromSpecialDrag)
{
    assert(pNodeIn);
    assert(pNodeOut);
    assert(pPortIn);
    assert(pPortOut);
    CFlowEdge* pEdge = new CFlowEdge;

    pEdge->nodeIn = pNodeIn->GetId();
    pEdge->nodeOut = pNodeOut->GetId();
    pEdge->portIn = pPortIn->GetName();
    pEdge->portOut = pPortOut->GetName();
    pEdge->enabled = bEnabled;

    pEdge->nPortIn = pPortIn->nPortIndex;
    pEdge->nPortOut = pPortOut->nPortIndex;

    ++pPortOut->nConnected;
    ++pPortIn->nConnected;

    RegisterEdge(pEdge, fromSpecialDrag);

    SetModified();

    // Create link in flow system.
    pEdge->addr_in = SFlowAddress(((CFlowNode*)pNodeIn)->GetFlowNodeId(), pPortIn->nPortIndex, false);
    pEdge->addr_out = SFlowAddress(((CFlowNode*)pNodeOut)->GetFlowNodeId(), pPortOut->nPortIndex, true);
    if (bEnabled)
    {
        m_legacyRuntimeFlowGraph->LinkNodes(pEdge->addr_out, pEdge->addr_in);
        if (!m_bLoadingNow)
        {
            m_legacyRuntimeFlowGraph->InitializeValues();
        }
    }
    return pEdge;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::SetEntity(const AZ::EntityId& id, bool bAdjustGraphNodes /* = false */)
{
    // Set the entity on the editor flowgraph for graph entity reference
    CEntityObject* entityObject = nullptr;
    EBUS_EVENT_ID_RESULT(entityObject, id, AzToolsFramework::ComponentEntityEditorRequestBus, GetSandboxObject);

    if (entityObject)
    {
        SetEntity(entityObject, bAdjustGraphNodes);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::SetEntity(CEntityObject* pEntity, bool bAdjustGraphNodes)
{
    assert(pEntity);
    m_pEntity = pEntity;

    if (m_legacyRuntimeFlowGraph != NULL && m_pEntity != NULL && m_pEntity->GetIEntity())
    {
        m_legacyRuntimeFlowGraph->SetGraphEntity(FlowEntityId(m_pEntity->GetEntityId()), 0);
    }
    if (bAdjustGraphNodes)
    {
        IHyperGraphEnumerator* pEnum = GetNodesEnumerator();
        for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
        {
            CHyperNode* pNode = (CHyperNode*)pINode;
            if (pNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY))
            {
                ((CFlowNode*)pNode)->SetEntity(pEntity);
            }
        }
        pEnum->Release();
    }
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_OWNER_CHANGE);
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_UPDATE_ENTITY, this);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::SetAIAction(IAIAction* pAIAction)
{
    m_pAIAction = pAIAction;

    GetIFlowGraph()->SetType(IFlowGraph::eFGT_AIAction);

    if (0 != m_legacyRuntimeFlowGraph)
    {
        m_legacyRuntimeFlowGraph->SetAIAction(pAIAction); // KLUDE to make game flowgraph aware that this is an AI action
        m_legacyRuntimeFlowGraph->SetAIAction(0);         // KLUDE cont'd: for safety we re-set afterwards (game flowgraph remembers)
    }
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_OWNER_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::SetCustomAction(ICustomAction* pCustomAction)
{
    m_pCustomAction = pCustomAction;

    GetIFlowGraph()->SetType(IFlowGraph::eFGT_CustomAction);

    if (0 != m_legacyRuntimeFlowGraph)
    {
        m_legacyRuntimeFlowGraph->SetCustomAction(pCustomAction); // KLUDE to make game flowgraph aware that this is an Custom action
        m_legacyRuntimeFlowGraph->SetCustomAction(0);         // KLUDE cont'd: for safety we re-set afterwards (game flowgraph remembers)
    }
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_OWNER_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
IHyperGraph* CFlowGraph::Clone()
{
    CFlowGraph* pGraph = new CFlowGraph(GetManager());

    if (!m_bEditable)
    {
        pGraph->SetIFlowGraph(m_legacyRuntimeFlowGraph->Clone());
        pGraph->SetModified();
        return pGraph;
    }

    pGraph->m_bLoadingNow = true;

    IHyperGraphEnumerator* pEnum = GetNodesEnumerator();
    for (IHyperNode* pINode = pEnum->GetFirst(); pINode; pINode = pEnum->GetNext())
    {
        CHyperNode* pSrcNode = (CHyperNode*)pINode;

        // create a clone of the node
        CHyperNode* pNewNode = pSrcNode->Clone();

        // comment is the only node which is no CFlowNode ... not entirely true anymore
        if (!pNewNode->IsFlowNode())
        {
            pNewNode->SetGraph(pGraph);
            pNewNode->Init();
        }
        else
        {
            // clone the node, this creates only our Editor shallow node
            // also assigns the m_pEntity (which we might have to correct!)
            CFlowNode* pNode = static_cast<CFlowNode*> (pNewNode);

            // set the graph of the newly created node
            pNode->SetGraph(pGraph);

            // create a real flowgraph node (note: inputs are not yet set!)
            pNode->Init();

            // set the inputs of the base FG node (shallow already has correct values
            // because pVars are copied by pSrcNode->Clone()
            pNode->SetInputs(false, true);  // Make sure entities in normal ports are set!

            // WE CAN'T DO IT HERE, BECAUSE THE pNode->GetDefaultEntity returns always 0
            // THE ENTITY IS ADJUSTED IN THE SetEntity call to the CFlowGraph
            // finally we have to set the correct entity for this node
            // e.g. GraphEntity has to be updated set to new graph entity
            if (pSrcNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY))
            {
                pNode->SetEntity(0);
                pNode->SetFlag(EHYPER_NODE_GRAPH_ENTITY, true);
            }
            else if (pSrcNode->CheckFlag(EHYPER_NODE_ENTITY))
            {
                CFlowNode* pSrcFlowNode = static_cast<CFlowNode*> (pSrcNode);
                pNode->SetEntity(pSrcFlowNode->GetEntity());
            }
        }

        // and add the node
        pGraph->AddNode(pNewNode);
    }
    pEnum->Release();

    std::vector<CHyperEdge*> edges;
    GetAllEdges(edges);
    for (int i = 0; i < edges.size(); i++)
    {
        CHyperEdge& edge = *(edges[i]);

        CHyperNode* pNodeIn = (CHyperNode*)pGraph->FindNode(edge.nodeIn);
        CHyperNode* pNodeOut = (CHyperNode*)pGraph->FindNode(edge.nodeOut);
        if (!pNodeIn || !pNodeOut)
        {
            continue;
        }

        CHyperNodePort* pPortIn = pNodeIn->FindPort(edge.portIn, true);
        CHyperNodePort* pPortOut = pNodeOut->FindPort(edge.portOut, false);
        if (!pPortIn || !pPortOut)
        {
            continue;
        }

        pGraph->MakeEdge(pNodeOut, pPortOut, pNodeIn, pPortIn, edge.enabled, false);
    }

    pGraph->m_bLoadingNow = false;

    // clone graph tokens
    for (size_t i = 0; i < m_legacyRuntimeFlowGraph->GetGraphTokenCount(); ++i)
    {
        const IFlowGraph::SGraphToken* pToken = m_legacyRuntimeFlowGraph->GetGraphToken(i);
        pGraph->m_legacyRuntimeFlowGraph->AddGraphToken(*pToken);
    }

    OnPostLoad();

    pGraph->SetEnabled(IsEnabled());
    pGraph->SetModified();

    return pGraph;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    if (!m_bEditable)
    {
        m_legacyRuntimeFlowGraph->SerializeXML(node, bLoading);
    }
    else
    {
        // when loading an entity flowgraph assign a name in advance, so
        // error messages are bit more descriptive
        if (bLoading && m_pEntity && m_legacyRuntimeFlowGraph->GetType() != IFlowGraph::eFGT_FlowGraphComponent)
        {
            SetName(m_pEntity->GetName());
        }

        CHyperGraph::Serialize(node, bLoading, ar);
        if (bLoading)
        {
            bool enabled;
            if (node->getAttr("enabled", enabled) == false)
            {
                enabled = true;
            }
            SetEnabled(enabled);

            EMultiPlayerType mpType = eMPT_ServerOnly;
            const char* mpTypeAttr = node->getAttr("MultiPlayer");
            if (mpTypeAttr)
            {
                if (strcmp("ClientOnly", mpTypeAttr) == 0)
                {
                    mpType = eMPT_ClientOnly;
                }
                else if (strcmp("ClientServer", mpTypeAttr) == 0)
                {
                    mpType = eMPT_ClientServer;
                }
            }
            SetMPType(mpType);

            XmlNodeRef hypergraphId = node->findChild("HyperGraphId");
            if (hypergraphId)
            {
                auto id = hypergraphId->getAttr("Id");
                SetHyperGraphId(id);
            }
            else
            {
                SetHyperGraphId(AZ::Uuid::Create());
            }
            m_legacyRuntimeFlowGraph->RemoveGraphTokens();
            XmlNodeRef graphTokens = node->findChild("GraphTokens");
            if (graphTokens)
            {
                int nTokens = graphTokens->getChildCount();
                for (int i = 0; i < nTokens; ++i)
                {
                    XmlString tokenName;
                    int tokenType;

                    XmlNodeRef tokenXml = graphTokens->getChild(i);
                    tokenXml->getAttr("Name", tokenName);
                    tokenXml->getAttr("Type", tokenType);

                    IFlowGraph::SGraphToken token;
                    token.name = tokenName.c_str();
                    token.type = (EFlowDataTypes)tokenType;

                    m_legacyRuntimeFlowGraph->AddGraphToken(token);
                }
            }
        }
        else
        {
            node->setAttr("enabled", IsEnabled());
            EMultiPlayerType mpType = GetMPType();
            if (mpType == eMPT_ClientOnly)
            {
                node->setAttr("MultiPlayer", "ClientOnly");
            }
            else if (mpType == eMPT_ClientServer)
            {
                node->setAttr("MultiPlayer", "ClientServer");
            }
            else
            {
                node->setAttr("MultiPlayer", "ServerOnly");
            }

            XmlNodeRef hypergraphId = node->newChild("HyperGraphId");
            hypergraphId->setAttr("Id", m_hypergraphId);
            XmlNodeRef tokens = node->newChild("GraphTokens");
            size_t gtCount = m_legacyRuntimeFlowGraph->GetGraphTokenCount();
            for (size_t i = 0; i < gtCount; ++i)
            {
                const IFlowGraph::SGraphToken* pToken = m_legacyRuntimeFlowGraph->GetGraphToken(i);
                XmlNodeRef tokenXml = tokens->newChild("Token");

                tokenXml->setAttr("Name", pToken->name);
                tokenXml->setAttr("Type", pToken->type);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraph::Migrate(XmlNodeRef& node)
{
    CFlowGraphMigrationHelper& migHelper = GetIEditor()->GetFlowGraphManager()->GetMigrationHelper();

    bool bChanged = migHelper.Substitute(node);

    const std::vector<CFlowGraphMigrationHelper::ReportEntry>& report = migHelper.GetReport();
    if (report.size() > 0)
    {
        for (int i = 0; i < report.size(); ++i)
        {
            const CFlowGraphMigrationHelper::ReportEntry& entry = report[i];
            CErrorRecord err;
            err.module = VALIDATOR_MODULE_FLOWGRAPH;
            err.severity = CErrorRecord::ESEVERITY_WARNING;
            err.error = entry.description;
            err.pItem = 0;
            GetIEditor()->GetErrorReport()->ReportError(err);
            QByteArray name = GetName().toUtf8();
            CryLogAlways("CFlowGraph::Migrate: FG '%s': %s", name.data(), entry.description.toUtf8().data());
        }
    }
    return bChanged;
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::OnPostLoad()
{
    if (m_legacyRuntimeFlowGraph)
    {
        m_legacyRuntimeFlowGraph->InitializeValues();
        m_legacyRuntimeFlowGraph->PrecacheResources();
    }
}

void CFlowGraph::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    // all flownodes need to have a post clone
    CHyperGraph::PostClone(pFromObject, ctx);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::SetGroupName(const QString& sName)
{
    // we have to override so everybody knows that we changed the group. maybe do this in CHyperGraph instead
    CHyperGraph::SetGroupName(sName);
    GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_OWNER_CHANGE);
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::BeginEditing()
{
    if (!m_bEditable)
    {
        m_bEditable = true;
        InitializeFromGame();
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::EndEditing()
{
    m_bEditable = false;
    CHyperGraph::ClearAll();
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::SetEnabled(bool bEnable)
{
    CHyperGraph::SetEnabled(bEnable);
    // Enable/Disable game flow graph.
    if (m_legacyRuntimeFlowGraph)
    {
        m_legacyRuntimeFlowGraph->SetEnabled(bEnable);
    }
}

//////////////////////////////////////////////////////////////////////////
void CFlowGraph::InitializeFromGame()
{
}

//////////////////////////////////////////////////////////////////////////
IUndoObject* CFlowGraph::CreateUndo()
{
    // create undo object
    return new CUndoFlowGraph(this);
}

//////////////////////////////////////////////////////////////////////////
bool CFlowGraph::CanConnectPorts(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, CHyperEdge* pExistingEdge)
{
    // Target node cannot be a track event node
    if (pTrgNode->GetClassName() == TRACKEVENTNODE_CLASS)
    {
        return false;
    }

    if (pTrgNode->GetClassName() == MISSING_NODE_CLASS || pSrcNode->GetClassName() == MISSING_NODE_CLASS)
    {
        Warning("Trying to link a missing node!");
        return false;
    }

    bool bCanConnect = CHyperGraph::CanConnectPorts(pSrcNode, pSrcPort, pTrgNode, pTrgPort, pExistingEdge);
    if (bCanConnect && m_legacyRuntimeFlowGraph)
    {
        CFlowNode* pSrcFlowNode = static_cast<CFlowNode*> (pSrcNode);
        TFlowNodeId srcId = pSrcFlowNode->GetFlowNodeId();
        SFlowNodeConfig srcConfig;
        m_legacyRuntimeFlowGraph->GetNodeConfiguration(srcId, srcConfig);

        CFlowNode* pTrgFlowNode = static_cast<CFlowNode*> (pTrgNode);
        TFlowNodeId trgId = pTrgFlowNode->GetFlowNodeId();
        SFlowNodeConfig trgConfig;
        m_legacyRuntimeFlowGraph->GetNodeConfiguration(trgId, trgConfig);

        if (srcConfig.pOutputPorts && trgConfig.pInputPorts)
        {
            const SOutputPortConfig* pSrcPortConfig = srcConfig.pOutputPorts + pSrcPort->nPortIndex;
            const SInputPortConfig* pTrgPortConfig = trgConfig.pInputPorts + pTrgPort->nPortIndex;
            int srcType = pSrcPortConfig->type;
            int trgType = pTrgPortConfig->defaultData.GetType();
            if (srcType == eFDT_EntityId)
            {
                bCanConnect = true;
            }
            else if (trgType == eFDT_EntityId)
            {
                bCanConnect = (srcType == eFDT_EntityId || srcType == eFDT_Int || srcType == eFDT_Any);
            }
            if (!bCanConnect)
            {
                Warning("An Entity port can only be connected from an Entity or an Int port!");
            }
        }
    }
    return bCanConnect;
}

void CFlowGraph::ValidateEdges(CHyperNode* pNode)
{
    std::vector<CHyperEdge*> edges;
    if (FindEdges(pNode, edges))
    {
        for (int i = 0; i < edges.size(); i++)
        {
            CHyperNode* nodeIn = (CHyperNode*)FindNode(edges[i]->nodeIn);
            if (nodeIn)
            {
                CHyperNodePort* portIn = nodeIn->FindPort(edges[i]->portIn, true);
                if (!portIn)
                {
                    RemoveEdge(edges[i]);
                }
            }
            CHyperNode* nodeOut = (CHyperNode*)FindNode(edges[i]->nodeOut);
            if (nodeOut)
            {
                CHyperNodePort* portOut = nodeOut->FindPort(edges[i]->portOut, false);
                if (!portOut)
                {
                    RemoveEdge(edges[i]);
                }
            }
        }
    }
}

CFlowNode* CFlowGraph::FindFlowNode(TFlowNodeId id) const
{
    NodesMap::const_iterator it = m_nodesMap.begin();
    NodesMap::const_iterator end = m_nodesMap.end();
    for (; it != end; ++it)
    {
        CFlowNode* pNode = static_cast<CFlowNode*>(it->second.get());
        TFlowNodeId nodeID = pNode->GetFlowNodeId();
        if (id == nodeID)
        {
            return pNode;
        }
    }

    return NULL;
}

void CFlowGraph::ExtractObjectsPrefabIdToGlobalIdMappingFromPrefab(CObjectArchive& archive)
{
    if (CEntityObject* pEntity = GetEntity())
    {
        if (CPrefabObject* pPrefabObject = pEntity->GetPrefab())
        {
            TBaseObjects childs;
            pPrefabObject->GetAllPrefabFlagedChildren(childs);

            for (int i = 0, count = childs.size(); i < count; ++i)
            {
                archive.RemapID(childs[i]->GetIdInPrefab(), childs[i]->GetId());
            }
        }
    }
}

void CFlowGraph::OnFlowNodeActivated(IFlowGraphPtr pFlowGraph, TFlowNodeId srcNode, TFlowPortId srcPort, TFlowNodeId toNode, TFlowPortId toPort, const char* value)
{
    OnFlowNodeActivation(pFlowGraph, srcNode, srcPort, toNode, toPort, value);
}

void CFlowGraph::BindToFlowgraph(IFlowGraphPtr pFlowGraph)
{
    ComponentFlowgraphRuntimeNotificationBus::Handler::BusDisconnect();
    m_componentEntityRuntimeFlowgraph = pFlowGraph;
    ComponentFlowgraphRuntimeNotificationBus::Handler::BusConnect(m_componentEntityRuntimeFlowgraph->GetGraphId());
}

void CFlowGraph::UnBindFromFlowgraph()
{
    ComponentFlowgraphRuntimeNotificationBus::Handler::BusDisconnect();
    m_componentEntityRuntimeFlowgraph = nullptr;
    ComponentFlowgraphRuntimeNotificationBus::Handler::BusConnect(m_legacyRuntimeFlowGraph->GetGraphId());
}

void CFlowGraph::SetHyperGraphId(const AZ::Uuid& id)
{
    ComponentHyperGraphRequestsBus::Handler::BusDisconnect();

    m_hypergraphId = id;

    if (m_legacyRuntimeFlowGraph)
    {
        m_legacyRuntimeFlowGraph->SetControllingHyperGraphId(m_hypergraphId);
    }

    ComponentHyperGraphRequestsBus::Handler::BusConnect(m_hypergraphId);
}
