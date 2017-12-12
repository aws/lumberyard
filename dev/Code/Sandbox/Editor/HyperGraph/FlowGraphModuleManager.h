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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMODULEMANAGER_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMODULEMANAGER_H
#pragma once


#include "LevelIndependentFileMan.h"
#include "HyperGraph/HyperGraph.h"
#include "../HyperGraph/FlowGraph.h"
#include <IFlowGraphModuleManager.h>

class CEditorFlowGraphModuleManager
    : public ILevelIndependentFileModule
    , public IFlowGraphModuleListener
{
public:
    CEditorFlowGraphModuleManager();
    virtual ~CEditorFlowGraphModuleManager();

    //ILevelIndependentFileModule
    virtual bool PromptChanges();
    //~ILevelIndependentFileModule

    //IFlowGraphModuleListener
    virtual void OnModuleInstanceCreated(IFlowGraphModule* module, TModuleInstanceId instanceID){}
    virtual void OnModuleInstanceDestroyed(IFlowGraphModule* module, TModuleInstanceId instanceID){}
    virtual void OnPostModuleDestroyed(){}
    virtual void OnModuleDestroyed(IFlowGraphModule* module);
    virtual void OnRootGraphChanged(IFlowGraphModule* module);
    virtual void OnScannedForModules();
    //~IFlowGraphModuleListener

    void Init();

    bool NewModule(QString& filename, IFlowGraphModule::EType type, CHyperGraph** pHyperGraph = NULL);

    IFlowGraphPtr GetModuleFlowGraph(int index) const;
    IFlowGraphPtr GetModuleFlowGraph(const char* name) const;
    void DeleteModuleFlowGraph(CFlowGraph* pGraph);

    void CreateModuleNodes(const char* moduleName);

    void SaveModuleGraph(CFlowGraph* pFG);

private:

    bool HasModifications();
    void SaveChangedGraphs();
    void ReloadHypergraphDialog();
    void CreateEditorFlowgraphs();

    typedef std::vector<CFlowGraph*> TFlowGraphs;
    TFlowGraphs m_FlowGraphs;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHMODULEMANAGER_H
