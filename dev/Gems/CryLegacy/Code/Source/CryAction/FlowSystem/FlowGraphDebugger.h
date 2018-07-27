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

#ifndef CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWGRAPHDEBUGGER_H
#define CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWGRAPHDEBUGGER_H
#pragma once

#if defined(INCLUDE_FLOWGRAPHDEBUGGER_EXTENSION)

#include "CryListenerSet.h"
#include <IFlowGraphDebugger.h>
#include <CryExtension/Impl/ClassWeaver.h>


class CFlowGraphDebugger
    : public IFlowGraphDebugger
{
    CRYINTERFACE_SIMPLE(IFlowGraphDebugger)
    CRYGENERATE_SINGLETONCLASS(CFlowGraphDebugger, IFlowGraphDebuggerExtensionName, 0x2D6810E10CBD467A, 0xB5646539C3778285)

public:

    // IFlowGraphDebugger
    virtual bool AddBreakpoint(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr);
    virtual bool RemoveBreakpoint(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr);
    virtual bool RemoveAllBreakpointsForNode(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID);
    virtual bool RemoveAllBreakpointsForGraph(IFlowGraphPtr pFlowgraph);
    virtual bool HasBreakpoint(IFlowGraphPtr pFlowGraph, const SFlowAddress& addr) const;
    virtual bool HasBreakpoint(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID) const;
    virtual bool HasBreakpoint(IFlowGraphPtr pFlowGraph) const;
    virtual bool EnableBreakpoint(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr, bool enable);
    virtual bool EnableTracepoint(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr, bool enable);
    virtual bool IsBreakpointEnabled(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr) const;
    virtual bool IsTracepoint(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr) const;
    virtual const SBreakPoint& GetBreakpoint() const {return m_BreakPoint; };
    virtual void InvalidateBreakpoint();
    virtual bool BreakpointHit() const;
    virtual bool RegisterListener(IFlowGraphDebugListener* pListener, const char* name);
    virtual void UnregisterListener(IFlowGraphDebugListener* pListener);
    virtual bool PerformActivation(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr, const TFlowInputData& value);
    virtual bool PerformActivation(IFlowGraphPtr pFlowgraph, int edgeIndex, const SFlowAddress& fromAddr, const SFlowAddress& toAddr, const TFlowInputData& value);
    virtual bool RePerformActivation();
    virtual void ClearBreakpoints();
    virtual bool GetBreakpoints(DynArray<SBreakPoint>& breakpointsDynArray);
    virtual IFlowGraphPtr GetRootGraph(IFlowGraphPtr pFlowGraph) const;
    virtual void EnableStepMode(bool bEnableStepMode){m_bStepModeEnabled = bEnableStepMode; }
    virtual bool IsStepModeEnabled(){return m_bStepModeEnabled; }
    virtual bool AddIgnoredFlowgraph(IFlowGraphPtr pFlowgraph);
    virtual bool RemoveIgnoredFlowgraph(IFlowGraphPtr pFlowgraph);
    virtual bool IsFlowgraphIgnored(IFlowGraphPtr pFlowgraph);
    virtual bool IgnoreFlowgraphType(IFlowGraph::EFlowGraphType type, bool bIgnore);
    virtual bool IsFlowgraphTypeIgnored(IFlowGraph::EFlowGraphType type);
    // ~IFlowgraphDebugger

private:

    bool CheckForValidIDs(TFlowNodeId nodeID, TFlowPortId portID) const;
    bool CheckForDelayedActivations();
    bool TryActivatePort(SBreakPoint& breakpoint, IFlowEdgeIterator::Edge& edge, bool& bPerform, int edgeIndex);

    void FillDynArray(DynArray<SBreakPoint>& breakpointsDynArray, TFlowPortIDS portIDS, IFlowGraphPtr pFlowgraph, TFlowNodeId nodeID, bool bIsOutput);

    const SBreakPointPortInfo* GetBreakpointInfo(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr) const;
    SBreakPointPortInfo* GetBreakpointInfo(IFlowGraphPtr pFlowgraph, const SFlowAddress& addr);

    TDebugInfo m_DebugInfo;
    bool m_BreakpointHit;
    bool m_RePerformActivation;
    bool m_bStepModeEnabled;
    SBreakPoint m_BreakPoint;

    typedef std::deque<SBreakPoint> TDelayedActivations;
    TDelayedActivations m_DelayedActivations;

    typedef std::vector<IFlowGraphPtr> TIgnoredFlowgraphs;
    TIgnoredFlowgraphs m_IgnoredFlowgraphs;

    typedef std::vector<IFlowGraph::EFlowGraphType> TIgnoredFlowgraphTypes;
    TIgnoredFlowgraphTypes m_IgnoredFlowgraphTypes;

    CListenerSet<IFlowGraphDebugListener*> m_Listeners;
};

#endif
#endif // CRYINCLUDE_CRYACTION_FLOWSYSTEM_FLOWGRAPHDEBUGGER_H
