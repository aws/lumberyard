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

// Description : HyperGraph declaration.


#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPH_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPH_H
#pragma once

#include "IHyperGraph.h"
#include "HyperGraphNode.h"

class CHyperGraph;

#define GET_TRANSPARENCY (gSettings.hyperGraphColors.opacity * 2.55f)

enum EHyperEdgeDirection
{
    eHED_Up = 0,
    eHED_Down,
    eHED_Left,
    eHED_Right
};

//////////////////////////////////////////////////////////////////////////
//
// CHyperGraphEdge connects output and input port of two graph nodes.
//
//////////////////////////////////////////////////////////////////////////
class CHyperEdge
    : public _i_reference_target_t
{
public:
    CHyperEdge()
        : dirIn(eHED_Up)
        , dirOut(eHED_Up)
        , cornerW(0)
        , cornerH(0)
        , cornerModified(0)
        , enabled(true)
        , debugCount(0) {};

    HyperNodeID nodeIn;
    HyperNodeID nodeOut;
    QString portIn;
    QString portOut;
    bool    enabled;

    // Cached Points where edge is drawn.
    QPointF pointIn;
    QPointF pointOut;
    EHyperEdgeDirection dirIn;
    EHyperEdgeDirection dirOut;

    QPointF cornerPoints[4];
    QPointF originalCenter;
    float cornerW, cornerH;
    int cornerModified;
    int debugCount;

    //////////////////////////////////////////////////////////////////////////
    uint16 nPortIn; // Index of input port.
    uint16 nPortOut; // Index of output port.
    //////////////////////////////////////////////////////////////////////////

    virtual bool IsEditable() { return false; }
    virtual void DrawSpecial(QPointF where) {}
    int  GetCustomSelectionMode() { return -1; }
};

//////////////////////////////////////////////////////////////////////////
// Node serialization context.
//////////////////////////////////////////////////////////////////////////
class CHyperGraphSerializer
{
public:
    CHyperGraphSerializer(CHyperGraph* pGraph, CObjectArchive* ar);

    // Check if loading or saving.
    bool IsLoading() const { return m_bLoading; }
    void SelectLoaded(bool bEnable) { m_bSelectLoaded = bEnable; };

    void SaveNode(CHyperNode* pNode, bool bSaveEdges = true);
    void SaveEdge(CHyperEdge* pEdge);

    // Load nodes from xml.
    // On load: returns true if migration has occured (-> dirty)
    bool Serialize(XmlNodeRef& node,
        bool bLoading,
        bool bLoadEdges = true,
        bool bIsPaste = false,
        bool isExport = false);

    // Get serialized xml node.
    XmlNodeRef GetXmlNode();

    HyperNodeID GetId(HyperNodeID id);
    void RemapId(HyperNodeID oldId, HyperNodeID newId);

    void GetLoadedNodes(std::vector<CHyperNode*>& nodes) const;
    CHyperNode* GetFirstNode();

private:
    bool m_bLoading;
    bool m_bSelectLoaded;

    CHyperGraph* m_pGraph;
    CObjectArchive* m_pAR;

    std::map<HyperNodeID, HyperNodeID> m_remap;

    typedef std::set<THyperNodePtr> Nodes;
    typedef std::set<CHyperEdge*> Edges;
    Nodes m_nodes;
    Edges m_edges;
};

