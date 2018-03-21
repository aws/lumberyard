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
#include "FlowGraphModuleManager.h"

#include "../HyperGraph/FlowGraphManager.h"
#include "HyperGraphDialog.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <IFlowGraphDebugger.h>

#include <QPushButton>
#include <QMessageBox>

static const char* FLOWGRAPH_MODULE_PATH_GLOBAL = "Libs/FlowgraphModules";
static const char* FLOWGRAPH_MODULE_PATH_LEVEL = "FlowgraphModules";
static const char* GRAPH_FILE_FILTER = "Graph XML Files (*.xml)";

//////////////////////////////////////////////////////////////////////////
CEditorFlowGraphModuleManager::CEditorFlowGraphModuleManager()
{
}

//////////////////////////////////////////////////////////////////////////
CEditorFlowGraphModuleManager::~CEditorFlowGraphModuleManager()
{
    GetIEditor()->GetLevelIndependentFileMan()->UnregisterModule(this);

    IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();
    if (pModuleManager)
    {
        pModuleManager->UnregisterListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::Init()
{
    GetIEditor()->GetLevelIndependentFileMan()->RegisterModule(this);

    if (gEnv->pFlowSystem == NULL)
    {
        return;
    }

    // get modules from engine module manager, create editor-side graphs to match
    IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();
    if (pModuleManager == NULL)
    {
        return;
    }

    pModuleManager->RegisterListener(this, "CEditorFlowGraphModuleManager");

    CreateEditorFlowgraphs();
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::SaveChangedGraphs()
{
    for (int i = 0; i < m_FlowGraphs.size(); ++i)
    {
        IFlowGraphPtr pGraph = GetModuleFlowGraph(i);
        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
        assert(pFG);
        if (pFG && pFG->IsModified())
        {
            SaveModuleGraph(pFG);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::SaveModuleGraph(CFlowGraph* pFG)
{
    // Saves from the editor have to save the editor flowgraph (not the game side one as
    //  it is not updated with new nodes). So allow the game-side module manager to
    //  add the ports to the xml from the editor flowgraph, then write to file.

    QString filename = gEnv->pFlowSystem->GetIModuleManager()->GetModulePath(pFG->GetName().toUtf8().data());
    filename = Path::GamePathToFullPath(filename);
    XmlNodeRef rootNode = gEnv->pSystem->CreateXmlNode("Graph");

    IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();
    if (pModuleManager)
    {
        pModuleManager->SaveModule(pFG->GetName().toUtf8().data(), rootNode);

        pFG->Serialize(rootNode, false);

        rootNode->saveToFile(filename.toUtf8().data());
    }
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFlowGraphModuleManager::HasModifications()
{
    if (!gEnv->pFlowSystem)
    {
        return false;
    }

    IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();

    if (pModuleManager == NULL)
    {
        return false;
    }

    for (int i = 0; i < m_FlowGraphs.size(); ++i)
    {
        IFlowGraphPtr pGraph = GetModuleFlowGraph(i);
        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
        assert(pFG);
        if (pFG && pFG->IsModified())
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFlowGraphModuleManager::NewModule(QString& filename, IFlowGraphModule::EType type, CHyperGraph** pHyperGraph)
{
    QString folderPath = "";
    QString file = "";
    switch (type)
    {
    case IFlowGraphModule::eT_Global:
        folderPath = Path::GetEditingGameDataFolder().c_str();
        file = FLOWGRAPH_MODULE_PATH_GLOBAL;
        break;
    case IFlowGraphModule::eT_Level:
        folderPath = GetIEditor()->GetLevelFolder();
        file = FLOWGRAPH_MODULE_PATH_LEVEL;
        break;
    default:
        CRY_ASSERT_MESSAGE(false, "Unknown module type encountered!");
        return false;
    }

    QString path(Path::Make(folderPath, file));

    CFileUtil::CreateDirectory(Path::AddPathSlash(path).toUtf8().data());

    QString userFile;
    if (!CFileUtil::SelectSaveFile(GRAPH_FILE_FILTER, "xml", path, userFile))
    {
        return false;
    }
    filename = userFile;

    // create (temporary) default graph for module
    CHyperGraph* pGraph = GetIEditor()->GetFlowGraphManager()->CreateGraph();
    pGraph->AddRef();
    IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager();

    if (pModuleManager == NULL)
    {
        return false;
    }

    QString moduleName = Path::GetFileName(filename);

    if (IFlowGraphModule* pModule = pModuleManager->GetModule(moduleName.toUtf8().data()))
    {
        QMessageBox::warning(AzToolsFramework::GetActiveWindow(),
            QObject::tr("Can not create Flowgraph Module"),
            QObject::tr("Flowgraph Module: %1 already exists. %2 Module")
                .arg(moduleName)
                .arg(IFlowGraphModule::GetTypeName(pModule->GetType())),
            QMessageBox::Close);
        return false;
    }

    // serialize graph to file, with module flag set
    XmlNodeRef root = gEnv->pSystem->CreateXmlNode("Graph");
    root->setAttr("isModule", true);
    root->setAttr("moduleName", moduleName.toUtf8().data());
    pGraph->Serialize(root, false);
    bool saved = root->saveToFile(filename.toUtf8().data());

    pGraph->Release();
    pGraph = NULL;

    if (saved)
    {
        // load module into module manager
        IFlowGraphModule* pModule = pModuleManager->LoadModuleFile(moduleName.toUtf8().data(), filename.toUtf8().data(), type);

        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pModule->GetRootGraph());
        if (pHyperGraph)
        {
            *pHyperGraph = pFG;
        }

        GetIEditor()->GetFlowGraphManager()->ReloadClasses();

        // now create a start node
        string startName = pModuleManager->GetStartNodeName(pFG->GetName().toUtf8().data());
        CHyperNode* pStartNode = (CHyperNode*)pFG->CreateNode(startName.c_str());
        pStartNode->SetPos(QPointF(80, 10));

        // and end node
        string returnName = pModuleManager->GetReturnNodeName(pFG->GetName().toUtf8().data());
        CHyperNode* pReturnNode = (CHyperNode*)pFG->CreateNode(returnName.c_str());
        pReturnNode->SetPos(QPointF(400, 10));

        // and connect them
        pFG->UnselectAll();
        pFG->ConnectPorts(pStartNode, &pStartNode->GetOutputs()->at(0), pReturnNode, &pReturnNode->GetInputs()->at(0), false);
        pFG->ConnectPorts(pStartNode, &pStartNode->GetOutputs()->at(2), pReturnNode, &pReturnNode->GetInputs()->at(1), false);

        SaveModuleGraph(pFG);

        return true;
    }
    else
    {
        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Can't create Flowgraph Module!\nCould not save new file!\n\nCreation canceled..."));
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CEditorFlowGraphModuleManager::PromptChanges()
{
    if (HasModifications())
    {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setWindowTitle(QObject::tr("Flowgraph Module(s) not saved!"));
        mbox.setText(QObject::tr("Some Flowgraph Modules were modified!\nDo you want to save your changes?"));
        const auto cancelBtn = mbox.addButton(QMessageBox::Cancel);
        mbox.addButton(QObject::tr("Don't Save"), QMessageBox::ActionRole);
        const auto saveBtn = mbox.addButton(QMessageBox::Save);
        mbox.setEscapeButton(QMessageBox::Cancel);
        mbox.exec();

        if (mbox.clickedButton() == saveBtn)
        {
            SaveChangedGraphs();
        }
        else if (mbox.clickedButton() == cancelBtn)
        {
            return false;
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphPtr CEditorFlowGraphModuleManager::GetModuleFlowGraph(int index) const
{
    if (index >= 0 && index < m_FlowGraphs.size())
    {
        CFlowGraph* pGraph = m_FlowGraphs[index];

        if (pGraph == NULL)
        {
            return NULL;
        }

        return pGraph->GetIFlowGraph();
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IFlowGraphPtr CEditorFlowGraphModuleManager::GetModuleFlowGraph(const char* name) const
{
    assert(name != NULL);
    if (name == NULL)
    {
        return NULL;
    }

    for (TFlowGraphs::const_iterator iter = m_FlowGraphs.begin(); iter != m_FlowGraphs.end(); ++iter)
    {
        CFlowGraph* pGraph = (*iter);
        if (pGraph == NULL)
        {
            continue;
        }

        if (pGraph->GetName().compare(name, Qt::CaseInsensitive) == 0)
        {
            return pGraph->GetIFlowGraph();
        }
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::DeleteModuleFlowGraph(CFlowGraph* pGraph)
{
    assert(pGraph != NULL);
    if (!pGraph)
    {
        return;
    }

    // 1. delete all caller nodes used in any of the flowgraphs
    // 2. delete the CFlowGraph from the internal list
    // 3. release our CFlowGraph reference
    if (IFlowGraphModuleManager* pModuleManager = gEnv->pFlowSystem->GetIModuleManager())
    {
        QString moduleGraphName = pGraph->GetName();

        if (CFlowGraphManager* pFlowGraphManager = GetIEditor()->GetFlowGraphManager())
        {
            const int numFlowgraphs = pFlowGraphManager->GetFlowGraphCount();
            const char* callerNodeName = pModuleManager->GetCallerNodeName(moduleGraphName.toUtf8().data());

            for (int i = 0; i < numFlowgraphs; ++i)
            {
                CFlowGraph* pFlowgraph = pFlowGraphManager->GetFlowGraph(i);
                IHyperGraphEnumerator* pEnum = pFlowgraph->GetNodesEnumerator();

                for (IHyperNode* pINode = pEnum->GetFirst(); pINode; )
                {
                    CHyperNode* pHyperNode = reinterpret_cast<CHyperNode*>(pINode);
                    if (!pHyperNode->GetClassName().compare(callerNodeName, Qt::CaseInsensitive))
                    {
                        IHyperNode* pTempNode = pEnum->GetNext();
                        pFlowgraph->RemoveNode(pINode);
                        pINode = pTempNode;
                    }
                    else
                    {
                        pINode = pEnum->GetNext();
                    }
                }
                pEnum->Release();
            }
        }
    }

    if (stl::find_and_erase(m_FlowGraphs, pGraph))
    {
        SAFE_RELEASE(pGraph);
    }
}

//////////////////////////////////////////////////////////////////////////
void CEditorFlowGraphModuleManager::CreateModuleNodes(const char* moduleName)
{
    IFlowGraphModuleManager* pManager = gEnv->pFlowSystem->GetIModuleManager();
    IFlowGraphModule* pModule = pManager->GetModule(moduleName);
    if (pModule)
    {
        pManager->CreateModuleNodes(moduleName, pModule->GetId());
        GetIEditor()->GetFlowGraphManager()->ReloadClasses();
    }
}

void CEditorFlowGraphModuleManager::OnModuleDestroyed(IFlowGraphModule* module)
{
    IFlowGraphPtr pRootGraph = module->GetRootGraph();
    CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(pRootGraph);

    if (pFlowgraph)
    {
        DeleteModuleFlowGraph(pFlowgraph);
    }
}

void CEditorFlowGraphModuleManager::OnRootGraphChanged(IFlowGraphModule* module)
{
    CFlowGraph* pFlowgraph = GetIEditor()->GetFlowGraphManager()->FindGraph(module->GetRootGraph());

    if (pFlowgraph)
    {
        return;
    }

    GetIEditor()->GetFlowGraphManager()->ReloadClasses();

    CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->CreateGraphForModule(module);
    assert(pFG);
    if (pFG)
    {
        pFG->AddRef();
        m_FlowGraphs.push_back(pFG);
    }
}

void CEditorFlowGraphModuleManager::OnScannedForModules()
{
    CreateEditorFlowgraphs();
    ReloadHypergraphDialog();
}


void CEditorFlowGraphModuleManager::ReloadHypergraphDialog()
{
    CHyperGraphDialog* pHyperGraphDialog = CHyperGraphDialog::instance();
    if (pHyperGraphDialog)
    {
        GetIEditor()->GetFlowGraphManager()->ReloadClasses();
        pHyperGraphDialog->ReloadGraphs();
        pHyperGraphDialog->ReloadComponents();
        pHyperGraphDialog->ReloadBreakpoints();
    }
}

void CEditorFlowGraphModuleManager::CreateEditorFlowgraphs()
{
    if (m_FlowGraphs.size() > 0)
    {
        TFlowGraphs::iterator iter = m_FlowGraphs.begin();
        for (iter; iter != m_FlowGraphs.end(); ++iter)
        {
            SAFE_RELEASE((*iter))
        }

        m_FlowGraphs.clear();
    }

    IModuleIteratorPtr pIter = gEnv->pFlowSystem->GetIModuleManager()->CreateModuleIterator();
    if (pIter == NULL)
    {
        return;
    }

    IFlowGraphModule* pModule = pIter->Next();
    const int count = pIter->Count();

    if (count == 0)
    {
        return;
    }

    m_FlowGraphs.reserve(count);

    for (int i = 0; i < count; ++i)
    {
        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->CreateGraphForModule(pModule);
        assert(pFG);
        if (pFG)
        {
            pFG->AddRef();

            m_FlowGraphs.push_back(pFG);
        }

        pModule = pIter->Next();
    }
}
