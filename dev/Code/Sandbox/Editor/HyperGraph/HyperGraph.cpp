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
#include "HyperGraph.h"
#include "HyperGraphManager.h"
#include "GameEngine.h"
#include "MissingNode.h"

#undef GetClassName

/*
//////////////////////////////////////////////////////////////////////////
// Undo object for CHyperNode.
class CUndoHyperNodeCreate : public IUndoObject
{
public:
    CUndoHyperNodeCreate( CHyperNode *node )
    {
        // Stores the current state of this object.
        assert( node != 0 );
        m_node = node;
        m_redo = 0;
        m_pGraph = node->GetGraph();
        m_node = node;
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual const char* GetDescription() { return "HyperNodeUndoCreate"; };

    virtual void Undo( bool bUndo )
    {
        if (bUndo)
        {
            if (m_node)
            {
                m_redo = XmlHelpers::CreateXmlNode("Redo");
                CHyperGraphSerializer serializer(m_pGraph);
                serializer.SaveNode(m_node,true);
                serializer.Serialize( m_redo,false );
            }
        }
        // Undo object state.
        if (m_node)
        {
            m_node->Invalidate();
            m_pGraph->RemoveNode( m_node );
        }
        m_node = 0;
    }
    virtual void Redo()
    {
        if (m_redo)
        {
            CHyperGraphSerializer serializer(m_pGraph);
            serializer.SelectLoaded(true);
            serializer.Serialize( m_redo,true );
            m_node = serializer.GetFirstNode();
        }
    }
private:
    _smart_ptr<CHyperGraph> m_pGraph;
    _smart_ptr<CHyperNode> m_node;
    XmlNodeRef m_redo;
};
*/

