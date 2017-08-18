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

#include "StdAfx.h"
#include "BlackBoxNode.h"
#include "QHyperNodePainter_BlackBox.h"
#include "FlowGraphNode.h"
#include "FlowGraph.h"
#include "FlowGraphManager.h"
#include "FlowGraphVariables.h"
#include <vector>
#include <tuple>

static const int g_defaultBlackBoxDrawPriority = 16;
static const Vec3 g_defaultBlackBoxColor(255, 255, 255);

static const float g_childNodeMovementOffset = 45.0f;

static QHyperNodePainter_BlackBox qbbPainter;


CBlackBoxNode::CBlackBoxNode()
{
    SetClass(GetClassType());
    m_pqPainter = &qbbPainter;
    m_name = "This is a group";
    m_bCollapsed = false;

    // Text size options
    {
        auto fontSizes = new std::vector<std::pair<QString, int> >();
        fontSizes->push_back(std::make_pair("Small", FontSize_Small));
        fontSizes->push_back(std::make_pair("Medium", FontSize_Medium));
        fontSizes->push_back(std::make_pair("Large", FontSize_Large));

        AddPortEx("TextSize", static_cast<int>(FontSize_Medium), "Title text size", fontSizes);
    }

    // Drawing box filled or empty
    AddPortEx("DisplayFilled", true, "Draw the body of the box or only the borders", true);

    // Node color options
    {
        ColorF defaultColor(g_defaultBlackBoxColor / 255.f);
        Vec3 colorVector = defaultColor.toVec3();
        AddPortEx("Color", colorVector, "Node color", true, IVariable::DT_COLOR);
    }

    // Draw priority
    m_drawPriority = g_defaultBlackBoxDrawPriority;
    AddPortEx("SortPriority", m_drawPriority, "Drawing priority", true);
}

void CBlackBoxNode::Init()
{
}

void CBlackBoxNode::Done()
{
    GetNodesSafe();
    for (int i = 0; i < m_nodes.size(); ++i)
    {
        m_nodes[i]->SetBlackBox(NULL);
    }
}

CHyperNode* CBlackBoxNode::Clone()
{
    CBlackBoxNode* pNode = new CBlackBoxNode();
    pNode->CopyFrom(*this);
    return pNode;
}

void CBlackBoxNode::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    CHyperNode::Serialize(node, bLoading, ar);

    CHyperGraph* pMyEditorFG = (CHyperGraph*)GetGraph();

    if (bLoading)
    {
        m_nodes.clear();
        m_nodesIDs.clear();
        m_ports.clear();

        if (pMyEditorFG)
        {
            if (XmlNodeRef children = node->findChild("BlackBoxChildren"))
            {
                for (int i = 0; i < children->getNumAttributes(); ++i)
                {
                    const char* key = NULL;
                    const char* value = NULL;
                    children->getAttributeByIndex(i, &key, &value);
                    HyperNodeID nodeId = (HyperNodeID)(atoi(value));

                    if (IHyperNode* pNode = pMyEditorFG->FindNode(nodeId))
                    {
                        AddNode((CHyperNode*)pNode);
                    }
                }
            }
        }

        OnInputsChanged();
        UpdateNodeBounds();
    }
    else
    {
        XmlNodeRef children = XmlHelpers::CreateXmlNode("BlackBoxChildren");
        for (int i = 0; i < m_nodesIDs.size(); ++i)
        {
            children->setAttr("Node" + ToString(i), m_nodesIDs[i]);
        }
        node->addChild(children);
    }
}


void CBlackBoxNode::OnInputsChanged()
{
    CHyperNodePort* pPort = FindPort("SortPriority", true);
    if (pPort)
    {
        pPort->pVar->Get(m_drawPriority);
    }
}

void CBlackBoxNode::OnZoomChange(float zoom)
{
    UpdateNodeBounds();
}

void CBlackBoxNode::SetPos(const QPointF& pos)
{
    float dX, dY;
    QPointF p = GetPos();
    dX = pos.x() - p.x();
    dY = pos.y() - p.y();
    for (int i = 0; i < m_nodes.size(); ++i)
    {
        p = m_nodes[i]->GetPos();
        p += QPointF(dX, dY);
        m_nodes[i]->SetPos(p);
    }

    UpdateNodeBounds();
}

