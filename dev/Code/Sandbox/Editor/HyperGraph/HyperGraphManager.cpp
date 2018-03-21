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
#include "HyperGraphManager.h"
#include "HyperGraphNode.h"
#include "HyperGraph.h"
#include "HyperGraphDialog.h"
#include "HyperGraphWidget.h"
#include "CommentNode.h"
#include "CommentBoxNode.h"
#include "QuickSearchNode.h"
#include "BlackBoxNode.h"
#include "MissingNode.h"
#include "GameEngine.h"

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::Init()
{
    m_notifyListenersDisabled = false;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::OpenView(IHyperGraph* pGraph)
{
    // Open view if it does exist already
    if (!CHyperGraphDialog::instance())
    {
        QtViewPaneManager::instance()->OpenPane(LyViewPane::LegacyFlowGraph);
    }
    if (CHyperGraphDialog::instance())
    {
        CHyperGraphDialog::instance()->SetGraph((CHyperGraph*)pGraph);
    }
}

//////////////////////////////////////////////////////////////////////////
CHyperNode* CHyperGraphManager::CreateNode(CHyperGraph* pGraph, const char* sNodeClass, HyperNodeID nodeId, const QPointF& /* pos */, CBaseObject* pObj, bool bAllowMissing)
{
    // AlexL: ignore pos, as a SetPos would screw up Undo. And it will be set afterwards anyway
    CHyperNode* pNode = NULL;
    CHyperNode* pPrototype = GetPrototypeByTag(sNodeClass, true);
    if (pPrototype)
    {
        pNode = pPrototype->Clone();
        pNode->m_id = nodeId;
        pNode->SetGraph(pGraph);
        pNode->Init();
        return pNode;
    }

    //////////////////////////////////////////////////////////////////////////
    // Hack for naming changes
    //////////////////////////////////////////////////////////////////////////
    QString newtype;
    // Hack for Math: to Vec3: change.
    if (strncmp(sNodeClass, "Math:", 5) == 0)
    {
        newtype = QStringLiteral("Vec3:") + QString(sNodeClass).mid(5);
    }

    if (newtype.size() > 0)
    {
        pPrototype = stl::find_in_map(m_prototypes, newtype, NULL);
        if (pPrototype)
        {
            pNode = pPrototype->Clone();
            pNode->m_id = nodeId;
            pNode->SetGraph(pGraph);
            pNode->Init();
            return pNode;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    if (0 == strcmp(sNodeClass, CCommentNode::GetClassType()))
    {
        pNode = new CCommentNode();
        pNode->m_id = nodeId;
        pNode->SetGraph(pGraph);
        pNode->Init();
        return pNode;
    }
    else if (0 == strcmp(sNodeClass, CCommentBoxNode::GetClassType()))
    {
        pNode = new CCommentBoxNode();
        pNode->m_id = nodeId;
        pNode->SetGraph(pGraph);
        pNode->Init();
        return pNode;
    }
    else if (0 == strcmp(sNodeClass, CBlackBoxNode::GetClassType()))
    {
        pNode = new CBlackBoxNode();
        pNode->m_id = nodeId;
        pNode->SetGraph(pGraph);
        pNode->Init();
        return pNode;
    }
    else if (0 == strcmp(sNodeClass, CQuickSearchNode::GetClassType()))
    {
        pNode = new CQuickSearchNode();
        pNode->m_id = nodeId;
        pNode->SetGraph(pGraph);
        pNode->Init();
        return pNode;
    }
    else if (bAllowMissing)
    {
        gEnv->pLog->LogError("Missing Node: %s, Referenced in FlowGraph %s", sNodeClass, pGraph->GetName().toUtf8().data());
        pNode = new CMissingNode(sNodeClass);
        pNode->m_id = nodeId;
        pNode->SetGraph(pGraph);
        return pNode;
    }

    gEnv->pLog->LogError("Node of class '%s' doesn't exist -> Node creation failed", sNodeClass);
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
CHyperNode* CHyperGraphManager::GetPrototypeByTag(const char* sNodeClass, bool checkUIName)
{
    QString actualNodeName = (checkUIName ? gEnv->pFlowSystem->GetClassTagFromUIName(CONST_TEMP_STRING(sNodeClass)).c_str() : sNodeClass);
    for (NodesPrototypesMap::iterator it = m_prototypes.begin(); it != m_prototypes.end(); ++it)
    {
        CHyperNode* pNode = it->second;
        if (pNode->GetClassName() == actualNodeName)
        {
            return pNode;
        }
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::GetPrototypes(QStringList& prototypes, bool bForUI, NodeFilterFunctor filterFunc)
{
    prototypes.clear();
    for (NodesPrototypesMap::iterator it = m_prototypes.begin(); it != m_prototypes.end(); ++it)
    {
        CHyperNode* pNode = it->second;
        if (filterFunc && filterFunc (pNode) == false)
        {
            continue;
        }
        if (bForUI && (pNode->CheckFlag(EHYPER_NODE_HIDE_UI)))
        {
            continue;
        }
        prototypes.push_back(bForUI ? pNode->GetUIClassName() : pNode->GetClassName());
    }
#if 0
    if (bForUI)
    {
        prototypes.push_back(CCommentNode::GetClassType());
        prototypes.push_back(CCommentBoxNode::GetClassType());
        prototypes.push_back(CBlackBoxNode::GetClassType());
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::GetPrototypesEx(std::vector<THyperNodePtr>& prototypes, bool bForUI, NodeFilterFunctor filterFunc /* = NodeFilterFunctor( */)
{
    prototypes.clear();
    for (NodesPrototypesMap::iterator it = m_prototypes.begin(); it != m_prototypes.end(); ++it)
    {
        CHyperNode* pNode = it->second;
        if (filterFunc && filterFunc (pNode) == false)
        {
            continue;
        }
        if (bForUI && (pNode->CheckFlag(EHYPER_NODE_HIDE_UI)))
        {
            continue;
        }
        prototypes.push_back (it->second);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::SendNotifyEvent(EHyperGraphEvent event, IHyperGraph* pGraph, IHyperNode* pNode)
{
    if (m_notifyListenersDisabled)
    {
        return;
    }

    Listeners::iterator next;
    for (Listeners::iterator it = m_listeners.begin(); it != m_listeners.end(); it = next)
    {
        next = it;
        next++;
        (*it)->OnHyperGraphManagerEvent(event, pGraph, pNode);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::SetGUIControlsProcessEvents(bool active, bool refreshTreeCtrList)
{
    CHyperGraphDialog* pHGDlg = CHyperGraphDialog::instance();
    if (pHGDlg)
    {
        pHGDlg->GetGraphsTreeCtrl()->SetIgnoreReloads(!active);
        pHGDlg->GetGraphWidget()->SetIgnoreHyperGraphEvents(!active);

        if (refreshTreeCtrList)
        {
            pHGDlg->ReloadGraphs();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::SetCurrentViewedGraph(CHyperGraph* pGraph)
{
    CHyperGraphDialog* pHGDlg = CHyperGraphDialog::instance();
    if (pHGDlg)
    {
        pHGDlg->GetGraphWidget()->SetGraph(pGraph);
    }
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::AddListener(IHyperGraphManagerListener* pListener)
{
    stl::push_back_unique(m_listeners, pListener);
}

//////////////////////////////////////////////////////////////////////////
void CHyperGraphManager::RemoveListener(IHyperGraphManagerListener* pListener)
{
    stl::find_and_erase(m_listeners, pListener);
}