//////////////////////////////////////////////////////////////////////////
//
// Hyper Graph contains nodes and edges that link them.
// This is a base class, specific graphs derives from this (CFlowGraph, etc..)
//
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CHyperGraph
    : public IHyperGraph
{
public:
    CHyperGraph(CHyperGraphManager* pManager);
    ~CHyperGraph();

    void OnHyperGraphManagerDestroyed() { m_pManager = NULL; }
    CHyperGraphManager* GetManager() const { return m_pManager; }

    //////////////////////////////////////////////////////////////////////////
    // IHyperGraph implementation
    //////////////////////////////////////////////////////////////////////////
    IHyperGraphEnumerator* GetNodesEnumerator() override;
    void AddListener(IHyperGraphListener* pListener) override;
    void RemoveListener(IHyperGraphListener* pListener) override;
    IHyperNode* CreateNode(const char* sNodeClass, const QPointF& pos = QPointF(0.0f, 0.0f)) override;
    virtual CHyperEdge* CreateEdge();
    IHyperNode* CloneNode(IHyperNode* pFromNode) override;
    void RemoveNode(IHyperNode* pNode) override;
    virtual void RemoveNodeKeepLinks(IHyperNode* pNode);
    virtual IHyperNode* FindNode(HyperNodeID id) const;
    void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar = 0) override;
    bool Save(const char* filename) override;
    bool Load(const char* filename) override;
    /**
    * Loads a flowgraph that has already been parsed into an "XmlNodeRef"
    * Still needs the name of the file so that it can be cached and used for future operations
    * @param graphNode XML node of the root of a flowgraph
    * @param filename Filename with full path of the XML file that stores the flowgraph
    * @return Indicates whether or not the loading was successful
    */
    bool LoadInternal(XmlNodeRef& graphNode, const char* filename) override;
    bool Migrate(XmlNodeRef& node) override;
    virtual bool Import(const char* filename);
    QString GetName() const override { return m_name; };
    void SetName(const QString& sName) override
    {
        m_name = sName;
        if (m_name.isEmpty())
        {
            m_filename.clear();
        }
    };
    QString GetType() const override { return QStringLiteral("HyperGraph"); }
    virtual IHyperGraph* Clone() { return 0; };
    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx);
    //////////////////////////////////////////////////////////////////////////


    void InvalidateNode(IHyperNode* pNode, bool bModified = true);

    // Unselect all nodes.
    void UnselectAll();
    // Is any node selected
    bool IsSelectedAny();

    // Returns true if ports can be connected.
    virtual bool CanConnectPorts(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, CHyperEdge* pExistingEdge = NULL);

    bool HasEdge(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, CHyperEdge* pExistingEdge = NULL);

    // Creates an edge between source node and target node.
    bool ConnectPorts(CHyperNode* pSrcNode, CHyperNodePort* pSrcPort, CHyperNode* pTrgNode, CHyperNodePort* pTrgPort, bool specialDrag);

    // Removes Edge.
    virtual void RemoveEdge(CHyperEdge* pEdge);
    virtual void RemoveEdgeSilent(CHyperEdge* pEdge);
    void EditEdge(CHyperEdge* pEdge);
    virtual void EnableEdge(CHyperEdge* pEdge, bool bEnable);
    virtual bool IsNodeActivationModified();
    virtual void ClearDebugPortActivation();

    bool GetAllEdges(std::vector<CHyperEdge*>& edges) const;
    bool FindEdges(CHyperNode* pNode, std::vector<CHyperEdge*>& edges) const;
    CHyperEdge* FindEdge(CHyperNode* pNode, CHyperNodePort* pPort) const;

    // Check if graph have any nodes.
    bool IsEmpty() const { return m_nodesMap.empty(); };

    // Mark hyper graph as modified.
    void SetModified(bool bModified = true);
    bool IsModified() const { return m_bModified; }

    HyperNodeID AllocateId() { return m_nextNodeId++; };

    IAIAction* GetAIAction() const override { return NULL; }

    ICustomAction* GetCustomAction() const override { return NULL; }

    void SetViewPosition(const QPoint& point, float zoom);
    bool GetViewPosition(QPoint& point, float& zoom);

    //////////////////////////////////////////////////////////////////////////
    virtual void SetGroupName(const QString& sName) { m_groupName = sName; };
    virtual QString GetGroupName() const { return m_groupName; };

    virtual void SetDescription(const QString& sDesc) { m_description = sDesc; };
    virtual QString GetDescription() const { return m_description; };

    virtual void SetEnabled(bool bEnable) { m_bEnabled = bEnable; }
    virtual bool IsEnabled() const { return m_bEnabled; }
    //////////////////////////////////////////////////////////////////////////

    virtual bool IsFlowGraph() { return false; }

    virtual IUndoObject* CreateUndo();
    virtual void RecordUndo();

    const QString& GetFilename() const { return m_filename; }

    void SendNotifyEvent(IHyperNode* pNode, EHyperGraphEvent event);

    float GetViewZoom() { return m_fViewZoom; }
    bool HasError() const { return GetMissingNodesCount() > 0 || GetMissingPortsCount() > 0; }
    int GetMissingNodesCount() const;
    int GetMissingPortsCount() const;
    bool IsLoading() const { return m_bLoadingNow; }

protected:
    virtual CHyperEdge* MakeEdge(CHyperNode* pNodeOut, CHyperNodePort* pPortOut, CHyperNode* pNodeIn, CHyperNodePort* pPortIn, bool bEnabled, bool fromSpecialDrag);
    virtual void OnPostLoad();

    // Clear all nodes and edges.
    virtual void ClearAll();

    virtual void RegisterEdge(CHyperEdge* pEdge, bool fromSpecialDrag);
    void AddNode(CHyperNode* pNode);

    typedef std::map<HyperNodeID, THyperNodePtr> NodesMap;
    NodesMap m_nodesMap;

private:
    friend class CHyperGraphSerializer;

    CHyperGraphManager* m_pManager;

    typedef std::multimap<HyperNodeID, _smart_ptr<CHyperEdge> > EdgesMap;
    EdgesMap m_edgesMap;

    typedef std::list<IHyperGraphListener*> Listeners;
    Listeners m_listeners;

    QString m_name;
    QString m_filename;
    HyperNodeID m_nextNodeId;

    bool m_bViewPosInitialized;
    QPoint m_viewOffset;
    float m_fViewZoom;

    QString m_groupName;
    QString m_description;

    bool m_bModified;
    bool m_bEnabled;

    int m_iMissingNodes;
    int m_iMissingPorts;

    void UpdateMissingCount();

protected:
    bool m_bLoadingNow;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPH_H
