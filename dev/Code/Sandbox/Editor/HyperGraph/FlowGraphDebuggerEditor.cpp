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
#include "FlowGraphDebuggerEditor.h"

#include "MainWindow.h"
#include "FlowGraphManager.h"
#include "HyperGraph.h" //IsFlowgraph check
#include "FlowGraphNode.h"
#include "IGameFramework.h" //Pause game
#include <IInput.h>
#include "HyperGraphDialog.h"
#include "FlowGraph.h"
#include "GameEngine.h" //enable flowsystemupdate in game engine
#include <IFlowGraphModuleManager.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#define VALIDATE_DEBUGGER(a) if (!a) {return false; }
#define VALIDATE_DEBUGGER_VOID(a) if (!a) {return; }

CFlowGraphDebuggerEditor::CFlowGraphDebuggerEditor()
    : m_InGame(false)
    , m_CursorVisible(false)
{
}

CFlowGraphDebuggerEditor::~CFlowGraphDebuggerEditor()
{
}

bool CFlowGraphDebuggerEditor::Init()
{
    if (GetIEditor()->GetFlowGraphManager())
    {
        GetIEditor()->GetFlowGraphManager()->AddListener(this);
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Flowgraph Manager doesn't exist [CFlowgraphDebugger::Init]");
        return false;
    }

    if (gEnv->pFlowSystem)
    {
        m_pFlowGraphDebugger = GetIFlowGraphDebuggerPtr();

        if (m_pFlowGraphDebugger && !m_pFlowGraphDebugger->RegisterListener(this, "CFlowgraphDebuggerEditor"))
        {
            CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Could not register as Flowgraph Debugger listener! [CFlowgraphDebugger::Init]");
        }
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Flowgraph System doesn't exist [CFlowgraphDebugger::Init]");
        return false;
    }

    GetIEditor()->RegisterNotifyListener(this);

    return true;
}

bool CFlowGraphDebuggerEditor::Shutdown()
{
    CFlowGraphManager* pFlowgraphManager = GetIEditor()->GetFlowGraphManager();
    bool bError = false;

    if (pFlowgraphManager)
    {
        pFlowgraphManager->RemoveListener(this);
    }
    else
    {
        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "Flowgraph Manager doesn't exist [CFlowgraphDebugger::~CFlowgraphDebugger]");
        bError = true;
    }

    GetIEditor()->UnregisterNotifyListener(this);

    if (m_pFlowGraphDebugger)
    {
        m_pFlowGraphDebugger->UnregisterListener(this);
    }
    else
    {
        bError = true;
    }

    return !bError;
}

bool CFlowGraphDebuggerEditor::AddBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    SFlowAddress addr;
    addr.node = pFlowNode->GetFlowNodeId();
    addr.port = pHyperNodePort->nPortIndex;
    addr.isOutput = !pHyperNodePort->bInput;

    const bool result = m_pFlowGraphDebugger->AddBreakpoint(pFlowNode->GetIFlowGraph(), addr);

    if (result)
    {
        pFlowNode->Invalidate(true, false);
    }

    return result;
}

bool CFlowGraphDebuggerEditor::RemoveBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    SFlowAddress addr;
    addr.node = pFlowNode->GetFlowNodeId();
    addr.port = pHyperNodePort->nPortIndex;
    addr.isOutput = !pHyperNodePort->bInput;

    const bool result = m_pFlowGraphDebugger->RemoveBreakpoint(pFlowNode->GetIFlowGraph(), addr);

    if (result)
    {
        pFlowNode->Invalidate(true, false);
    }

    return result;
}

bool CFlowGraphDebuggerEditor::EnableBreakpoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort, bool enable)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    SFlowAddress addr;
    addr.node = pFlowNode->GetFlowNodeId();
    addr.port = pHyperNodePort->nPortIndex;
    addr.isOutput = !pHyperNodePort->bInput;

    const bool result = m_pFlowGraphDebugger->EnableBreakpoint(pFlowNode->GetIFlowGraph(), addr, enable);

    if (result)
    {
        pFlowNode->Invalidate(true, false);
    }

    return result;
}

bool CFlowGraphDebuggerEditor::EnableTracepoint(CFlowNode* pFlowNode, CHyperNodePort* pHyperNodePort, bool enable)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    SFlowAddress addr;
    addr.node = pFlowNode->GetFlowNodeId();
    addr.port = pHyperNodePort->nPortIndex;
    addr.isOutput = !pHyperNodePort->bInput;

    const bool result = m_pFlowGraphDebugger->EnableTracepoint(pFlowNode->GetIFlowGraph(), addr, enable);

    if (result)
    {
        pFlowNode->Invalidate(true, false);
    }

    return result;
}

