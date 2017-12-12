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

// Description : Black Box composite node


#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_BLACKBOXNODE_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_BLACKBOXNODE_H
#pragma once


#include "HyperGraphNode.h"
class CFlowNode;
class CHyperNode;

class CBlackBoxNode
    : public CHyperNode
{
public:
    static const char* GetClassType()
    {
        return "_blackbox";
    }
    CBlackBoxNode();
    ~CBlackBoxNode();
    void Init();
    void Done();
    CHyperNode* Clone();
    void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar) override;

    void SetPos(const QPointF& pos) override;

    /**
    * Called whenever bounds of a Group need to be updated
    */
    void UpdateNodeBounds();

    //KDAB_PORT     int GetObjectAt(const QPointF& point) override;

    void AddNode(CHyperNode* pNode);
    void RemoveNode(CHyperNode* pNode);
    bool IncludesNode(CHyperNode* pNode);
    bool IncludesNode(HyperNodeID nodeId);
    void Clear() {m_nodes.clear(); }
    std::vector<CHyperNode*>* GetNodes() { return &m_nodes; }
    std::vector<CHyperNode*>* GetNodesSafe();
    QPointF GetPointForPort(CHyperNodePort* port);
    CHyperNodePort* GetPortAtPoint2(const QPointF& point);
    CHyperNode* GetNode(CHyperNodePort* port);
    ILINE void SetPort(CHyperNodePort* port, const QPointF& pos)
    {
        m_ports[port] = pos;
    }

    void SetNormalBounds(QRectF& rect) { m_normalBounds = rect; }

    void Minimize()
    {
        m_bCollapsed = !m_bCollapsed;
        for (int i = 0; i < m_nodes.size(); ++i)
        {
            m_nodes[i]->Invalidate(true);
        }
        if (m_bCollapsed)
        {
            m_iBrackets = -1;
        }
        Invalidate(true);
    }
    bool IsMinimized() { return m_bCollapsed; }

    CHyperNodePort* GetPortAtPoint(const QPointF& point) override
    {
        //go through sub-nodes to retrieve port .. ?
        return NULL;
    }
    bool IsEditorSpecialNode() override { return true; }
    bool IsFlowNode() override { return false; }

    void OnInputsChanged() override;

    void OnZoomChange(float zoom) override;

    enum FontSize
    {
        FontSize_Small = 1,
        FontSize_Medium = 2,
        FontSize_Large = 3
    };

private:

    std::vector<CHyperNode*> m_nodes;
    std::vector<HyperNodeID> m_nodesIDs;
    std::map<CHyperNodePort*, QPointF> m_ports;
    bool    m_bCollapsed;
    int     m_iBrackets;
    QRectF m_normalBounds;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_BLACKBOXNODE_H
