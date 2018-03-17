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
#include "CustomActionsEditorManager.h"

#include <IEntitySystem.h>

#include "../HyperGraph/FlowGraph.h"
#include "../HyperGraph/FlowGraphManager.h"
#include "../HyperGraph/HyperGraphDialog.h"
#include "StartupLogoDialog.h"
#include "Objects/EntityObject.h"
#include <ICustomActions.h>
#include <IGameFramework.h>

#include "QtUI/WaitCursor.h"
#include "QtUtil.h"

#include <QMessageBox>

#define GRAPH_FILE_FILTER "Graph XML Files (*.xml)"

//////////////////////////////////////////////////////////////////////////
// Custom Actions Editor Manager
//////////////////////////////////////////////////////////////////////////
CCustomActionsEditorManager::CCustomActionsEditorManager()
{
}

CCustomActionsEditorManager::~CCustomActionsEditorManager()
{
    FreeCustomActionGraphs();
}

void CCustomActionsEditorManager::Init(ISystem* system)
{
    CStartupLogoDialog::SetText("Loading Custom Action Flowgraphs...");

    ReloadCustomActionGraphs();
}

//////////////////////////////////////////////////////////////////////////
void CCustomActionsEditorManager::ReloadCustomActionGraphs()
{
    //  FreeActionGraphs();
    ICustomActionManager* pCustomActionManager = GetISystem()->GetIGame()->GetIGameFramework()->GetICustomActionManager();
    if (!pCustomActionManager)
    {
        return;
    }

    pCustomActionManager->LoadLibraryActions(CUSTOM_ACTIONS_PATH);
    LoadCustomActionGraphs();
}

void CCustomActionsEditorManager::LoadCustomActionGraphs()
{
    ICustomActionManager* pCustomActionManager = GetISystem()->GetIGame()->GetIGameFramework()->GetICustomActionManager();
    if (!pCustomActionManager)
    {
        return;
    }

    CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager();
    CRY_ASSERT(pFlowGraphManager != NULL);

    const size_t numLibraryActions = pCustomActionManager->GetNumberOfCustomActionsFromLibrary();
    for (size_t i = 0; i < numLibraryActions; i++)
    {
        ICustomAction* pCustomAction = pCustomActionManager->GetCustomActionFromLibrary(i);
        CRY_ASSERT(pCustomAction != NULL);
        if (pCustomAction)
        {
            CFlowGraph* pFlowGraph = pFlowGraphManager->FindGraphForCustomAction(pCustomAction);
            if (!pFlowGraph)
            {
                pFlowGraph = pFlowGraphManager->CreateGraphForCustomAction(pCustomAction);
                pFlowGraph->AddRef();
                QString filename(CUSTOM_ACTIONS_PATH);
                filename += '/';
                filename += pCustomAction->GetCustomActionGraphName();
                filename += ".xml";
                pFlowGraph->SetName("");
                pFlowGraph->Load(filename.toUtf8().data());
            }
        }
    }
}

void CCustomActionsEditorManager::SaveAndReloadCustomActionGraphs()
{
    ISystem*    pSystem(GetISystem());
    if (!pSystem)
    {
        return;
    }

    IGame*  pGame(pSystem->GetIGame());
    if (!pGame)
    {
        return;
    }

    IGameFramework* pGameFramework(pGame->GetIGameFramework());
    if (!pGameFramework)
    {
        return;
    }

    ICustomActionManager* pCustomActionManager = pGameFramework->GetICustomActionManager();
    CRY_ASSERT(pCustomActionManager != NULL);
    if (!pCustomActionManager)
    {
        return;
    }

    QString actionName;
    CHyperGraphDialog* pHGDlg = CHyperGraphDialog::instance();
    if (pHGDlg)
    {
        CHyperGraph* pGraph = pHGDlg->GetGraph();
        if (pGraph)
        {
            ICustomAction* pCustomAction = pGraph->GetCustomAction();
            if (pCustomAction)
            {
                actionName = pCustomAction->GetCustomActionGraphName();
                pHGDlg->SetGraph(NULL, true);   // KDAB_PORT view only
            }
        }
    }

    SaveCustomActionGraphs();
    ReloadCustomActionGraphs();

    if (!actionName.isEmpty())
    {
        ICustomAction* pCustomAction = pCustomActionManager->GetCustomActionFromLibrary(actionName.toUtf8().data());
        if (pCustomAction)
        {
            CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
            CFlowGraph* pFlowGraph = pManager->FindGraphForCustomAction(pCustomAction);
            assert(pFlowGraph);
            if (pFlowGraph)
            {
                pManager->OpenView(pFlowGraph);
            }
        }
    }
}

