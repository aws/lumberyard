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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHDEBUGGEREDITOR_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHDEBUGGEREDITOR_H
#pragma once

#include "IHyperGraph.h" //IHyperGraphManagerListener
#include <IFlowGraphDebugger.h> //IFlowGraphDebugListener


class CFlowGraphDebuggerEditor
    : public IFlowGraphDebugListener
    , IHyperGraphManagerListener
    , IEditorNotifyListener
{
public:
    CFlowGraphDebuggerEditor();
    virtual ~CFlowGraphDebuggerEditor();

    bool Init();
    bool Shutdown();

    bool AddBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort);
    bool RemoveBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort);
    bool RemoveAllBreakpointsForNode(CFlowNode* pFlowNode);
    bool RemoveAllBreakpointsForGraph(IFlowGraphPtr pFlowGraph);
    bool HasBreakpoint(CFlowNode* pFlowNode, const CHyperNodePort* pHyperNodePort) const;
    bool HasBreakpoint(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID) const;
    bool HasBreakpoint(IFlowGraphPtr pFlowGraph) const;
    bool EnableBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort, bool enable);
    bool EnableTracepoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort, bool enable);
    void PauseGame();
    void ResumeGame();

    // IHyperGraphManagerListener
    virtual void OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode);
    // ~IHyperGraphManagerListener

    // IEditorNotifyListener
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);
    // ~IEditorNotifyListener

    // IFlowgraphDebugListener
    virtual void OnBreakpointAdded(const SBreakPoint& breakpoint){}
    virtual void OnBreakpointRemoved(const SBreakPoint& breakpoint){}
    virtual void OnAllBreakpointsRemovedForNode(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID){}
    virtual void OnAllBreakpointsRemovedForGraph(IFlowGraphPtr pFlowGraph){}
    virtual void OnBreakpointHit(const SBreakPoint& breakpoint);
    virtual void OnTracepointHit(const STracePoint& tracepoint);
    virtual void OnBreakpointInvalidated(const SBreakPoint& breakpoint);
    virtual void OnEnableBreakpoint(const SBreakPoint& breakpoint, bool enable){}
    virtual void OnEnableTracepoint(const STracePoint& tracepoint, bool enable){}
    // ~IFlowgraphDebugListener

private:
    void CenterViewAroundNode(CFlowNode* pNode) const;

    bool m_InGame;
    bool m_CursorVisible;
    IFlowGraphDebuggerPtr m_pFlowGraphDebugger;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHDEBUGGEREDITOR_H
