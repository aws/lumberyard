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
#include "BreakPointsCtrl.h"

#include "../HyperGraph/FlowGraph.h"
#include "../HyperGraph/FlowGraphManager.h"
#include "../HyperGraph/FlowGraphNode.h"
#include "../HyperGraph/FlowGraphDebuggerEditor.h"

#include <IFlowGraphModuleManager.h>
#include <QtUtil.h>


#define VALIDATE_DEBUGGER(a) if (!a) {return; }


CBreakpointsTreeCtrl::CBreakpointsTreeCtrl(QWidget* parent /* = 0 */)
    : QTreeWidget(parent)
{
    m_pFlowGraphDebugger = GetIFlowGraphDebuggerPtr();
    if (m_pFlowGraphDebugger)
    {
        if (!m_pFlowGraphDebugger->RegisterListener(this, "CBreakpointsTreeCtrl"))
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Could not register as Flowgraph Debugger listener! [CBreakpointsTreeCtrl::Create]");
        }

        FillBreakpoints();
    }

    m_pFlowGraphDebuggerEditor = GetIEditor()->GetFlowGraphDebuggerEditor();
    setHeaderHidden(true);
}

CBreakpointsTreeCtrl::~CBreakpointsTreeCtrl()
{
    if (m_pFlowGraphDebugger)
    {
        m_pFlowGraphDebugger->UnregisterListener(this);
    }
}

void CBreakpointsTreeCtrl::DeleteAllItems()
{
    clear();
}

QTreeWidgetItem* CBreakpointsTreeCtrl::FindItemByGraph(QTreeWidget* pTreeCtrl, IFlowGraphPtr pFlowgraph)
{
    if (pTreeCtrl->topLevelItemCount())
    {
        for (int i = 0; i < pTreeCtrl->topLevelItemCount(); ++i)
        {
            QTreeWidgetItem* item = pTreeCtrl->topLevelItem(i);
            QVariant gr = item->data(0, BreakPointItemRole);
            if (gr.isValid())
            {
                SBreakpointItem breakpointItem = gr.value<SBreakpointItem>();
                if (breakpointItem.flowgraph == pFlowgraph)
                {
                    return item;
                }
            }
        }
    }

    return NULL;
}

QTreeWidgetItem* CBreakpointsTreeCtrl::FindItemByNode(QTreeWidget* pTreeCtrl, QTreeWidgetItem* hGraphItem, TFlowNodeId nodeId)
{
    for (int i = 0; i < hGraphItem->childCount(); ++i)
    {
        QTreeWidgetItem* hNodeItem = hGraphItem->child(i);
        for (int j = 0; j < hNodeItem->childCount(); ++j)
        {
            QTreeWidgetItem* hPortItem = hNodeItem->child(j);
            QVariant v = hPortItem->data(0, BreakPointItemRole);
            if (v.isValid())
            {
                SBreakpointItem breakpointItem = v.value<SBreakpointItem>();
                if (breakpointItem.addr.node == nodeId)
                {
                    return hNodeItem;
                }
            }
        }
    }

    return NULL;
}

QTreeWidgetItem* CBreakpointsTreeCtrl::FindItemByPort(QTreeWidget* pTreeCtrl, QTreeWidgetItem* hNodeItem, TFlowPortId portId, bool isOutput)
{
    for (int i = 0; i < hNodeItem->childCount(); ++i)
    {
        QTreeWidgetItem* hPortItem = hNodeItem->child(i);

        QVariant v = hPortItem->data(0, BreakPointItemRole);
        if (v.isValid())
        {
            SBreakpointItem breakpointItem = v.value<SBreakpointItem>();
            if (breakpointItem.addr.port == portId && breakpointItem.addr.isOutput == isOutput)
            {
                return hPortItem;
            }
        }
    }

    return NULL;
}

