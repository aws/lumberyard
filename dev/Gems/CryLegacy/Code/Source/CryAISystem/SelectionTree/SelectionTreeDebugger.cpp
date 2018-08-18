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

#include "CryLegacy_precompiled.h"
#include "SelectionTreeDebugger.h"
#include "SelectionTreeTemplate.h"

#ifndef RELEASE

CSelectionTreeDebugger* CSelectionTreeDebugger::GetInstance()
{
    static CSelectionTreeDebugger instance;
    return &instance;
}

CSelectionTreeDebugger::CSelectionTreeDebugger()
    : m_pObserver(NULL)
    , m_AI(NULL)
{
}

void CSelectionTreeDebugger::StartEvaluation(const CAIActor* pActor, const SelectionTree& tree, const SelectionVariables& vars)
{
    m_AI = NULL;
    if (m_pObserver && pActor->GetName() == m_AIName)
    {
        m_AI = pActor;

        const char* treename = tree.GetTemplate().GetName();
        assert(treename);

        if (m_CurrTree != treename)
        {
            ISelectionTreeObserver::STreeNodeInfo root;
            ReadNodeInfo(tree, root, tree.GetNodeAt(0));
            m_pObserver->SetSelectionTree(treename, root);
            m_CurrTree = treename;
        }

        ISelectionTreeObserver::TVariableStateMap varsDump;
        const SelectionVariableDeclarations::VariableDescriptions& varDesc = tree.GetTemplate().GetVariableDeclarations().GetDescriptions();
        for (SelectionVariableDeclarations::VariableDescriptions::const_iterator it = varDesc.begin(); it != varDesc.end(); ++it)
        {
            bool value;
            const SelectionVariableID id = it->first;
            const char* name = it->second.name.c_str();
            bool ok = vars.GetVariable(id, &value);
            assert(ok);
            varsDump[name] = value;
        }

        if (m_pObserver)
        {
            m_pObserver->DumpVars(varsDump);
            m_pObserver->StartEval();
        }
    }
}

void CSelectionTreeDebugger::EvaluateNode(const SelectionTreeNode& node)
{
    if (m_AI && m_pObserver)
    {
        m_pObserver->EvalNode(node.GetNodeID());
    }
}

void CSelectionTreeDebugger::EvaluateNodeCondition(const SelectionTreeNode& node, bool condition)
{
    if (m_AI && m_pObserver)
    {
        m_pObserver->EvalNodeCondition(node.GetNodeID(), condition);
    }
}

void CSelectionTreeDebugger::EvaluateStateCondition(const SelectionTreeNode& node, bool condition)
{
    if (m_AI && m_pObserver)
    {
        m_pObserver->EvalStateCondition(node.GetNodeID(), condition);
    }
}

void CSelectionTreeDebugger::EndEvaluation(const CAIActor* pActor, SelectionNodeID currentNodeId)
{
    if (m_AI == pActor && m_pObserver)
    {
        m_pObserver->StopEval(currentNodeId);
    }
    m_AI = NULL;
}

void CSelectionTreeDebugger::SetObserver(ISelectionTreeObserver* pListener, const char* nameAI)
{
    m_pObserver = pListener;
    m_AI = NULL;
    m_AIName = nameAI;
    m_CurrTree = "";
}

void CSelectionTreeDebugger::ReadNodeInfo(const SelectionTree& tree, ISelectionTreeObserver::STreeNodeInfo& outNode, const SelectionTreeNode& inNode)
{
    outNode.Id = inNode.GetNodeID();
    outNode.Childs.resize(inNode.GetChildren().size());
    int i = 0;
    for (SelectionTreeNode::Children::const_iterator it = inNode.GetChildren().begin(); it != inNode.GetChildren().end(); ++it)
    {
        ReadNodeInfo(tree, outNode.Childs[i++], tree.GetNode(it->childID));
    }
}

#endif