void CBlackBoxNode::UpdateNodeBounds()
{
    // If the node hasn't been painted yet
    if (m_qdispList.Empty())
    {
        Invalidate(true);
    }

    QRectF bounds = m_qdispList.GetBounds();

    //This is a temporary fix so that Cloning does not lock up the editor.
    //However, a group node should probably not be allowed to exists with zero nodes.
    //This check should be turned into an assert once the clone functionality is fixed.
    if (m_nodes.size() > 0)
    {
        // Find the top left most node in this group and move the group node to the correct position above it
        QPointF topLeft(std::numeric_limits<qreal>::max(), std::numeric_limits<qreal>::max());

        for (auto currentNode : m_nodes)
        {
            auto currentNodePosition = currentNode->GetPos();

            if (topLeft.x() > currentNodePosition.x())
            {
                topLeft.setX(currentNodePosition.x());
            }

            if (topLeft.y() > currentNodePosition.y())
            {
                topLeft.setY(currentNodePosition.y());
            }
        }

        m_rect = QRectF(topLeft.x() - g_childNodeMovementOffset,
                topLeft.y() - g_childNodeMovementOffset,
                bounds.width(), bounds.height());
    }

    // The position was changed so a repaint is required
    Invalidate(true);
}

void CBlackBoxNode::AddNode(CHyperNode* pNode)
{
    if (pNode && !stl::find(m_nodes, pNode))
    {
        if (pNode->GetGraph() == GetGraph())
        {
            m_nodes.push_back(pNode);
            pNode->SetBlackBox(this);
            m_nodesIDs.push_back(pNode->GetId());

            UpdateNodeBounds();
        }
    }
}

void CBlackBoxNode::RemoveNode(CHyperNode* pNode)
{
    if (pNode)
    {
        // Flag that is set only if a node is actually removed
        bool wasNodeRemoved = false;

        // Clear the node from this black box
        auto currentNode = std::find(m_nodes.begin(), m_nodes.end(), pNode);
        if (currentNode != m_nodes.end())
        {
            // Clear this node from this Group
            m_nodes.erase(currentNode);

            // Make sure that the node is no longer attached to this blackbox
            pNode->SetBlackBox(nullptr);

            // Indicate that a node has been removed
            wasNodeRemoved = true;
        }

        // Clear the node id from this black box
        auto nodeId = std::find(m_nodesIDs.begin(), m_nodesIDs.end(), pNode->GetId());
        if (nodeId != m_nodesIDs.end())
        {
            m_nodesIDs.erase(nodeId);
        }

        if (wasNodeRemoved)
        {
            // Invalidate the drawing of this Group
            Invalidate(true);

            // If the Group is empty
            if (m_nodes.empty())
            {
                // Then remove it
                GetGraph()->RemoveNode(this);
            }
            else
            {
                UpdateNodeBounds();
            }
        }
    }
}

bool CBlackBoxNode::IncludesNode(CHyperNode* pNode)
{
    if (stl::find(m_nodes, pNode))
    {
        return true;
    }
    return false;
}

bool CBlackBoxNode::IncludesNode(HyperNodeID nodeId)
{
    for (int i = 0; i < m_nodes.size(); ++i)
    {
        if (m_nodes[i]->GetId() == nodeId)
        {
            return true;
        }
    }
    return false;
}

QPointF CBlackBoxNode::GetPointForPort(CHyperNodePort* port)
{
    auto it = m_ports.begin();
    auto end = m_ports.end();
    for (; it != end; ++it)
    {
        if (QString::compare(it->first->GetHumanName(), port->GetHumanName()) == 0)
        {
            return GetPos() + it->second;
        }
    }
    return GetPos();
}

CHyperNodePort* CBlackBoxNode::GetPortAtPoint2(const QPointF& point)
{
    auto it = m_ports.begin();
    auto end = m_ports.end();
    QPointF pos = GetPos();
    for (; it != end; ++it)
    {
        int dX = (pos.x() + it->second.x()) - point.x();
        int dY = (pos.y() + it->second.y()) - point.y();
        if (abs(dX) < 10.0f && abs(dY) < 10.0f)
        {
            return it->first;
        }
    }
    return NULL;
}

CHyperNode* CBlackBoxNode::GetNode(CHyperNodePort* port)
{
    std::vector<CHyperNode*>::iterator it = m_nodes.begin();
    std::vector<CHyperNode*>::iterator end = m_nodes.end();
    for (; it != end; ++it)
    {
        CHyperNode::Ports* ports = NULL;
        if (port->bInput)
        {
            ports = (*it)->GetInputs();
        }
        else
        {
            ports = (*it)->GetOutputs();
        }
        for (int i = 0; i < ports->size(); ++i)
        {
            CHyperNodePort* pP = &((*ports)[i]);
            if (pP == port)
            {
                return (*it);
            }
        }
    }
    return NULL;
}


std::vector<CHyperNode*>* CBlackBoxNode::GetNodesSafe()
{
    CHyperGraph* pMyEditorFG = (CHyperGraph*)GetGraph();
    m_nodes.clear();

    if (!pMyEditorFG)
    {
        return &m_nodes;
    }

    for (int i = 0; i < m_nodesIDs.size(); ++i)
    {
        if (IHyperNode* pNode = pMyEditorFG->FindNode(m_nodesIDs[i]))
        {
            m_nodes.push_back((CHyperNode*)pNode);
        }
    }

    return &m_nodes;
}

CBlackBoxNode::~CBlackBoxNode()
{
}