void CBreakpointsTreeCtrl::OnBreakpointAdded(const SBreakPoint& breakpoint)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);
    CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);

    if (pFlowgraph)
    {
        QTreeWidgetItem* hGraphItem = FindItemByGraph(this, pFlowGraph);

        if (hGraphItem)
        {
            QTreeWidgetItem* nodeItem = FindItemByNode(this, hGraphItem, breakpoint.addr.node);
            CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpoint.addr.node);
            CHyperNodePort* pPort = pNode->FindPort(breakpoint.addr.port, !breakpoint.addr.isOutput);

            if (nodeItem)
            {
                QTreeWidgetItem* portItem = FindItemByPort(this, nodeItem, breakpoint.addr.port, breakpoint.addr.isOutput);

                if (!portItem)
                {
                    if (pPort)
                    {
                        QIcon icon = breakpoint.addr.isOutput ? QIcon(":/HyperGraph/debugger/debug_output_port.png") : QIcon(":/HyperGraph/debugger/debug_input_port.png");
                        portItem = new QTreeWidgetItem(nodeItem);
                        portItem->setText(0, pPort->GetHumanName());
                        portItem->setIcon(0, icon);

                        SBreakpointItem breakPointData;
                        breakPointData.flowgraph = pFlowGraph;
                        breakPointData.addr = breakpoint.addr;
                        breakPointData.type = SBreakpointItem::eIT_Port;

                        QVariant v;
                        v.setValue<SBreakpointItem>(breakPointData);
                        portItem->setData(0, BreakPointItemRole, v);
                        portItem->setExpanded(true);
                    }
                }
            }
            else
            {
                nodeItem = new QTreeWidgetItem(hGraphItem);
                nodeItem->setText(0, pNode->GetUIClassName());
                nodeItem->setIcon(0, QIcon(":/HyperGraph/debugger/debug_node.png"));

                SBreakpointItem breakPointData;
                breakPointData.flowgraph = pFlowGraph;
                breakPointData.addr = breakpoint.addr;
                breakPointData.type = SBreakpointItem::eIT_Node;
                QVariant v;
                v.setValue<SBreakpointItem>(breakPointData);
                nodeItem->setData(0, BreakPointItemRole, v);

                if (pPort)
                {
                    QIcon icon = breakpoint.addr.isOutput ? QIcon(":/HyperGraph/debugger/debug_output_port.png") : QIcon(":/HyperGraph/debugger/debug_input_port.png");
                    QTreeWidgetItem* portItem = new QTreeWidgetItem(nodeItem);
                    portItem->setText(0, pPort->GetHumanName());
                    portItem->setIcon(0, icon);

                    SBreakpointItem breakPointData;
                    breakPointData.flowgraph = pFlowGraph;
                    breakPointData.addr = breakpoint.addr;
                    breakPointData.type = SBreakpointItem::eIT_Port;

                    v.setValue<SBreakpointItem>(breakPointData);
                    portItem->setData(0, BreakPointItemRole, v);

                    nodeItem->setExpanded(true);
                }
            }
        }
        else
        {
            hGraphItem = new QTreeWidgetItem();
            hGraphItem->setIcon(0, QIcon(":/HyperGraph/debugger/debug_fg.png"));
            hGraphItem->setText(0, pFlowgraph->GetName());
            this->addTopLevelItem(hGraphItem);

            SBreakpointItem breakPointData;
            breakPointData.flowgraph = pFlowGraph;
            breakPointData.addr = breakpoint.addr;
            breakPointData.type = SBreakpointItem::eIT_Graph;
            QVariant v;
            v.setValue<SBreakpointItem>(breakPointData);
            hGraphItem->setData(0, BreakPointItemRole, v);

            CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpoint.addr.node);

            if (pNode)
            {
                QTreeWidgetItem* nodeItem = new QTreeWidgetItem(hGraphItem);
                nodeItem->setIcon(0, QIcon(":/HyperGraph/debugger/debug_node.png"));
                nodeItem->setText(0, pNode->GetUIClassName());
                CHyperNodePort* pPort = pNode->FindPort(breakpoint.addr.port, !breakpoint.addr.isOutput);

                SBreakpointItem breakPointData;
                breakPointData.flowgraph = pFlowGraph;
                breakPointData.addr = breakpoint.addr;
                breakPointData.type = SBreakpointItem::eIT_Node;
                v.setValue<SBreakpointItem>(breakPointData);
                nodeItem->setData(0, BreakPointItemRole, v);

                if (pPort)
                {
                    QIcon icon = breakpoint.addr.isOutput ? QIcon(":/HyperGraph/debugger/debug_output_port.png") : QIcon(":/HyperGraph/debugger/debug_input_port.png");
                    QTreeWidgetItem* portItem = new QTreeWidgetItem(nodeItem);
                    portItem->setIcon(0, icon);
                    portItem->setText(0, pPort->GetHumanName());

                    SBreakpointItem breakPointData;
                    breakPointData.flowgraph = pFlowGraph;
                    breakPointData.addr = breakpoint.addr;
                    breakPointData.type = SBreakpointItem::eIT_Port;

                    v.setValue<SBreakpointItem>(breakPointData);
                    portItem->setData(0, BreakPointItemRole, v);

                    setItemExpanded(hGraphItem, true);
                    setItemExpanded(nodeItem, true);
                }
            }
        }
    }
}