bool CFlowGraphDebuggerEditor::RemoveAllBreakpointsForNode(CFlowNode* pFlowNode)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    const bool result = m_pFlowGraphDebugger->RemoveAllBreakpointsForNode(pFlowNode->GetIFlowGraph(), pFlowNode->GetFlowNodeId());

    CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowNode->GetIFlowGraph());
    if (result && pFlowgraph)
    {
        GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE, pFlowgraph);
    }

    return result;
}

bool CFlowGraphDebuggerEditor::RemoveAllBreakpointsForGraph(IFlowGraphPtr pFlowGraph)
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    const bool result = m_pFlowGraphDebugger->RemoveAllBreakpointsForGraph(pFlowGraph);

    CFlowGraph* pGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);
    if (result && pGraph)
    {
        GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE, pGraph);
    }

    return result;
}

bool CFlowGraphDebuggerEditor::HasBreakpoint(IFlowGraphPtr pFlowGraph, TFlowNodeId nodeID) const
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);
    return m_pFlowGraphDebugger->HasBreakpoint(pFlowGraph, nodeID);
}

bool CFlowGraphDebuggerEditor::HasBreakpoint(CFlowNode* pFlowNode, const CHyperNodePort* pHyperNodePort) const
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);

    SFlowAddress addr;
    addr.node = pFlowNode->GetFlowNodeId();
    addr.port = pHyperNodePort->nPortIndex;
    addr.isOutput = !pHyperNodePort->bInput;

    return m_pFlowGraphDebugger->HasBreakpoint(pFlowNode->GetIFlowGraph(), addr);
}

bool CFlowGraphDebuggerEditor::HasBreakpoint(IFlowGraphPtr pFlowGraph) const
{
    VALIDATE_DEBUGGER(m_pFlowGraphDebugger);
    return m_pFlowGraphDebugger->HasBreakpoint(pFlowGraph);
}

void CFlowGraphDebuggerEditor::PauseGame()
{
    VALIDATE_DEBUGGER_VOID(m_pFlowGraphDebugger);

    const SBreakPoint& breakpoint = m_pFlowGraphDebugger->GetBreakpoint();
    IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);

    CFlowGraph* pGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);

    if (pGraph)
    {
        GetIEditor()->GetFlowGraphManager()->OpenView(pGraph);

        if (CFlowNode* pNode = pGraph->FindFlowNode(breakpoint.addr.node))
        {
            CenterViewAroundNode(pNode);
            pGraph->UnselectAll();
            pNode->SetSelected(true);
        }
    }

    gEnv->pFlowSystem->Enable(false);

    if (gEnv->pGame && (gEnv->pGame->GetIGameFramework()->IsGamePaused() == false))
    {
        gEnv->pGame->GetIGameFramework()->PauseGame(true, true);
    }

    if ((m_CursorVisible == false) && m_InGame)
    {
        AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                        &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                        AzFramework::SystemCursorState::UnconstrainedAndVisible);
        m_CursorVisible = true;
    }
}

void CFlowGraphDebuggerEditor::ResumeGame()
{
    VALIDATE_DEBUGGER_VOID(m_pFlowGraphDebugger);

    if (m_pFlowGraphDebugger->BreakpointHit())
    {
        bool bResume = true;

        const SBreakPoint& breakpoint = m_pFlowGraphDebugger->GetBreakpoint();

        IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);

        if (pFlowGraph)
        {
            bResume = m_pFlowGraphDebugger->RePerformActivation();
        }

        CFlowGraph* pGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);

        if (bResume)
        {
            if (m_CursorVisible)
            {
                AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                                &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                                AzFramework::SystemCursorState::ConstrainedAndHidden);
                m_CursorVisible = false;
            }

            gEnv->pFlowSystem->Enable(true);

            if (gEnv->pGame)
            {
                gEnv->pGame->GetIGameFramework()->PauseGame(false, true);
            }

            // give the focus back to the main view
            MainWindow::instance()->setFocus();
        }

        if (pGraph)
        {
            GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE, pGraph);
        }

        if (gEnv->pInput)
        {
            gEnv->pInput->ClearKeyState();
        }
    }
}

