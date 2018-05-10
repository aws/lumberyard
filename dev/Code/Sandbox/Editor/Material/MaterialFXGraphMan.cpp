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
#include "MaterialFXGraphMan.h"

#include "../HyperGraph/FlowGraph.h"
#include "../HyperGraph/HyperGraphDialog.h"
#include "../HyperGraph/FlowGraphManager.h"
#include "../HyperGraph/FlowGraphModuleDlgs.h"

#include <IMaterialEffects.h>

#include <QPushButton>
#include <QMessageBox>

#define MATERIAL_FX_PATH QStringLiteral("Libs/MaterialEffects/FlowGraphs")
#define GRAPH_FILE_FILTER "Graph XML Files (*.xml)"

namespace
{
    //! Returns the relative path, case insensitive, strips out leading or trailing path separator.
    //! /param rootPath The root path to use to make a relative path from.
    //! /param fullPath The desired path to make relative.
    //! /return The relative path without leading and trailing path separator
    AZStd::string GetRelativePath(const AZStd::string& rootPath, const AZStd::string& fullPath)
    {
        AZStd::string normalizedRootPath = rootPath;
        AzFramework::StringFunc::Path::Normalize(normalizedRootPath);

        AZStd::string normalizedFilePath = fullPath;
        AzFramework::StringFunc::Path::Normalize(normalizedFilePath);

        AZStd::string relativePath;

        // Get the relative path we need to create folders in the flowgraph treeview.
        AzFramework::StringFunc::Path::GetFullPath(normalizedFilePath.c_str(), relativePath);

        // Strip the root path
        AzFramework::StringFunc::Replace(relativePath, normalizedRootPath.c_str(), "");

        if (AzFramework::StringFunc::FirstCharacter(relativePath.c_str()) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
        {
            relativePath = relativePath.substr(1, relativePath.size());
        }

        if (AzFramework::StringFunc::LastCharacter(relativePath.c_str()) == AZ_CORRECT_FILESYSTEM_SEPARATOR)
        {
            relativePath = relativePath.substr(0, relativePath.size() - 1);
        }

        return relativePath;
    }

    //! Gets the default group name of the flowgraph depending on its path.
    //! /param rootPath The root path where the flowgraph is found, used to make a relative path.
    //! /param filePath The full path to the flowgraph file.
    //! /return The name of the group based on the flowgraph's path.
    //! /remark If sub folders are more than 1 level deep, only the first sub-folder name is used as the group name.
    AZStd::string GetDefaultGroupName(const AZStd::string& rootPath, const AZStd::string& filePath)
    {
        AZStd::string relativePath = GetRelativePath(rootPath, filePath);

        // Currently the behavior of all flowgraph modules is to only use the first level sub-folder
        // as the group name, so we retain this behavior.
        AZStd::vector<AZStd::string> tokens;
        AzFramework::StringFunc::Tokenize(relativePath.c_str(), tokens, AZ_CORRECT_FILESYSTEM_SEPARATOR);
        if (!tokens.empty())
        {
            return tokens[0].c_str();
        }

        return "";
    }
}

CMaterialFXGraphMan::CMaterialFXGraphMan()
{
}

CMaterialFXGraphMan::~CMaterialFXGraphMan()
{
    GetIEditor()->GetLevelIndependentFileMan()->UnregisterModule(this);
}

void CMaterialFXGraphMan::Init()
{
    GetIEditor()->GetLevelIndependentFileMan()->RegisterModule(this);
    ReloadFXGraphs();
}

void CMaterialFXGraphMan::ReloadFXGraphs()
{
    if (!gEnv->pMaterialEffects)
    {
        return;
    }

    ClearEditorGraphs();

    gEnv->pMaterialEffects->ReloadMatFXFlowGraphs(true);

    for (int i = 0; i < gEnv->pMaterialEffects->GetMatFXFlowGraphCount(); ++i)
    {
        string filename;
        IFlowGraphPtr pGraph = gEnv->pMaterialEffects->GetMatFXFlowGraph(i, &filename);
        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->CreateGraphForMatFX(pGraph, filename.c_str());
        assert(pFG);
        if (pFG)
        {
            pFG->AddRef();
            m_matFxGraphs.push_back(pGraph);
        }
    }
}

void CMaterialFXGraphMan::SaveChangedGraphs()
{
    if (!gEnv->pMaterialEffects)
    {
        return;
    }

    for (int i = 0; i < gEnv->pMaterialEffects->GetMatFXFlowGraphCount(); ++i)
    {
        IFlowGraphPtr pGraph = gEnv->pMaterialEffects->GetMatFXFlowGraph(i);
        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
        assert(pFG);
        if (pFG && pFG->IsModified())
        {
            QString filename(MATERIAL_FX_PATH);
            filename += '/';
            filename += pFG->GetName();
            filename += ".xml";
            pFG->Save(filename.toUtf8().data());
        }
    }
}

bool CMaterialFXGraphMan::HasModifications()
{
    if (!gEnv->pMaterialEffects)
    {
        return false;
    }

    for (int i = 0; i < gEnv->pMaterialEffects->GetMatFXFlowGraphCount(); ++i)
    {
        IFlowGraphPtr pGraph = gEnv->pMaterialEffects->GetMatFXFlowGraph(i);
        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
        assert(pFG);
        if (pFG && pFG->IsModified())
        {
            return true;
        }
    }
    return false;
}

bool CMaterialFXGraphMan::NewMaterialFx(QString& filename, CHyperGraph** pHyperGraph)
{
    if (!gEnv->pMaterialEffects)
    {
        return false;
    }

    const QString materialFXPath(MATERIAL_FX_PATH);
    const AZStd::string flowGraphMaterialFxRootPath = AZStd::string::format("@devassets@/%s", materialFXPath.toStdString().c_str());

    char assetPath[AZ_MAX_PATH_LEN] = { 0 };
    gEnv->pFileIO->ResolvePath(flowGraphMaterialFxRootPath.c_str(), assetPath, AZ_MAX_PATH_LEN);

    // Ensure the path exists
    if (!gEnv->pFileIO->IsDirectory(assetPath))
    {
        gEnv->pFileIO->CreatePath(assetPath);
    }

    // Ask the user for the file path and name.
    QString file;
    if (!CFileUtil::SelectSaveFile(GRAPH_FILE_FILTER, "xml", assetPath, file))
    {
        return false;
    }
    filename = file;

    // Ensure that we save this flowgraph on the correct root folder.
    AZStd::string targetFilename = filename.toUtf8().data();
    AZStd::string targetPath = assetPath;

    AzFramework::StringFunc::Path::Normalize(targetFilename);
    AzFramework::StringFunc::Path::Normalize(targetPath);

    if (!AzFramework::StringFunc::Path::IsFileInFolder(targetFilename.c_str(), targetPath.c_str(), true, true, false))
    {
        AZStd::string error = AZStd::string::format("Material FX Graphs must be within: %s", targetPath.c_str());
        QMessageBox::information(QApplication::activeWindow(), QString(), error.c_str());
        return false;
    }

    // Do not support overwriting existing material fx graphs.
    if (gEnv->pFileIO->Exists(targetFilename.c_str()))
    {
        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Can't create Material FX Graph because another Material FX Graph with this name already exists!\n\nCreation canceled..."));
        return false;
    }

    // Create default MatFX Graph
    CHyperGraph* pGraph = GetIEditor()->GetFlowGraphManager()->CreateGraph();
    CHyperNode* pStartNode = (CHyperNode*) pGraph->CreateNode("MaterialFX:HUDStartFX");
    pStartNode->SetPos(QPointF(80, 10));
    CHyperNode* pEndNode = (CHyperNode*) pGraph->CreateNode("MaterialFX:HUDEndFX");
    pEndNode->SetPos(QPointF(400, 10));
    pGraph->SetGroupName(GetDefaultGroupName(assetPath, filename.toUtf8().data()).c_str());

    pGraph->UnselectAll();
    pGraph->ConnectPorts(pStartNode, &pStartNode->GetOutputs()->at(0), pEndNode, &pEndNode->GetInputs()->at(0), false);

    // Save the "barebones" material fx graph.
    bool saved = pGraph->Save(targetFilename.c_str());
    delete pGraph;
    pGraph = nullptr;

    if (!saved)
    {
        QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Can't create Material FX Graph!\nCould not save new file!\n\nCreation canceled..."));
        return false;
    }

    // Reload all material fx graphs, this will include the one we just created.
    ReloadFXGraphs();

    // We are expected to return a pointer to the hypergraph of the matfx FlowGraph we just created.
    if (pHyperGraph)
    {
        char aliasedPath[AZ_MAX_PATH_LEN] = { 0 };
        if (azstrcpy(aliasedPath, AZ_MAX_PATH_LEN, targetFilename.c_str()) != 0)
        {
            return false;
        }

        gEnv->pFileIO->ConvertToAlias(aliasedPath, sizeof(aliasedPath));

        AZStd::string aliasNormalizedPath = aliasedPath;
        AzFramework::StringFunc::Replace(aliasNormalizedPath, AZ_CORRECT_FILESYSTEM_SEPARATOR, '/');

        for (IFlowGraphPtr pGraph : m_matFxGraphs)
        {
            CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
            if (pFG && pFG->GetFilename().compare(aliasNormalizedPath.c_str(), Qt::CaseInsensitive) == 0)
            {
                *pHyperGraph = pFG;
                break;
            }
        }
    }

    return true;
}

bool CMaterialFXGraphMan::PromptChanges()
{
    if (HasModifications())
    {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setWindowTitle(QObject::tr("Material FX Graph(s) not saved!"));
        mbox.setText(QObject::tr("Some Material FX flowgraphs are modified!\nDo you want to save your changes?"));
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

void CMaterialFXGraphMan::ClearEditorGraphs()
{
    TGraphList::iterator iter = m_matFxGraphs.begin();
    for (; iter != m_matFxGraphs.end(); ++iter)
    {
        IFlowGraphPtr pGraph = *iter;
        CFlowGraph* pFG = GetIEditor()->GetFlowGraphManager()->FindGraph(pGraph);
        SAFE_RELEASE(pFG);
    }

    m_matFxGraphs.clear();
}