void CBreakpointsTreeCtrl::OnBreakpointRemoved(const SBreakPoint& breakpoint)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);
    QTreeWidgetItem* hGraphItem = FindItemByGraph(this, pFlowGraph);

    if (hGraphItem)
    {
        QTreeWidgetItem* nodeItem = FindItemByNode(this, hGraphItem, breakpoint.addr.node);
        QTreeWidgetItem* portItem = FindItemByPort(this, nodeItem, breakpoint.addr.port, breakpoint.addr.isOutput);

        if (portItem && nodeItem)
        {
            delete portItem;
            if (nodeItem->childCount() == 0)
            {
                delete nodeItem;
            }
            if (hGraphItem->childCount() == 0)
            {
                delete hGraphItem;
            }
        }
    }
}

void CBreakpointsTreeCtrl::OnAllBreakpointsRemovedForNode(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(pFlowGraph);
    QTreeWidgetItem* hGraphItem = FindItemByGraph(this, pFlowGraph);

    if (hGraphItem)
    {
        QTreeWidgetItem* nodeItem = FindItemByNode(this, hGraphItem, nodeID);
        if (nodeItem)
        {
            delete nodeItem;
            if (hGraphItem->childCount() == 0)
            {
                delete hGraphItem;
            }
        }
    }
}

void CBreakpointsTreeCtrl::OnBreakpointHit(const SBreakPoint& breakpoint)
{
    QTreeWidgetItem* hPortItem = GetPortItem(breakpoint);

    if (hPortItem)
    {
        string val;
        breakpoint.value.GetValueWithConversion(val);
        QString itemtext = hPortItem->text(0); // GetItemText(hPortItem);
        hPortItem->setText(0, QString("%1 - (%2)").arg(itemtext).arg(QtUtil::ToQString(val)));
        hPortItem->setIcon(0, QIcon(":/HyperGraph/debugger/debug_breakpoint.png"));
        hPortItem->setSelected(true);
    }
}

void CBreakpointsTreeCtrl::OnAllBreakpointsRemovedForGraph(IFlowGraphPtr pFlowGraph)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(pFlowGraph);
    QTreeWidgetItem* hGraphItem = FindItemByGraph(this, pFlowGraph);

    if (hGraphItem)
    {
        delete hGraphItem;
    }
}

void CBreakpointsTreeCtrl::OnBreakpointInvalidated(const SBreakPoint& breakpoint)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    QTreeWidgetItem* hPortItem = GetPortItem(breakpoint);

    if (hPortItem)
    {
        IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);
        CFlowGraph* pGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);
        CFlowNode* pNode = pGraph->FindFlowNode(breakpoint.addr.node);

        if (pNode)
        {
            CHyperNodePort* pPort = pNode->FindPort(breakpoint.addr.port, !breakpoint.addr.isOutput);

            if (pPort)
            {
                CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);
                CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpoint.addr.node);
                hPortItem->setText(0, pPort->GetHumanName());
            }
        }
        QIcon icon = breakpoint.addr.isOutput ? QIcon(":/HyperGraph/debugger/debug_output_port.png") : QIcon(":/HyperGraph/debugger/debug_input_port.png");
        hPortItem->setIcon(0, icon);
        hPortItem->setSelected(false);
    }
}

void CBreakpointsTreeCtrl::OnEnableBreakpoint(const SBreakPoint& breakpoint, bool enable)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    QTreeWidgetItem* hPortItem = GetPortItem(breakpoint);

    if (hPortItem)
    {
        QIcon defaultIcon = breakpoint.addr.isOutput ? QIcon(":/HyperGraph/debugger/debug_output_port.png") : QIcon(":/HyperGraph/debugger/debug_input_port.png");
        QIcon icon = defaultIcon;

        const bool bIsTracepoint = m_pFlowGraphDebugger->IsTracepoint(breakpoint.flowGraph, breakpoint.addr);

        if (enable && bIsTracepoint)
        {
            icon = QIcon(":/HyperGraph/debugger/debug_tracepoint.png");
        }
        else if (!enable)
        {
            icon = QIcon(":/HyperGraph/debugger/debug_disabled.png");
        }

        hPortItem->setIcon(0, icon);
    }
}

void CBreakpointsTreeCtrl::OnEnableTracepoint(const STracePoint& tracepoint, bool enable)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    QTreeWidgetItem* hPortItem = GetPortItem(tracepoint);
    const bool bIsEnabled = m_pFlowGraphDebugger->IsBreakpointEnabled(tracepoint.flowGraph, tracepoint.addr);

    if (hPortItem && bIsEnabled)
    {
        QIcon defaultIcon = tracepoint.addr.isOutput ? QIcon(":/HyperGraph/debugger/debug_output_port.png") : QIcon(":/HyperGraph/debugger/debug_input_port.png");
        QIcon icon = enable ? QIcon(":/HyperGraph/debugger/debug_tracepoint.png") : defaultIcon;
        hPortItem->setIcon(0, icon);
    }
}