//////////////////////////////////////////////////////////////////////////
CHyperGraphSerializer::CHyperGraphSerializer(CHyperGraph* pGraph, CObjectArchive* ar)
{
    m_pGraph = pGraph;
    m_pAR = ar;
    m_bLoading = false;
    m_bSelectLoaded = false;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphSerializer::SaveNode(CHyperNode* pNode, bool bSaveEdges)
{
    m_nodes.insert(pNode);
    if (bSaveEdges)
    {
        std::vector<CHyperEdge*> edges;
        if (m_pGraph->FindEdges(pNode, edges))
        {
            for (int i = 0; i < edges.size(); i++)
            {
                SaveEdge(edges[i]);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphSerializer::SaveEdge(CHyperEdge* pEdge)
{
    m_edges.insert(pEdge);
}

namespace {
    struct sortNodesById
    {
        bool operator()(const THyperNodePtr& lhs, const THyperNodePtr& rhs) const
        {
            return lhs->GetId() < rhs->GetId();
        }
    };

    struct sortEdges
    {
        bool operator() (const _smart_ptr<CHyperEdge>& lhs, const _smart_ptr<CHyperEdge>& rhs) const
        {
            if (lhs->nodeOut < rhs->nodeOut)
            {
                return true;
            }
            else if (lhs->nodeOut > rhs->nodeOut)
            {
                return false;
            }
            else if (lhs->nodeIn < rhs->nodeIn)
            {
                return true;
            }
            else if (lhs->nodeIn > rhs->nodeIn)
            {
                return false;
            }
            else if (lhs->portOut < rhs->portOut)
            {
                return true;
            }
            else if (lhs->portOut > rhs->portOut)
            {
                return false;
            }
            else if (lhs->portIn < rhs->portIn)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
    };
};

//////////////////////////////////////////////////////////////////////////
struct NodeComparator
{
    /**
    * Since the binary_search is looking for a particular id in a std::set of THyperNodePtr
    * sorted according to their HyperNodeID, two comparators need to be provided one for each
    * way the comparision happens.
    */

    bool operator()(const THyperNodePtr& lhs, const HyperNodeID rhs) const
    {
        return lhs->GetId() < rhs;
    }

    bool operator()(const HyperNodeID lhs, const THyperNodePtr& rhs) const
    {
        return lhs < rhs->GetId();
    }
};

//////////////////////////////////////////////////////////////////////////
bool CHyperGraphSerializer::Serialize(XmlNodeRef& node,
    bool bLoading,
    bool bLoadEdges,
    bool bIsPaste,
    bool isExport)
{
    static ICVar* pCVarFlowGraphWarnings = gEnv->pConsole->GetCVar("g_display_fg_warnings");

    bool bHasChanges = false;
    m_bLoading = bLoading;
    if (!m_bLoading)
    {
        // Save
        node->setAttr("Description", m_pGraph->GetDescription().toUtf8().data());
        node->setAttr("Group", m_pGraph->GetGroupName().toUtf8().data());

        // sort nodes by their IDs to make a pretty output file (suitable for merging)
        typedef std::set<THyperNodePtr, sortNodesById > TSortedNodesSet;
        TSortedNodesSet sortedNodes (m_nodes.begin(), m_nodes.end());

        // sizes must match otherwise we had ID duplicates which is VERY BAD
        assert (sortedNodes.size() == m_nodes.size());

        // sort edges (don't use a set, because then our compare operation must be unique)
        typedef std::vector<_smart_ptr<CHyperEdge> > TEdgeVec;
        TEdgeVec sortedEdges (m_edges.begin(), m_edges.end());
        std::sort (sortedEdges.begin(), sortedEdges.end(), sortEdges());

        // Serialize nodes and edges into the xml.
        XmlNodeRef nodesXml = node->newChild("Nodes");
        for (TSortedNodesSet::iterator nit = sortedNodes.begin(); nit != sortedNodes.end(); ++nit)
        {
            CHyperNode* pNode = *nit;
            XmlNodeRef nodeXml = nodesXml->newChild("Node");
            nodeXml->setAttr("Id", pNode->GetId());
            pNode->Serialize(nodeXml, bLoading, m_pAR);
        }

        XmlNodeRef edgesXml = node->newChild("Edges");
        for (TEdgeVec::iterator eit = sortedEdges.begin(); eit != sortedEdges.end(); ++eit)
        {
            CHyperEdge* pEdge = *eit;

            // If this is an export operation, verify the fact that both the input and output nodes are in the selection
            if (isExport)
            {
                NodeComparator comparator;

                if (!std::binary_search(
                        sortedNodes.begin(), sortedNodes.end(), pEdge->nodeIn, comparator))
                {
                    continue;
                }

                if (!std::binary_search(
                        sortedNodes.begin(), sortedNodes.end(), pEdge->nodeOut, comparator))
                {
                    continue;
                }
            }

            XmlNodeRef edgeXml = edgesXml->newChild("Edge");
            edgeXml->setAttr("nodeIn", pEdge->nodeIn);
            edgeXml->setAttr("nodeOut", pEdge->nodeOut);
            edgeXml->setAttr("portIn", pEdge->portIn.toUtf8().data());
            edgeXml->setAttr("portOut", pEdge->portOut.toUtf8().data());
            edgeXml->setAttr("enabled", pEdge->enabled);
        }
    }
    else
    {
        // Load
        m_pGraph->RecordUndo();
        GetIEditor()->SuspendUndo();

        m_pGraph->m_bLoadingNow = true;

        CHyperGraphManager* pManager = m_pGraph->GetManager();

        if (!bIsPaste)
        {
            m_pGraph->SetDescription(node->getAttr("Description"));
            m_pGraph->SetGroupName(node->getAttr("Group"));
        }

        if (!bIsPaste && gSettings.bFlowGraphMigrationEnabled)
        {
            bHasChanges = m_pGraph->Migrate(node);
        }

        XmlNodeRef nodesXml = node->findChild("Nodes");
        if (nodesXml)
        {
            HyperNodeID nodeId;
            for (int i = 0; i < nodesXml->getChildCount(); i++)
            {
                XmlNodeRef nodeXml = nodesXml->getChild(i);
                QString nodeclass;
                if (!nodeXml->getAttr("Class", nodeclass))
                {
                    continue;
                }
                if (!nodeXml->getAttr("Id", nodeId))
                {
                    continue;
                }

                // Check if node id is already occupied.
                if (m_pGraph->FindNode(nodeId))
                {
                    // If taken, allocate new id, and remap old id.
                    HyperNodeID newId = m_pGraph->AllocateId();
                    RemapId(nodeId, newId);
                    nodeId = newId;
                }

                CHyperNode* pNode = pManager->CreateNode(m_pGraph, nodeclass.toUtf8().data(), nodeId, QPointF(0.f, 0.f), NULL, !bIsPaste);
                if (!pNode)
                {
                    continue;
                }
                pNode->Serialize(nodeXml, bLoading, m_pAR);

                // Check for any input values on this node
                XmlNodeRef nodeInputs = nodeXml->findChild("Inputs");
                if (nodeInputs)
                {
                    // If there are existing input values, cross reference them with the ports on this node
                    for (int i = 0; i < nodeInputs->getNumAttributes(); i++)
                    {
                        const char* key;
                        const char* value;
                        nodeInputs->getAttributeByIndex(i, &key, &value);

                        // If a value that is not currently a port is found, report a warning to the user
                        if (pNode->FindPort(key, true) == nullptr)
                        {
                            gEnv->pLog->LogWarning("Missing Port: [%s] with Value [%s] On Node [%s], Referenced in FlowGraph [%s]. Saving this level may result in loss of data.",
                                key,
                                value,
                                pNode->GetClassName().toUtf8().data(),
                                m_pGraph->GetName().toUtf8().data());
                        }
                    }
                }
                m_pGraph->AddNode(pNode);

                m_nodes.insert(pNode);

                if (m_bSelectLoaded)
                {
                    pNode->SetSelected(true);
                }
            }
        }

        if (bLoadEdges)
        {
            XmlNodeRef edgesXml = node->findChild("Edges");
            if (edgesXml)
            {
                HyperNodeID nodeIn, nodeOut;
                QString portIn, portOut;
                bool bEnabled;
                for (int i = 0; i < edgesXml->getChildCount(); i++)
                {
                    XmlNodeRef edgeXml = edgesXml->getChild(i);

                    edgeXml->getAttr("nodeIn", nodeIn);
                    edgeXml->getAttr("nodeOut", nodeOut);
                    edgeXml->getAttr("portIn", portIn);
                    edgeXml->getAttr("portOut", portOut);
                    if (edgeXml->getAttr("enabled", bEnabled) == false)
                    {
                        bEnabled = true;
                    }

                    nodeIn = GetId(nodeIn);
                    nodeOut = GetId(nodeOut);

                    CHyperNode* pNodeIn = (CHyperNode*)m_pGraph->FindNode(nodeIn);
                    CHyperNode* pNodeOut = (CHyperNode*)m_pGraph->FindNode(nodeOut);
                    if (!pNodeIn || !pNodeOut)
                    {
                        QString cause;
                        if (!pNodeIn && !pNodeOut)
                        {
                            cause = QStringLiteral("source node %1 and target node %2 do not exist").arg(nodeOut, nodeIn);
                        }
                        else if (!pNodeIn)
                        {
                            cause = QStringLiteral("target node %1 does not exist").arg(nodeIn);
                        }
                        else if (!pNodeOut)
                        {
                            cause = QStringLiteral("source node %1 does not exist").arg(nodeOut);
                        }

                        CErrorRecord err;
                        err.module = VALIDATOR_MODULE_FLOWGRAPH;
                        err.severity = CErrorRecord::ESEVERITY_ERROR;
                        err.error = QStringLiteral("Loading Graph '%1': Can't connect edge <%2,%3> -> <%4,%5> because %6")
                            .arg(m_pGraph->GetName()).arg(nodeOut).arg(portOut).arg(nodeIn).arg(portIn)
                            .arg(cause);
                        err.pItem = 0;
                        GetIEditor()->GetErrorReport()->ReportError(err);

                        if (!pCVarFlowGraphWarnings || (pCVarFlowGraphWarnings && pCVarFlowGraphWarnings->GetIVal() == 1))
                        {
                            Warning("Loading Graph '%s': Can't connect edge <%d,%s> -> <%d,%s> because %s",
                                m_pGraph->GetName().toUtf8().data(), nodeOut, portOut.toUtf8().data(), nodeIn, portIn.toUtf8().data(),
                                cause.toUtf8().data());
                        }
                        continue;
                    }
                    CHyperNodePort* pPortIn = pNodeIn->FindPort(portIn, true);
                    CHyperNodePort* pPortOut = pNodeOut->FindPort(portOut, false);

                    if (!bIsPaste)
                    {
                        if (!pPortIn)
                        {
                            pPortIn = pNodeIn->CreateMissingPort(portIn.toUtf8().data(), true);
                            gEnv->pLog->LogError("Missing Port: %s, Referenced in FlowGraph %s, on node %s", portIn.toUtf8().data(), m_pGraph->GetName().toUtf8().constData(), pNodeIn->GetClassName().toUtf8().constData());
                        }

                        if (!pPortOut)
                        {
                            pPortOut = pNodeOut->CreateMissingPort(portOut.toUtf8().data(), false);
                            gEnv->pLog->LogError("Missing Port: %s, Referenced in FlowGraph %s, on node %s", portOut.toUtf8().data(), m_pGraph->GetName().toUtf8().constData(), pNodeOut->GetClassName().toUtf8().constData());
                        }
                    }

                    if (!pPortIn || !pPortOut)
                    {
                        QString cause;
                        if (!pPortIn && !pPortOut)
                        {
                            cause = QStringLiteral("source port '%1' and target port '%2' do not exist").arg(portOut, portIn);
                        }
                        else if (!pPortIn)
                        {
                            cause = QStringLiteral("target port '%1' does not exist").arg(portIn);
                        }
                        else if (!pPortOut)
                        {
                            cause = QStringLiteral("source port '%1' does not exist").arg(portOut);
                        }

                        CErrorRecord err;
                        err.module = VALIDATOR_MODULE_FLOWGRAPH;
                        err.severity = CErrorRecord::ESEVERITY_ERROR;
                        err.error = QStringLiteral("Loading Graph '%1': Can't connect edge <%2,%3> -> <%4,%5> because %6")
                            .arg(m_pGraph->GetName()).arg(nodeOut).arg(portOut).arg(nodeIn).arg(portIn)
                            .arg(cause);
                        err.pItem = 0;
                        GetIEditor()->GetErrorReport()->ReportError(err);

                        if (!pCVarFlowGraphWarnings || (pCVarFlowGraphWarnings && pCVarFlowGraphWarnings->GetIVal() == 1))
                        {
                            Warning("Loading Graph '%s': Can't connect edge <%d,%s> -> <%d,%s> because %s",
                                m_pGraph->GetName().toUtf8().data(), nodeOut, portOut.toUtf8().data(), nodeIn, portIn.toUtf8().data(),
                                cause.toUtf8().data());
                        }
                        continue;
                    }

                    if (!bIsPaste || (pPortIn->bAllowMulti == false && m_pGraph->FindEdge(pNodeIn, pPortIn) == 0))
                    {
                        const bool isDuplicateEdge = m_pGraph->HasEdge(pNodeOut, pPortOut, pNodeIn, pPortIn);
                        if (!isDuplicateEdge)
                        {
                            m_pGraph->MakeEdge(pNodeOut, pPortOut, pNodeIn, pPortIn, bEnabled, false);
                        }
                    }
                }
            }
        }
        m_pGraph->m_bLoadingNow = false;
        // m_pGraph->SendNotifyEvent( 0,EHG_GRAPH_INVALIDATE );
        m_pGraph->OnPostLoad();

        GetIEditor()->ResumeUndo();
    }
    return bHasChanges;
}

//////////////////////////////////////////////////////////////////////////
HyperNodeID CHyperGraphSerializer::GetId(HyperNodeID id)
{
    id = stl::find_in_map(m_remap, id, id);
    return id;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphSerializer::RemapId(HyperNodeID oldId, HyperNodeID newId)
{
    m_remap[oldId] = newId;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphSerializer::GetLoadedNodes(std::vector<CHyperNode*>& nodes) const
{
    stl::set_to_vector(m_nodes, nodes);
}

//////////////////////////////////////////////////////////////////////////
CHyperNode* CHyperGraphSerializer::GetFirstNode()
{
    if (m_nodes.empty())
    {
        return 0;
    }
    return *m_nodes.begin();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Undo object for CHyperGraph.
//////////////////////////////////////////////////////////////////////////
class CUndoHyperGraph
    : public IUndoObject
{
public:
    CUndoHyperGraph(CHyperGraph* pGraph)
    {
        // Stores the current state of given graph.
        assert(pGraph != 0);
        m_pGraph = pGraph;
        m_redo = 0;
        m_undo = XmlHelpers::CreateXmlNode("HyperGraph");
        m_pGraph->Serialize(m_undo, false);
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "HyperNodeUndoCreate"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_redo = XmlHelpers::CreateXmlNode("HyperGraph");
            m_pGraph->Serialize(m_redo, false);
        }
        m_pGraph->Serialize(m_undo, true);
    }
    virtual void Redo()
    {
        if (m_redo)
        {
            m_pGraph->Serialize(m_redo, true);
        }
    }
private:
    _smart_ptr<CHyperGraph> m_pGraph;
    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
};


//////////////////////////////////////////////////////////////////////////
template <class TMap>
class CHyperGraphNodeEnumerator
    : public IHyperGraphEnumerator
{
    TMap* m_pMap;
    typename TMap::iterator m_iterator;

public:
    CHyperGraphNodeEnumerator(TMap* pMap)
    {
        assert(pMap);
        m_pMap = pMap;
        m_iterator = m_pMap->begin();
    }
    virtual void Release() { delete this; };
    virtual IHyperNode* GetFirst()
    {
        m_iterator = m_pMap->begin();
        if (m_iterator == m_pMap->end())
        {
            return 0;
        }
        return m_iterator->second;
    }
    virtual IHyperNode* GetNext()
    {
        if (m_iterator != m_pMap->end())
        {
            m_iterator++;
        }
        if (m_iterator == m_pMap->end())
        {
            return 0;
        }
        return m_iterator->second;
    }
};

//////////////////////////////////////////////////////////////////////////
IHyperGraphEnumerator* CHyperGraph::GetNodesEnumerator()
{
    return new CHyperGraphNodeEnumerator<NodesMap>(&m_nodesMap);
}

//////////////////////////////////////////////////////////////////////////
CHyperGraph::CHyperGraph(CHyperGraphManager* pManager)
{
    m_bEnabled = true;
    m_bModified = false;
    m_bLoadingNow = false;
    m_iMissingNodes = 0;
    m_iMissingPorts = 0;
    m_pManager = pManager;
    m_name = "Default";

    m_viewOffset = QPoint(0, 0);
    m_fViewZoom = 1;
    m_bViewPosInitialized = false;

    m_nextNodeId = 1;
}

//////////////////////////////////////////////////////////////////////////
CHyperGraph::~CHyperGraph()
{
    m_pManager = 0;
    SendNotifyEvent(0, EHG_GRAPH_REMOVED);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::SetModified(bool bModified)
{
    if (bModified)
    {
        UpdateMissingCount();
    }
    m_bModified = bModified;
}

//////////////////////////////////////////////////////////////////////////
int CHyperGraph::GetMissingNodesCount() const
{
    return m_iMissingNodes;
}

//////////////////////////////////////////////////////////////////////////
int CHyperGraph::GetMissingPortsCount() const
{
    return m_iMissingPorts;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::UpdateMissingCount()
{
    m_iMissingPorts = 0;
    m_iMissingNodes = 0;
    for (NodesMap::const_iterator it = m_nodesMap.begin(); it != m_nodesMap.end(); ++it)
    {
        if (it->second->GetClassName() == MISSING_NODE_CLASS)
        {
            ++m_iMissingNodes;
        }
        m_iMissingPorts += it->second->GetMissingPortCount();
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::AddListener(IHyperGraphListener* pListener)
{
    stl::push_back_unique(m_listeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::RemoveListener(IHyperGraphListener* pListener)
{
    stl::find_and_erase(m_listeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::SendNotifyEvent(IHyperNode* pNode, EHyperGraphEvent event)
{
    Listeners::iterator next;
    for (Listeners::iterator it = m_listeners.begin(); it != m_listeners.end(); it = next)
    {
        next = it;
        next++;
        (*it)->OnHyperGraphEvent(pNode, event);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::InvalidateNode(IHyperNode* pNode, bool bModified /* = true*/)
{
    SendNotifyEvent(pNode, EHG_NODE_CHANGE);

    if (bModified)
    {
        SetModified();
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::AddNode(CHyperNode* pNode)
{
    if (!pNode)
    {
        return;
    }

    HyperNodeID id = pNode->GetId();
    m_nodesMap[id] = pNode;
    if (id >= m_nextNodeId)
    {
        m_nextNodeId = id + 1;
    }
    SendNotifyEvent(pNode, EHG_NODE_ADD);
    GetManager()->SendNotifyEvent(EHG_NODE_ADD, this, pNode);
    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::RemoveNode(IHyperNode* pNode)
{
    if (!pNode)
    {
        return;
    }

    SendNotifyEvent(pNode, EHG_NODE_DELETE);
    GetManager()->SendNotifyEvent(EHG_NODE_DELETE, 0, pNode);

    // Delete all connected edges.
    std::vector<CHyperEdge*> edges;
    if (FindEdges((CHyperNode*)pNode, edges))
    {
        for (int i = 0; i < edges.size(); i++)
        {
            RemoveEdge(edges[i]);
        }
    }
    ((CHyperNode*)pNode)->Done();
    m_nodesMap.erase(pNode->GetId());
    SendNotifyEvent(pNode, EHG_NODE_DELETED);
    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::RemoveNodeKeepLinks(IHyperNode* pNode)
{
    if (!pNode)
    {
        return;
    }

    SendNotifyEvent(pNode, EHG_NODE_DELETE);
    GetManager()->SendNotifyEvent(EHG_NODE_DELETE, 0, pNode);

    ((CHyperNode*)pNode)->Done();
    m_nodesMap.erase(pNode->GetId());
    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::RemoveEdge(CHyperEdge* pEdge)
{
    if (!pEdge)
    {
        return;
    }

    CHyperNode* nodeIn = (CHyperNode*)FindNode(pEdge->nodeIn);
    if (nodeIn)
    {
        CHyperNodePort* portIn = nodeIn->FindPort(pEdge->portIn, true);
        if (portIn)
        {
            --portIn->nConnected;
        }
        nodeIn->Unlinked(true);
    }
    CHyperNode* nodeOut = (CHyperNode*)FindNode(pEdge->nodeOut);
    if (nodeOut)
    {
        CHyperNodePort* portOut = nodeOut->FindPort(pEdge->portOut, false);
        if (portOut)
        {
            --portOut->nConnected;
        }
        nodeOut->Unlinked(false);
    }

    // Remove NodeIn link.
    EdgesMap::iterator it = m_edgesMap.lower_bound(pEdge->nodeIn);
    while (it != m_edgesMap.end() && it->first == pEdge->nodeIn)
    {
        if (it->second == pEdge)
        {
            m_edgesMap.erase(it);
            break;
        }
        it++;
    }
    // Remove NodeOut link.
    it = m_edgesMap.lower_bound(pEdge->nodeOut);
    while (it != m_edgesMap.end() && it->first == pEdge->nodeOut)
    {
        if (it->second == pEdge)
        {
            m_edgesMap.erase(it);
            break;
        }
        it++;
    }

    SendNotifyEvent(0, EHG_GRAPH_INVALIDATE);
    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::RemoveEdgeSilent(CHyperEdge* pEdge)
{
    if (!pEdge)
    {
        return;
    }

    CHyperNode* nodeIn = (CHyperNode*)FindNode(pEdge->nodeIn);
    if (nodeIn)
    {
        CHyperNodePort* portIn = nodeIn->FindPort(pEdge->portIn, true);
        if (portIn)
        {
            --portIn->nConnected;
        }
        nodeIn->Unlinked(true);
    }
    CHyperNode* nodeOut = (CHyperNode*)FindNode(pEdge->nodeOut);
    if (nodeOut)
    {
        CHyperNodePort* portOut = nodeOut->FindPort(pEdge->portOut, false);
        if (portOut)
        {
            --portOut->nConnected;
        }
        nodeOut->Unlinked(false);
    }

    // Remove NodeIn link.
    EdgesMap::iterator it = m_edgesMap.lower_bound(pEdge->nodeIn);
    while (it != m_edgesMap.end() && it->first == pEdge->nodeIn)
    {
        if (it->second == pEdge)
        {
            m_edgesMap.erase(it);
            break;
        }
        it++;
    }
    // Remove NodeOut link.
    it = m_edgesMap.lower_bound(pEdge->nodeOut);
    while (it != m_edgesMap.end() && it->first == pEdge->nodeOut)
    {
        if (it->second == pEdge)
        {
            m_edgesMap.erase(it);
            break;
        }
        it++;
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::ClearAll()
{
    m_nodesMap.clear();
    m_edgesMap.clear();
    m_iMissingNodes = 0;
    m_iMissingPorts = 0;

    SendNotifyEvent(NULL, EHG_GRAPH_CLEAR_ALL);
}

//////////////////////////////////////////////////////////////////////////
CHyperEdge* CHyperGraph::CreateEdge()
{
    return new CHyperEdge();
}

//////////////////////////////////////////////////////////////////////////
IHyperNode* CHyperGraph::CreateNode(const char* sNodeClass, const QPointF& pos)
{
    CHyperNode* pNode = NULL;
    if (strcmp(sNodeClass, "selected_entity") == 0)
    {
        QPointF curPos = pos;
        CSelectionGroup* pSelection = GetIEditor()->GetSelection();
        for (int o = 0; o < pSelection->GetCount() && o < 20; ++o)
        {
            pNode = m_pManager->CreateNode(this, sNodeClass, AllocateId(), pos, pSelection->GetObject(o));
            if (pNode)
            {
                AddNode(pNode);
                pNode->SetPos(curPos);
                curPos += QPointF(0.f, 25.0f);
            }
        }
    }
    else
    {
        pNode = m_pManager->CreateNode(this, sNodeClass, AllocateId(), pos);
        if (pNode)
        {
            AddNode(pNode);
            pNode->SetPos(pos);
        }
    }
    return pNode;
}

//////////////////////////////////////////////////////////////////////////
IHyperNode* CHyperGraph::CloneNode(IHyperNode* pFromNode)
{
    if (!pFromNode)
    {
        return NULL;
    }

    CHyperNode* pNode = ((CHyperNode*)pFromNode)->Clone();
    pNode->m_id = AllocateId();
    AddNode(pNode);
    SetModified();
    return pNode;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx)
{
    for (NodesMap::iterator it = m_nodesMap.begin(); it != m_nodesMap.end(); ++it)
    {
        CHyperNode* pNode = it->second;
        pNode->PostClone(pFromObject, ctx);
    }
}

//////////////////////////////////////////////////////////////////////////
IHyperNode* CHyperGraph::FindNode(HyperNodeID id) const
{
    return stl::find_in_map(m_nodesMap, id, NULL);
}


//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::IsSelectedAny()
{
    for (NodesMap::iterator it = m_nodesMap.begin(); it != m_nodesMap.end(); ++it)
    {
        CHyperNode* pNode = it->second;
        if (pNode->IsSelected())
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::UnselectAll()
{
    for (NodesMap::iterator it = m_nodesMap.begin(); it != m_nodesMap.end(); ++it)
    {
        CHyperNode* pNode = it->second;
        if (pNode->IsSelected())
        {
            pNode->SetSelected(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::HasEdge(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, CHyperEdge* pExistingEdge)
{
    assert(pSrcNode);
    assert(pSrcPort);
    assert(pTrgNode);
    assert(pTrgPort);

    HyperNodeID nSrcNodeId = pSrcNode->GetId();
    HyperNodeID nTrgNodeId = pTrgNode->GetId();

    EdgesMap::iterator it = m_edgesMap.lower_bound(nSrcNodeId);
    while (it != m_edgesMap.end() && it->first == nSrcNodeId)
    {
        CHyperEdge* pEdge = it->second;

        // Following parameter must match in order to consider this a duplicate.
        // 1. Edge's outnode == pSrcNode
        // 2. Edge's innode == pTrgNode
        // 3. Edge's outport == pSrcNode's outport
        // 4. Edge's inport == pTrgNode's inport
        if (pEdge->nodeOut  == nSrcNodeId &&
            pEdge->nodeIn   == nTrgNodeId &&
            pEdge->nPortOut == pSrcPort->nPortIndex &&
            pEdge->nPortIn  == pTrgPort->nPortIndex)
        {
            pExistingEdge = pEdge;
            return true;
        }
        ++it;
    }

    pExistingEdge = NULL;
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::CanConnectPorts(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, CHyperEdge* pExistingEdge)
{
    assert(pSrcNode);
    assert(pSrcPort);
    assert(pTrgNode);
    assert(pTrgPort);
    if (!pSrcPort)
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Unable to connect output of node '%s' ('%s')", pSrcNode->GetName(), pSrcNode->GetDescription());
        return false;
    }
    if (pSrcPort->pVar->GetType() != pTrgPort->pVar->GetType())
    {
        // All types can be connected?
        //return false;
    }
    if (pSrcPort->bInput == pTrgPort->bInput)
    {
        return false;
    }
    if (pSrcNode == pTrgNode)
    {
        return false;
    }

    const bool isDuplicateEdge = HasEdge(pSrcNode, pSrcPort, pTrgNode, pTrgPort, pExistingEdge);
    if (isDuplicateEdge)
    {
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::GetAllEdges(std::vector<CHyperEdge*>& edges) const
{
    edges.clear();
    edges.reserve(m_edgesMap.size());
    for (EdgesMap::const_iterator it = m_edgesMap.begin(); it != m_edgesMap.end(); ++it)
    {
        edges.push_back(it->second);
    }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
    return !edges.empty();
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::FindEdges(CHyperNode* pNode, std::vector<CHyperEdge*>& edges) const
{
    if (!pNode)
    {
        return false;
    }

    edges.clear();

    HyperNodeID nodeId = pNode->GetId();
    EdgesMap::const_iterator it = m_edgesMap.lower_bound(nodeId);
    while (it != m_edgesMap.end() && it->first == nodeId)
    {
        edges.push_back(it->second);
        it++;
    }

    return !edges.empty();
}

//////////////////////////////////////////////////////////////////////////
CHyperEdge* CHyperGraph::FindEdge(CHyperNode* pNode, CHyperNodePort* pPort) const
{
    HyperNodeID nodeId = pNode->GetId();
    QString portname = pPort->GetName();
    EdgesMap::const_iterator it = m_edgesMap.lower_bound(nodeId);
    while (it != m_edgesMap.end() && it->first == nodeId)
    {
        if (pPort->bInput)
        {
            if (it->second->nodeIn == nodeId && it->second->portIn.compare(portname, Qt::CaseInsensitive) == 0)
            {
                return it->second;
            }
        }
        else
        {
            if (it->second->nodeOut == nodeId && it->second->portOut.compare(portname, Qt::CaseInsensitive) == 0)
            {
                return it->second;
            }
        }
        it++;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::ConnectPorts(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, bool specialDrag)
{
    if (!CanConnectPorts(pSrcNode, pSrcPort, pTrgNode, pTrgPort))
    {
        return false;
    }

    CHyperNode* nodeIn = pTrgNode;
    CHyperNode* nodeOut = pSrcNode;
    CHyperNodePort* portIn = pTrgPort;
    CHyperNodePort* portOut = pSrcPort;
    if (pSrcPort->bInput)
    {
        nodeIn = pSrcNode;
        nodeOut = pTrgNode;
        portIn = pSrcPort;
        portOut = pTrgPort;
    }

    // check if input port already connected.
    if (!portIn->bAllowMulti)
    {
        CHyperEdge* pOldEdge = FindEdge(nodeIn, portIn);
        if (pOldEdge)
        {
            // Disconnect old edge.
            RemoveEdge(pOldEdge);
        }
    }

    if (!portOut->bAllowMulti)
    {
        CHyperEdge* pOldEdge = FindEdge(nodeOut, portOut);
        if (pOldEdge)
        {
            RemoveEdge(pOldEdge);
        }
    }

    MakeEdge(nodeOut, portOut, nodeIn, portIn, true, specialDrag);
    SendNotifyEvent(0, EHG_GRAPH_INVALIDATE);

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::RegisterEdge(CHyperEdge* pEdge, bool fromSpecialDrag)
{
    m_edgesMap.insert(EdgesMap::value_type(pEdge->nodeIn, pEdge));
    m_edgesMap.insert(EdgesMap::value_type(pEdge->nodeOut, pEdge));
}

//////////////////////////////////////////////////////////////////////////
CHyperEdge* CHyperGraph::MakeEdge(CHyperNode* pNodeOut, CHyperNodePort* pPortOut, CHyperNode* pNodeIn, CHyperNodePort* pPortIn, bool bEnabled, bool fromSpecialDrag)
{
    assert(pNodeIn);
    assert(pNodeOut);
    assert(pPortIn);
    assert(pPortOut);

    CHyperEdge* pNewEdge = CreateEdge();

    pNewEdge->nodeIn = pNodeIn->GetId();
    pNewEdge->nodeOut = pNodeOut->GetId();
    pNewEdge->portIn = pPortIn->GetName();
    pNewEdge->portOut = pPortOut->GetName();
    pNewEdge->enabled = bEnabled;

    ++pPortOut->nConnected;
    ++pPortIn->nConnected;

    // All connected ports must be made visible
    pPortOut->bVisible = true;
    pPortIn->bVisible = true;

    pNewEdge->nPortIn = pPortIn->nPortIndex;
    pNewEdge->nPortOut = pPortOut->nPortIndex;

    RegisterEdge(pNewEdge, fromSpecialDrag);

    SetModified();

    return pNewEdge;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::EnableEdge(CHyperEdge* pEdge, bool bEnable)
{
    pEdge->enabled = bEnable;
    SendNotifyEvent(0, EHG_GRAPH_EDGE_STATE_CHANGED);
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::IsNodeActivationModified()
{
    bool retVal = false;
    NodesMap::iterator it = m_nodesMap.begin();
    NodesMap::iterator end = m_nodesMap.end();
    for (; it != end; ++it)
    {
        if (it->second->IsPortActivationModified())
        {
            it->second->Invalidate(true);
            retVal = true;
        }
    }
    return retVal;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::ClearDebugPortActivation()
{
    NodesMap::iterator it = m_nodesMap.begin();
    NodesMap::iterator end = m_nodesMap.end();
    for (; it != end; ++it)
    {
        it->second->ClearDebugPortActivation();
    }

    for (EdgesMap::const_iterator eit = m_edgesMap.begin(); eit != m_edgesMap.end(); ++eit)
    {
        eit->second->debugCount = 0;
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::OnPostLoad()
{
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    static bool g_graphChangedWarning = false;

    CHyperGraphSerializer serializer(this, ar);
    if (bLoading)
    {
        ClearAll();

        bool bHasChanges = serializer.Serialize(node, bLoading);
        SetModified(bHasChanges);
        SendNotifyEvent(0, EHG_GRAPH_INVALIDATE);
        if (!g_graphChangedWarning && bHasChanges)
        {
            Warning("The hypergraph has been changed during serialization. Please see the log for details on replaced nodes and reconnected links.");
            g_graphChangedWarning = true;
        }
    }
    else
    {
        for (NodesMap::iterator nit = m_nodesMap.begin(); nit != m_nodesMap.end(); ++nit)
        {
            serializer.SaveNode(nit->second);
        }
        for (EdgesMap::const_iterator eit = m_edgesMap.begin(); eit != m_edgesMap.end(); ++eit)
        {
            serializer.SaveEdge(eit->second);
        }
        serializer.Serialize(node, bLoading);
        SetModified(false);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::Migrate(XmlNodeRef& /* node */)
{
    // we changed nothing
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::Save(const char* filename)
{
    bool bWasModified = IsModified();

    m_filename = Path::GamePathToFullPath(filename);
    m_filename = Path::GetRelativePath(m_filename);

    XmlNodeRef graphNode = XmlHelpers::CreateXmlNode("Graph");
    Serialize(graphNode, false);

    bool success = XmlHelpers::SaveXmlNode(GetIEditor()->GetFileUtil(), graphNode, filename);
    if (!success)
    {
        SetModified(bWasModified);
    }
    return success;
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::Load(const char* filename)
{
    XmlNodeRef graphNode = XmlHelpers::LoadXmlFromFile(filename);

    return LoadInternal(graphNode, filename);
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::LoadInternal(XmlNodeRef& graphNode, const char* filename)
{
    if (graphNode)
    {
        m_filename = filename;
        if (m_name.isEmpty())
        {
            m_name = m_filename;
            m_name = Path::RemoveExtension(m_name);
        }

        if (graphNode->isTag("Graph"))
        {
            Serialize(graphNode, true);
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::Import(const char* filename)
{
    XmlNodeRef graphNode = XmlHelpers::LoadXmlFromFile(filename);
    if (graphNode != NULL && graphNode->isTag("Graph"))
    {
        CUndo undo("Import HyperGraph");

        UnselectAll();
        CHyperGraphSerializer serializer(this, 0);
        serializer.SelectLoaded(false);
        bool bHasChanges = serializer.Serialize(graphNode, true);
        SetModified(bHasChanges);
        SendNotifyEvent(0, EHG_GRAPH_INVALIDATE);
        return true;
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::RecordUndo()
{
    if (CUndo::IsRecording())
    {
        IUndoObject* pUndo = CreateUndo(); // derived classes can create undo for the Graph
        assert (pUndo != 0);
        CUndo::Record(pUndo);
    }
}

//////////////////////////////////////////////////////////////////////////
IUndoObject* CHyperGraph::CreateUndo()
{
    return new CUndoHyperGraph(this);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::EditEdge(CHyperEdge* pEdge)
{
    for (Listeners::const_iterator iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
    {
        (*iter)->OnLinkEdit(pEdge);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraph::SetViewPosition(const QPoint& point, float zoom)
{
    m_bViewPosInitialized = true;
    m_viewOffset = point;
    m_fViewZoom = zoom;
}

//////////////////////////////////////////////////////////////////////////
bool CHyperGraph::GetViewPosition(QPoint& point, float& zoom)
{
    point = m_viewOffset;
    zoom = m_fViewZoom;
    return m_bViewPosInitialized;
}