void CFlowGraphDebuggerEditor::CenterViewAroundNode(CFlowNode* pNode) const
{
    CHyperGraphDialog* pHyperGraphDialog = CHyperGraphDialog::instance();
    if (pHyperGraphDialog)
    {
        pHyperGraphDialog->CenterViewAroundNode(pNode, false, 0.95f);
        // we need the focus for the graph view (F5 to continue...)
        pHyperGraphDialog->SetFocusOnView();
    }
}

void CFlowGraphDebuggerEditor::OnHyperGraphManagerEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode)
{
    if (!pNode)
    {
        return;
    }

    switch (event)
    {
    case EHG_NODE_DELETE:
    {
        CHyperGraph* pHyperGraph = static_cast<CHyperGraph*>(pNode->GetGraph());
        // we have to check whether a deleted flownode has actually some breakpoints
        // and remove them too
        if (pHyperGraph->IsFlowGraph())
        {
            CFlowNode* pFlowNode = static_cast<CFlowNode*>(pNode);
            if (pFlowNode->IsFlowNode() && HasBreakpoint(pFlowNode->GetIFlowGraph(), pFlowNode->GetFlowNodeId()))
            {
                RemoveAllBreakpointsForNode(pFlowNode);
            }
        }
    }
    break;
    }
}

void CFlowGraphDebuggerEditor::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginLoad:
    {
        if (m_pFlowGraphDebugger)
        {
            m_pFlowGraphDebugger->ClearBreakpoints();
        }

        CHyperGraphDialog* pHyperGraphDialog = CHyperGraphDialog::instance();
        if (pHyperGraphDialog)
        {
            pHyperGraphDialog->ClearBreakpoints();
            pHyperGraphDialog->InvalidateView(true);
        }
    }
    break;
    case eNotify_OnBeginGameMode:
    {
        if (m_CursorVisible)
        {
            AzFramework::InputSystemCursorRequestBus::Event(AzFramework::InputDeviceMouse::Id,
                                                            &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                                                            AzFramework::SystemCursorState::ConstrainedAndVisible);
            m_CursorVisible = false;
        }

        m_InGame = true;
    }
    break;
    case eNotify_OnEndGameMode:
    case eNotify_OnEndSimulationMode:
    {
        if (m_pFlowGraphDebugger)
        {
            if (m_pFlowGraphDebugger->BreakpointHit())
            {
                ResumeGame();
            }

            m_pFlowGraphDebugger->EnableStepMode(false);
        }
        m_InGame = false;
    }
    break;
    }
}

void CFlowGraphDebuggerEditor::OnBreakpointHit(const SBreakPoint& breakpoint)
{
    VALIDATE_DEBUGGER_VOID(m_pFlowGraphDebugger);

    IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);
    CFlowGraph* pGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);

    if (pGraph)
    {
        GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE, pGraph);
    }

    PauseGame();
}

void CFlowGraphDebuggerEditor::OnBreakpointInvalidated(const SBreakPoint& breakpoint)
{
    VALIDATE_DEBUGGER_VOID(m_pFlowGraphDebugger);

    IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(breakpoint.flowGraph);
    CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);

    if (pFlowgraph)
    {
        GetIEditor()->GetFlowGraphManager()->SendNotifyEvent(EHG_GRAPH_INVALIDATE, pFlowgraph);
    }
}

void CFlowGraphDebuggerEditor::OnTracepointHit(const STracePoint& tracepoint)
{
    VALIDATE_DEBUGGER_VOID(m_pFlowGraphDebugger);

    IFlowGraphPtr pFlowGraph = m_pFlowGraphDebugger->GetRootGraph(tracepoint.flowGraph);
    CFlowGraph* pGraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pFlowGraph);

    if (pGraph)
    {
        if (CFlowNode* pNode = pGraph->FindFlowNode(tracepoint.addr.node))
        {
            CHyperNodePort* pPort = pNode->FindPort(tracepoint.addr.port, !tracepoint.addr.isOutput);

            if (pPort)
            {
                string val;
                tracepoint.value.GetValueWithConversion(val);

                QByteArray portName = pNode->GetPortName(*pPort).toUtf8();
                QByteArray className = pNode->GetClassName().toUtf8();
                QByteArray graphName = pGraph->GetName().toUtf8();
                CryLog("$2[TRACEPOINT HIT - FrameID: %d] GRAPH: %s (ID: %d) - NODE: %s (ID: %d) - PORT: %s - VALUE: %s", gEnv->pRenderer->GetFrameID(), graphName.data(), pGraph->GetIFlowGraph()->GetGraphId(), className.data(), pNode->GetId(), portName.data(), val.c_str());
            }
        }
    }
}