QTreeWidgetItem* CBreakpointsTreeCtrl::GetPortItem(const SBreakPointBase& breakpoint)
{
    if (!m_pFlowGraphDebugger)
    {
        return NULL;
    }

    IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);
    QTreeWidgetItem* hGraphItem = FindItemByGraph(this, pFlowGraph);

    if (hGraphItem)
    {
        QTreeWidgetItem* nodeItem = FindItemByNode(this, hGraphItem, breakpoint.addr.node);
        QTreeWidgetItem* portItem = FindItemByPort(this, nodeItem, breakpoint.addr.port, breakpoint.addr.isOutput);

        return portItem;
    }

    return NULL;
}

void CBreakpointsTreeCtrl::RemoveBreakpointForPort(QTreeWidgetItem* hItem)
{
    QVariant v = hItem->data(0, BreakPointItemRole);
    if (v.isValid())
    {
        SBreakpointItem breakpointItem = v.value<SBreakpointItem>();
        if (m_pFlowGraphDebuggerEditor)
        {
            CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(breakpointItem.flowgraph);
            if (pFlowgraph)
            {
                CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpointItem.addr.node);
                CHyperNodePort* pPort = pNode->FindPort(breakpointItem.addr.port, !breakpointItem.addr.isOutput);
                m_pFlowGraphDebuggerEditor->RemoveBreakpoint(pNode, pPort);
                pNode->Invalidate(true, false);
            }
        }
    }
}

void CBreakpointsTreeCtrl::RemoveBreakpointsForGraph(QTreeWidgetItem* hItem)
{
    QVariant v = hItem->data(0, BreakPointItemRole);
    if (v.isValid())
    {
        SBreakpointItem breakpointItem = v.value<SBreakpointItem>();

        if (m_pFlowGraphDebuggerEditor)
        {
            CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(breakpointItem.flowgraph);
            if (pFlowgraph)
            {
                m_pFlowGraphDebuggerEditor->RemoveAllBreakpointsForGraph(breakpointItem.flowgraph);
            }
        }
    }
}

void CBreakpointsTreeCtrl::RemoveBreakpointsForNode(QTreeWidgetItem* hItem)
{
    QVariant v = hItem->data(0, BreakPointItemRole);
    if (v.isValid())
    {
        SBreakpointItem breakpointItem = v.value<SBreakpointItem>();

        if (m_pFlowGraphDebuggerEditor)
        {
            CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(breakpointItem.flowgraph);
            if (pFlowgraph)
            {
                CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpointItem.addr.node);
                m_pFlowGraphDebuggerEditor->RemoveAllBreakpointsForNode(pNode);
            }
        }
    }
}

void CBreakpointsTreeCtrl::FillBreakpoints()
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    DeleteAllItems();
    DynArray<SBreakPoint> breakpoints;
    if (m_pFlowGraphDebugger->GetBreakpoints(breakpoints))
    {
        DynArray<SBreakPoint>::iterator it = breakpoints.begin();
        DynArray<SBreakPoint>::iterator end = breakpoints.end();

        for (; it != end; ++it)
        {
            OnBreakpointAdded(*it);
        }
    }
}

void CBreakpointsTreeCtrl::EnableBreakpoint(QTreeWidgetItem* hItem, bool enable)
{
    QVariant v = hItem->data(0, BreakPointItemRole);
    if (v.isValid())
    {
        SBreakpointItem breakpointItem = v.value<SBreakpointItem>();

        if (m_pFlowGraphDebuggerEditor)
        {
            CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(breakpointItem.flowgraph);
            if (pFlowgraph)
            {
                CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpointItem.addr.node);
                CHyperNodePort* pPort = pNode->FindPort(breakpointItem.addr.port, !breakpointItem.addr.isOutput);
                m_pFlowGraphDebuggerEditor->EnableBreakpoint(pNode, pPort, enable);
            }
        }
    }
}

void CBreakpointsTreeCtrl::EnableTracepoint(QTreeWidgetItem* hItem, bool enable)
{
    QVariant v = hItem->data(0, BreakPointItemRole);
    if (v.isValid())
    {
        SBreakpointItem breakpointItem = v.value<SBreakpointItem>();

        if (m_pFlowGraphDebuggerEditor)
        {
            CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(breakpointItem.flowgraph);
            if (pFlowgraph)
            {
                CFlowNode* pNode = pFlowgraph->FindFlowNode(breakpointItem.addr.node);
                CHyperNodePort* pPort = pNode->FindPort(breakpointItem.addr.port, !breakpointItem.addr.isOutput);
                m_pFlowGraphDebuggerEditor->EnableTracepoint(pNode, pPort, enable);
            }
        }
    }
}
