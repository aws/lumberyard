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
#include "HyperGraphNode.h"
#include "HyperGraph.h"
#include "QHyperNodePainter_Default.h"
#include "BlackBoxNode.h"
#include "MissingNode.h"
#include "FlowGraphVariables.h"


#define MIN_ZOOM_DRAW_ALL_ELEMS 0.1f

//////////////////////////////////////////////////////////////////////////
// Undo object for CHyperNode.
class CUndoHyperNode
    : public IUndoObject
{
public:
    CUndoHyperNode(CHyperNode* node)
    {
        // Stores the current state of this object.
        assert(node != 0);
        m_node = node;
        m_redo = 0;
        m_undo = XmlHelpers::CreateXmlNode("Undo");
        m_node->Serialize(m_undo, false);
    }
protected:
    virtual int GetSize() { return sizeof(*this); }
    virtual QString GetDescription() { return "HyperNodeUndo"; };

    virtual void Undo(bool bUndo)
    {
        if (bUndo)
        {
            m_redo = XmlHelpers::CreateXmlNode("Redo");
            m_node->Serialize(m_redo, false);
        }
        // Undo object state.
        m_node->Invalidate(true); // Invalidate previous position too.
        m_node->Serialize(m_undo, true);
        m_node->Invalidate(true);
    }
    virtual void Redo()
    {
        m_node->Invalidate(true); // Invalidate previous position too.
        m_node->Serialize(m_redo, true);
        m_node->Invalidate(true);
    }

private:
    THyperNodePtr m_node;
    XmlNodeRef m_undo;
    XmlNodeRef m_redo;
};

static QHyperNodePainter_Default qpainterDefault;

//////////////////////////////////////////////////////////////////////////
CHyperNode::CHyperNode()
{
    m_pGraph = NULL;
    m_bSelected = 0;
    m_bSizeValid = 0;
    m_nFlags = 0;
    m_id = 0;
    m_pBlackBox = NULL;
    m_debugCount = 0;
    m_drawPriority = MAX_DRAW_PRIORITY;
    m_iMissingPort = 0;
    m_rect = QRectF(0, 0, 0, 0);
    m_pqPainter = &qpainterDefault;
    m_name = "";
    m_classname = "";
}

//////////////////////////////////////////////////////////////////////////
CHyperNode::~CHyperNode()
{
    if (m_pBlackBox)
    {
        m_pBlackBox = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::Done()
{
    if (m_pBlackBox)
    {
        ((CBlackBoxNode*)m_pBlackBox)->RemoveNode(this);
        m_pBlackBox = NULL;
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::CopyFrom(const CHyperNode& src)
{
    m_id = src.m_id;
    m_name = src.m_name;
    m_classname = src.m_classname;
    m_rect = src.m_rect;
    m_nFlags = src.m_nFlags;
    m_iMissingPort = src.m_iMissingPort;

    m_inputs = src.m_inputs;
    m_outputs = src.m_outputs;
}

//////////////////////////////////////////////////////////////////////////
IHyperGraph* CHyperNode::GetGraph() const
{
    return m_pGraph;
};

//////////////////////////////////////////////////////////////////////////
void CHyperNode::SetName(const QString& sName)
{
    m_name = sName;
    if (m_pGraph)
    {
        m_pGraph->SendNotifyEvent(this, EHG_NODE_SET_NAME);
    }
    Invalidate(true);
}

void CHyperNode::SetPos(const QPointF& pos)
{
    m_rect = QRectF(pos.x(), pos.y(), m_rect.width(), m_rect.height());
    Invalidate(false);

    // If this Node is attached to a group
    if (GetBlackBox())
    {
        // Indicate to the Group that one of its children moved
        GetBlackBox()->UpdateNodeBounds();

        // Invalidate the group to force a redraw
        m_pBlackBox->Invalidate(true, false);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::Invalidate(bool redraw, bool isModified /*= true*/)
{
    if (redraw)
    {
        m_qdispList.Clear();
    }
    if (m_pGraph)
    {
        m_pGraph->InvalidateNode(this, isModified);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::SetRect(const QRectF& rc)
{
    if (rc == m_rect)
    {
        return;
    }

    RecordUndo();
    m_rect = rc;
    SetModified();
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::SetSelected(bool bSelected)
{
    if (bSelected != m_bSelected)
    {
        m_bSelected = bSelected;
        if (!bSelected)
        {
            // Clear selection of all ports.
            int i;
            for (i = 0; i < m_inputs.size(); i++)
            {
                m_inputs[i].bSelected = false;
            }
            for (i = 0; i < m_outputs.size(); i++)
            {
                m_outputs[i].bSelected = false;
            }
        }

        m_pGraph->SendNotifyEvent(this, bSelected ? EHG_NODE_SELECT : EHG_NODE_UNSELECT);
        Invalidate(true);
    }
}

//////////////////////////////////////////////////////////////////////////
QString CHyperNode::GetTitle() const
{
    if (m_name.isEmpty())
    {
        QString title = GetUIClassName();
#if 0 // AlexL: 10/03/2006 show full group:class
        int p = title.Find(':');
        if (p >= 0)
        {
            title = title.Mid(p + 1);
        }
#endif
        return title;
    }
    return m_name + QStringLiteral(" (") + QString(GetUIClassName()) + QStringLiteral(")");
};

QString CHyperNode::VarToValue(IVariable* var)
{
    QString value = var->GetDisplayValue();

    // smart object helpers are stored in format class:name
    // but it's enough to display only the name
    if (var->GetDataType() == IVariable::DT_SOHELPER || var->GetDataType() == IVariable::DT_SONAVHELPER || var->GetDataType() == IVariable::DT_SOANIMHELPER)
    {
        value = value.mid(value.indexOf(':') + 1);
    }
    else if (var->GetDataType() == IVariable::DT_SEQUENCE_ID)
    {
        // Return the human-readable name instead of the ID.
        IAnimSequence* pSeq = GetIEditor()->GetMovieSystem()->FindSequenceById((uint32)value.toUInt());
        if (pSeq)
        {
            return pSeq->GetName();
        }
    }

    return value;
}

//////////////////////////////////////////////////////////////////////////
QString CHyperNode::GetPortName(const CHyperNodePort& port)
{
    if (port.bInput)
    {
        QString text = port.pVar->GetHumanName();
        if (port.pVar->GetType() != IVariable::UNKNOWN && !port.nConnected)
        {
            text = text + QStringLiteral("=") + VarToValue(port.pVar);
        }
        return text;
    }
    return port.pVar->GetHumanName();
}

//////////////////////////////////////////////////////////////////////////
CHyperNodePort* CHyperNode::FindPort(const QString& portname, bool bInput)
{
    if (bInput)
    {
        for (int i = 0; i < m_inputs.size(); i++)
        {
            if (portname.compare(m_inputs[i].GetName(), Qt::CaseInsensitive) == 0)
            {
                return &m_inputs[i];
            }
        }
    }
    else
    {
        for (int i = 0; i < m_outputs.size(); i++)
        {
            if (portname.compare(m_outputs[i].GetName(), Qt::CaseInsensitive) == 0)
            {
                return &m_outputs[i];
            }
        }
    }
    return 0;
}

CHyperNodePort* CHyperNode::FindPort(int idx, bool bInput)
{
    if (bInput)
    {
        if (idx >= m_inputs.size())
        {
            return NULL;
        }

        return &m_inputs[idx];
    }
    else
    {
        if (idx >= m_outputs.size())
        {
            return NULL;
        }

        return &m_outputs[idx];
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CHyperNodePort* CHyperNode::CreateMissingPort(const char* portname, bool bInput, bool bAttribute)
{
    if (m_pGraph && m_pGraph->IsLoading())
    {
        IVariablePtr pVar;
        if (bAttribute)
        {
            pVar = new CVariableFlowNode<QString>();
        }
        else
        {
            pVar = new CVariableFlowNodeVoid();
        }

        if (GetClassName() != MISSING_NODE_CLASS)
        {
            pVar->SetHumanName(QString("MISSING: ") + portname);
        }
        else
        {
            pVar->SetHumanName(portname);
        }

        pVar->SetName(portname);
        pVar->SetDescription("MISSING PORT");

        CHyperNodePort newPort(pVar, bInput, !bInput);      // bAllowMulti if output
        AddPort(newPort);

        ++m_iMissingPort;

        if (bInput)
        {
            return &(*m_inputs.rbegin());
        }
        else
        {
            return &(*m_outputs.rbegin());
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar)
{
    if (bLoading)
    {
        unsigned int nInputHideMask = 0;
        unsigned int nOutputHideMask = 0;
        int flags = 0;
        // Saving.
        Vec3 pos(0, 0, 0);
        node->getAttr("Name", m_name);
        node->getAttr("Class", m_classname);
        node->getAttr("pos", pos);
        node->getAttr("flags", flags);
        node->getAttr("InHideMask", nInputHideMask);
        node->getAttr("OutHideMask", nOutputHideMask);

        if (flags & 1)
        {
            m_bSelected = true;
        }
        else
        {
            m_bSelected = false;
        }

        m_rect.setX(pos.x);
        m_rect.setY (pos.y);
        m_rect.setWidth(0);
        m_rect.setHeight(0);
        m_bSizeValid = false;

        {
            XmlNodeRef portsNode = node->findChild("Inputs");
            if (portsNode)
            {
                for (int i = 0; i < m_inputs.size(); i++)
                {
                    IVariable* pVar = m_inputs[i].pVar;
                    if (pVar->GetType() != IVariable::UNKNOWN)
                    {
                        pVar->Serialize(portsNode, true);
                    }

                    if (ar && pVar->GetDataType() == IVariable::DT_SEQUENCE_ID)
                    {
                        // A remapping might be necessary for a sequence Id.
                        int seqId = 0;
                        pVar->Get(seqId);
                        pVar->Set((int)ar->RemapSequenceId((uint32)seqId));
                    }

                    if ((nInputHideMask & (1 << i)) && m_inputs[i].nConnected == 0)
                    {
                        m_inputs[i].bVisible = false;
                    }
                    else
                    {
                        m_inputs[i].bVisible = true;
                    }
                }
                /*
                                for (int i = 0; i < portsNode->getChildCount(); i++)
                                {
                                    XmlNodeRef portNode = portsNode->getChild(i);
                                    CHyperNodePort *pPort = FindPort( portNode->getTag(),true );
                                    if (!pPort)
                                        continue;
                                    pPort->pVar->Set( portNode->getContent() );
                                }
                                */
            }
        }
        // restore outputs visibility.
        {
            for (int i = 0; i < m_outputs.size(); i++)
            {
                if ((nOutputHideMask & (1 << i)) && m_outputs[i].nConnected == 0)
                {
                    m_outputs[i].bVisible = false;
                }
                else
                {
                    m_outputs[i].bVisible = true;
                }
            }
        }

        m_pBlackBox = NULL;

        Invalidate(true);
    }
    else
    {
        // Saving.
        if (!m_name.isEmpty())
        {
            node->setAttr("Name", m_name.toUtf8().data());
        }
        node->setAttr("Class", m_classname.toUtf8().data());

        Vec3 pos(m_rect.x(), m_rect.y(), 0);
        node->setAttr("pos", pos);

        int flags = 0;
        if (m_bSelected)
        {
            flags |= 1;
        }
        node->setAttr("flags", flags);

        unsigned int nInputHideMask = 0;
        unsigned int nOutputHideMask = 0;
        if (!m_inputs.empty())
        {
            XmlNodeRef portsNode = node->newChild("Inputs");
            for (int i = 0; i < m_inputs.size(); i++)
            {
                if (!m_inputs[i].bVisible)
                {
                    nInputHideMask |= (1 << i);
                }
                IVariable* pVar = m_inputs[i].pVar;

                if (pVar->GetType() != IVariable::UNKNOWN)
                {
                    pVar->Serialize(portsNode, false);
                }
            }
        }
        {
            // Calc output vis mask.
            for (int i = 0; i < m_outputs.size(); i++)
            {
                if (!m_outputs[i].bVisible)
                {
                    nOutputHideMask |= (1 << i);
                }
            }
        }
        if (nInputHideMask != 0)
        {
            node->setAttr("InHideMask", nInputHideMask);
        }
        if (nOutputHideMask != 0)
        {
            node->setAttr("OutHideMask", nOutputHideMask);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::SetModified(bool bModified)
{
    m_pGraph->SetModified(bModified);
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::ClearPorts()
{
    m_inputs.clear();
    m_outputs.clear();
    m_iMissingPort = 0;
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::AddPort(const CHyperNodePort& port)
{
    if (port.bInput)
    {
        CHyperNodePort temp = port;
        temp.nPortIndex = m_inputs.size();
        m_inputs.push_back(temp);
    }
    else
    {
        CHyperNodePort temp = port;
        temp.nPortIndex = m_outputs.size();
        m_outputs.push_back(temp);
    }
    Invalidate(true);
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::PopulateContextMenu(QMenu* menu, int baseCommandId)
{
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::OnContextMenuCommand(int nCmd)
{
}

//////////////////////////////////////////////////////////////////////////
CVarBlock* CHyperNode::GetInputsVarBlock()
{
    CVarBlock* pVarBlock = new CVarBlock;
    for (int i = 0; i < m_inputs.size(); i++)
    {
        IVariable* pVar = m_inputs[i].pVar;
        if (pVar->GetType() != IVariable::UNKNOWN)
        {
            pVarBlock->AddVariable(pVar);
        }
    }
    return pVarBlock;
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::RecordUndo()
{
    if (CUndo::IsRecording() && !m_pBlackBox)
    {
        IUndoObject* pUndo = CreateUndo();
        assert (pUndo != 0);
        CUndo::Record(pUndo);
    }
}

//////////////////////////////////////////////////////////////////////////
IUndoObject* CHyperNode::CreateUndo()
{
    return new CUndoHyperNode(this);
}

//////////////////////////////////////////////////////////////////////////
IHyperGraphEnumerator* CHyperNode::GetRelatedNodesEnumerator()
{
    // Traverse the parent hierarchy upwards to find the ultimate ancestor.
    CHyperNode* pNode = this;
    while (pNode->GetParent())
    {
        pNode = static_cast<CHyperNode*>(pNode->GetParent());
    }

    // Create an enumerator that returns all the descendants of this node.
    return new CHyperNodeDescendentEnumerator(pNode);
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::SelectInputPort(int nPort, bool bSelected)
{
    if (nPort >= 0 && nPort < m_inputs.size())
    {
        if (m_inputs[nPort].bSelected != bSelected)
        {
            m_inputs[nPort].bSelected = bSelected;
            Invalidate(true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::SelectOutputPort(int nPort, bool bSelected)
{
    if (nPort >= 0 && nPort < m_outputs.size())
    {
        if (m_outputs[nPort].bSelected != bSelected)
        {
            m_outputs[nPort].bSelected = bSelected;
            Invalidate(true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperNode::UnselectAllPorts()
{
    bool bAnySelected = false;
    int i = 0;
    for (i = 0; i < m_inputs.size(); i++)
    {
        if (m_inputs[i].bSelected)
        {
            bAnySelected = true;
        }
        m_inputs[i].bSelected = false;
    }

    for (i = 0; i < m_outputs.size(); i++)
    {
        if (m_outputs[i].bSelected)
        {
            bAnySelected = true;
        }
        m_outputs[i].bSelected = false;
    }

    if (bAnySelected)
    {
        Invalidate(true, false);
    }
}

CHyperNode* CHyperNode::GetBlackBoxWhenMinimized() const
{
    if (m_pBlackBox)
    {
        if (((CBlackBoxNode*)m_pBlackBox)->IsMinimized())
        {
            return m_pBlackBox;
        }
    }
    return nullptr;
}

CBlackBoxNode* CHyperNode::GetBlackBox() const
{
    return static_cast<CBlackBoxNode*>(m_pBlackBox);
}

//////////////////////////////////////////////////////////////////////////

// QT code
void CHyperNode::Draw(QHyperGraphWidget* pView, QPainter* gr, bool render)
{
    if (m_qdispList.Empty())
    {
        Redraw();
    }
    if (render)
    {
        gr->save();
        QPointF pos = GetPos();
        gr->translate(pos);
        m_qdispList.Draw(gr, m_pGraph->GetViewZoom() > MIN_ZOOM_DRAW_ALL_ELEMS);
        gr->restore();
    }
}

void CHyperNode::Redraw()
{
    m_qdispList.Clear();
    m_pqPainter->Paint(this, &m_qdispList);

    QRectF temp = m_rect;
    QRectF rect = m_qdispList.GetBounds();
    m_rect = QRectF(temp.x(), temp.y(), rect.width(), rect.height());
}

QSizeF CHyperNode::CalculateSize()
{
    if (m_qdispList.Empty())
    {
        Redraw();
    }

    QSizeF size(m_rect.width(), m_rect.height());
    return size;
}
