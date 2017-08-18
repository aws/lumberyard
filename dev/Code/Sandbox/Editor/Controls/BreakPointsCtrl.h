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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_BREAKPOINTSCTRL_H
#define CRYINCLUDE_EDITOR_CONTROLS_BREAKPOINTSCTRL_H
#pragma once


#include "IFlowGraphDebugger.h"
#include <QTreeWidget>

class CFlowGraphDebuggerEditor;

struct SBreakpointItem
{
    enum EItemType
    {
        eIT_Graph = 0,
        eIT_Node,
        eIT_Port
    };

    IFlowGraphPtr flowgraph;
    SFlowAddress addr;
    EItemType type;
};
Q_DECLARE_METATYPE(SBreakpointItem);

class CBreakpointsTreeCtrl
    : public QTreeWidget
    , IFlowGraphDebugListener
{
public:
    enum
    {
        BreakPointItemRole = Qt::UserRole
    };

    CBreakpointsTreeCtrl(QWidget* parent = 0);
    virtual ~CBreakpointsTreeCtrl();

    // IFlowgraphDebugListener
    void OnBreakpointAdded(const SBreakPoint& breakpoint);
    void OnBreakpointRemoved(const SBreakPoint& breakpoint);
    void OnAllBreakpointsRemovedForNode(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID);
    void OnAllBreakpointsRemovedForGraph(IFlowGraphPtr pFlowGraph);
    void OnBreakpointHit(const SBreakPoint& breakpoint);
    void OnTracepointHit(const STracePoint& tracepoint){};
    void OnBreakpointInvalidated(const SBreakPoint& breakpoint);
    void OnEnableBreakpoint(const SBreakPoint& breakpoint, bool enable);
    void OnEnableTracepoint(const STracePoint& tracepoint, bool enable);
    //~ IFlowgraphDebugListener

    void RemoveBreakpointsForGraph(QTreeWidgetItem* hItem);
    void RemoveBreakpointsForNode(QTreeWidgetItem* hItem);
    void RemoveBreakpointForPort(QTreeWidgetItem* hItem);
    void EnableBreakpoint(QTreeWidgetItem* hItem, bool enable);
    void EnableTracepoint(QTreeWidgetItem* hItem, bool enable);

    void DeleteAllItems();
    void FillBreakpoints();

    QTreeWidgetItem* GetNextItem(QTreeWidgetItem* hItem);
    QTreeWidgetItem* FindItemByGraph(QTreeWidget* pTreeCtrl, IFlowGraphPtr pFlowgraph);
    QTreeWidgetItem* FindItemByNode(QTreeWidget* pTreeCtrl, QTreeWidgetItem* hGraphItem, TFlowNodeId nodeId);
    QTreeWidgetItem* FindItemByPort(QTreeWidget* pTreeCtrl, QTreeWidgetItem* hNodeItem, TFlowPortId portId, bool isOutput);

private:
    QTreeWidgetItem* GetPortItem(const SBreakPointBase& breakpoint);

    IFlowGraphDebuggerPtr m_pFlowGraphDebugger;
    CFlowGraphDebuggerEditor* m_pFlowGraphDebuggerEditor;
};

#endif // CRYINCLUDE_EDITOR_CONTROLS_BREAKPOINTSCTRL_H
