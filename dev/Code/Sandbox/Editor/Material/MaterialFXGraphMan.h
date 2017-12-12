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

#ifndef CRYINCLUDE_EDITOR_MATERIAL_MATERIALFXGRAPHMAN_H
#define CRYINCLUDE_EDITOR_MATERIAL_MATERIALFXGRAPHMAN_H
#pragma once


#include "LevelIndependentFileMan.h"
#include "HyperGraph/HyperGraph.h"

class CMaterialFXGraphMan
    : public ILevelIndependentFileModule
{
public:
    CMaterialFXGraphMan();
    virtual ~CMaterialFXGraphMan();

    void Init();
    void ReloadFXGraphs();

    void ClearEditorGraphs();
    void SaveChangedGraphs();
    bool HasModifications();

    bool NewMaterialFx(QString& filename, CHyperGraph** pHyperGraph = NULL);

    //ILevelIndependentFileModule
    virtual bool PromptChanges();
    //~ILevelIndependentFileModule

private:
    typedef std::list<IFlowGraphPtr> TGraphList;
    TGraphList m_matFxGraphs;
};

#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIALFXGRAPHMAN_H