void CCustomActionsEditorManager::SaveCustomActionGraphs()
{
    ICustomActionManager* pCustomActionManager = GetISystem()->GetIGame()->GetIGameFramework()->GetICustomActionManager();
    CRY_ASSERT(pCustomActionManager != NULL);
    if (!pCustomActionManager)
    {
        return;
    }

    WaitCursor waitCursor;

    const size_t numActions = pCustomActionManager->GetNumberOfCustomActionsFromLibrary();
    ICustomAction* pCustomAction;
    for (size_t i = 0; i < numActions; i++)
    {
        pCustomAction = pCustomActionManager->GetCustomActionFromLibrary(i);
        CRY_ASSERT(pCustomAction != NULL);

        CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager();
        CRY_ASSERT(pFlowGraphManager != NULL);

        CFlowGraph* m_pFlowGraph = pFlowGraphManager->FindGraphForCustomAction(pCustomAction);
        if (m_pFlowGraph->IsModified())
        {
            m_pFlowGraph->Save(QString(m_pFlowGraph->GetName() + QStringLiteral(".xml")).toUtf8().data());
            pCustomAction->Invalidate();
        }
    }
}

void CCustomActionsEditorManager::FreeCustomActionGraphs()
{
    CFlowGraphManager* pFGMgr = GetIEditor()->GetFlowGraphManager();
    if (pFGMgr)
    {
        pFGMgr->FreeGraphsForCustomActions();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CCustomActionsEditorManager::NewCustomAction(QString& filename)
{
    AZStd::string directoryName = Path::GetEditingGameDataFolder() + "/" + CUSTOM_ACTIONS_PATH;
    CFileUtil::CreateDirectory(directoryName.c_str());

    QString newFileName;
    if (!CFileUtil::SelectSaveFile(GRAPH_FILE_FILTER, "xml", QString(directoryName.c_str()), newFileName))
    {
        return false;
    }
    filename = newFileName.toLower();

    // check if file exists.
    if (CFileUtil::FileExists(filename))
    {
        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Can't create Custom Action because another Custom Action with this name already exists!\n\nCreation canceled..."));
        return false;
    }

    // Make a new graph.
    CFlowGraphManager* pManager = GetIEditor()->GetFlowGraphManager();
    CRY_ASSERT(pManager != NULL);

    CHyperGraph* pGraph = pManager->CreateGraph();

    CHyperNode* pStartNode = (CHyperNode*) pGraph->CreateNode("CustomAction:Start");
    pStartNode->SetPos(QPointF(80, 10));
    CHyperNode* pSucceedNode = (CHyperNode*) pGraph->CreateNode("CustomAction:Succeed");
    pSucceedNode->SetPos(QPointF(80, 70));
    CHyperNode* pAbortNode = (CHyperNode*) pGraph->CreateNode("CustomAction:Abort");
    pAbortNode->SetPos(QPointF(80, 140));
    CHyperNode* pEndNode = (CHyperNode*) pGraph->CreateNode("CustomAction:End");
    pEndNode->SetPos(QPointF(400, 70));

    pGraph->UnselectAll();

    // Set up the default connections
    pGraph->ConnectPorts(pStartNode, &pStartNode->GetOutputs()->at(0), pEndNode, &pEndNode->GetInputs()->at(1), false);
    pGraph->ConnectPorts(pSucceedNode, &pSucceedNode->GetOutputs()->at(0), pEndNode, &pEndNode->GetInputs()->at(5), false);
    pGraph->ConnectPorts(pAbortNode, &pAbortNode->GetOutputs()->at(0), pEndNode, &pEndNode->GetInputs()->at(6), false);

    bool r = pGraph->Save(filename.toUtf8().data());

    delete pGraph;

    ReloadCustomActionGraphs();

    return r;
}

//////////////////////////////////////////////////////////////////////////
void CCustomActionsEditorManager::GetCustomActions(QStringList& values) const
{
    ICustomActionManager* pCustomActionManager = GetISystem()->GetIGame()->GetIGameFramework()->GetICustomActionManager();
    CRY_ASSERT(pCustomActionManager != NULL);
    if (!pCustomActionManager)
    {
        return;
    }

    values.clear();

    const size_t numCustomActions = pCustomActionManager->GetNumberOfCustomActionsFromLibrary();
    for (size_t i = 0; i < numCustomActions; i++)
    {
        ICustomAction* pCustomAction = pCustomActionManager->GetCustomActionFromLibrary(i);
        CRY_ASSERT(pCustomAction != NULL);

        const char* szActionName = pCustomAction->GetCustomActionGraphName();
        if (szActionName)
        {
            values.push_back(szActionName);
        }
    }
}
