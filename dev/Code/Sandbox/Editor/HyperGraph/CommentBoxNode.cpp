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
#include "CommentBoxNode.h"
#include "QHyperNodePainter_CommentBox.h"
#include "FlowGraphVariables.h"

static const Vec3 g_defaultCommentColor(70, 90, 180);
static const int g_defaultCommentDrawPriority = 16;

static const float g_defaultWidth = 300.f;
static const float g_defaultHeight = 100.f;

CCommentBoxNode::CCommentBoxNode()
{
    SetClass(GetClassType());
    m_pqPainter = new QHyperNodePainter_CommentBox();
    m_name = "This is a comment box";
    m_resizeBorderRect.setWidth(g_defaultWidth * QHyperNodePainter_CommentBox::AccessStaticVar_ZoomFactor());
    m_resizeBorderRect.setHeight(g_defaultHeight * QHyperNodePainter_CommentBox::AccessStaticVar_ZoomFactor());

    {
        auto fontSizes = new std::vector<std::pair<QString, int> >();
        fontSizes->push_back(std::make_pair("Small", FontSize_Small));
        fontSizes->push_back(std::make_pair("Medium", FontSize_Medium));
        fontSizes->push_back(std::make_pair("Large", FontSize_Large));

        AddPortEx("TextSize", static_cast<int>(FontSize_Small), "Title text size", fontSizes);
    }

    // Node color options
    {
        ColorF defaultColor(g_defaultCommentColor / 255.f);
        Vec3 colorVector = defaultColor.toVec3();
        AddPortEx("Color", colorVector, "Node color", true, IVariable::DT_COLOR);
    }

    // Drawing box filled or empty
    AddPortEx("DisplayFilled", true, "Draw the body of the box or only the borders", true);

    // Indicates whether the comment box is drawn filled or empty
    AddPortEx("DisplayBox", true, "Draw the box", true);

    // Draw priority
    m_drawPriority = g_defaultCommentDrawPriority;
    AddPortEx("SortPriority", m_drawPriority, "Drawing priority", true);
}

CCommentBoxNode::~CCommentBoxNode()
{
}


void CCommentBoxNode::Init()
{
}

void CCommentBoxNode::Done()
{
}


CHyperNode* CCommentBoxNode::Clone()
{
    CCommentBoxNode* pNode = new CCommentBoxNode();
    pNode->CopyFrom(*this);
    pNode->m_resizeBorderRect = m_resizeBorderRect;
    return pNode;
}


// parameter is relative to the node pos
void CCommentBoxNode::SetResizeBorderRect(const QRectF& newRelBordersRect)
{
    QRectF newRect = newRelBordersRect;

    if (IsGridBound())
    {
        newRect.setWidth(floor(((float)newRect.width() / GRID_SIZE) + 0.5f) * GRID_SIZE);
        newRect.setHeight(floor(((float)newRect.height() / GRID_SIZE) + 0.5f) * GRID_SIZE);
    }

    float incYSize = newRect.height() - m_resizeBorderRect.height();
    if ((incYSize + m_rect.height()) <= 0)
    {
        return;
    }
    m_rect.setHeight(m_rect.height() + incYSize);

    m_resizeBorderRect = newRect;
}


// parameter is absolute coordinates
void CCommentBoxNode::SetBorderRect(const QRectF& newAbsBordersRect)
{
    QPointF pos = newAbsBordersRect.topLeft();
    SetPos(pos);
    m_rect.setHeight(newAbsBordersRect.height());
    m_resizeBorderRect.setHeight(newAbsBordersRect.height());
    m_rect.setWidth(newAbsBordersRect.width());
    m_resizeBorderRect.setWidth(newAbsBordersRect.width());
    OnPossibleSizeChange();
}


void CCommentBoxNode::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    CHyperNode::Serialize(node, bLoading, ar);
    float v;

    if (bLoading)
    {
        XmlNodeRef resizeBorderNode = node->findChild("ResizeBorder");
        resizeBorderNode->getAttr("X", v);
        m_resizeBorderRect.setX(v);
        resizeBorderNode->getAttr("Y", v);
        m_resizeBorderRect.setY(v);
        resizeBorderNode->getAttr("Width", v);
        m_resizeBorderRect.setWidth(v);
        resizeBorderNode->getAttr("Height", v);
        m_resizeBorderRect.setHeight(v);
        XmlNodeRef nodeSize = node->findChild("NodeSize");
        if (nodeSize)
        {
            nodeSize->getAttr("Width", v);
            m_rect.setWidth(v);
            nodeSize->getAttr("Height", v);
            m_rect.setHeight(v);
        }
        CHyperNodePort* pPort = FindPort("SortPriority", true);
        if (pPort)
        {
            pPort->pVar->Get(m_drawPriority);
        }

        OnPossibleSizeChange();
    }
    else
    {
        XmlNodeRef resizeBorderNode = node->newChild("ResizeBorder");
        resizeBorderNode->setAttr("X", m_resizeBorderRect.x());
        resizeBorderNode->setAttr("Y", m_resizeBorderRect.y());
        resizeBorderNode->setAttr("Width", m_resizeBorderRect.width());
        resizeBorderNode->setAttr("Height", m_resizeBorderRect.height());
        XmlNodeRef nodeSize = node->newChild("NodeSize");
        nodeSize->setAttr("Width", m_rect.width());
        nodeSize->setAttr("Height", m_rect.height());
    }
}


void CCommentBoxNode::OnZoomChange(float zoom)
{
    const float MIN_ZOOM_VALUE = 0.00001f;
    zoom = max(zoom, MIN_ZOOM_VALUE);
    float zoomFactor = max(1.f, 1.f / zoom);
    QHyperNodePainter_CommentBox::AccessStaticVar_ZoomFactor() = zoomFactor;
    OnPossibleSizeChange(); // the size and position of the node can be changed in the first draw after a zoom change (because the title could have been resized), so we need another redraw.
}


void CCommentBoxNode::OnInputsChanged()
{
    CHyperNodePort* pPort = FindPort("SortPriority", true);
    if (pPort)
    {
        pPort->pVar->Get(m_drawPriority);
    }
    OnPossibleSizeChange();  // the title font size could be modified
}


void CCommentBoxNode::OnPossibleSizeChange()
{
    Invalidate(true);
    if (!m_qdispList.Empty())
    {
        QRectF bounds = m_qdispList.GetBounds();
        m_rect.setWidth(bounds.width());
        m_rect.setHeight(bounds.height());
        Invalidate(true); // need a double call because the size and/or position could change after the first one.
    }
}

void CCommentBoxNode::SetPos(const QPointF& pos)
{
    QPointF p = pos;
    if (IsGridBound())
    {
        p.setX(floor(((float)pos.x() / GRID_SIZE) + 0.5f) * GRID_SIZE);
        p.setY(floor(((float)pos.y() / GRID_SIZE) + 0.5f) * GRID_SIZE);
    }

    m_rect = QRectF(p.x(), p.y(), m_rect.width(), m_rect.height());
    Invalidate(false);
}