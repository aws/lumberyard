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
#include "CopyTool.h"
#include "Tools/Select/SelectTool.h"
#include "Core/Model.h"
#include "Objects/DesignerObject.h"
#include "Tools/DesignerTool.h"
#include "Util/ElementManager.h"

void CopyTool::Enter()
{
    CUndo undo("Designer : Copy a Part");

    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    GetModel()->SetShelf(0);
    GetModel()->RecordUndo("Designer : Copy a Part", GetBaseObject());

    ElementManager copiedElements;
    Copy(CD::SMainContext(GetBaseObject(), GetCompiler(), GetModel()), &copiedElements);

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Clear();
    pSelected->Add(copiedElements);

    UpdateAll();
    CD::GetDesigner()->SwitchToSelectTool();
}

void CopyTool::Copy(CD::SMainContext& mc, ElementManager* pOutCopiedElements)
{
    DESIGNER_ASSERT(pOutCopiedElements);
    if (!pOutCopiedElements)
    {
        return;
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    std::vector<CD::PolygonPtr> selectedPolygons;

    for (int i = 0, iElementCount(pSelected->GetCount()); i < iElementCount; ++i)
    {
        const SElement& elementInfo = pSelected->Get(i);
        if (!elementInfo.IsFace())
        {
            continue;
        }

        DESIGNER_ASSERT(elementInfo.m_pPolygon);
        if (!elementInfo.m_pPolygon || !elementInfo.m_pPolygon->IsValid())
        {
            continue;
        }

        CD::PolygonPtr pPolygon = mc.pModel->QueryEquivalentPolygon(elementInfo.m_pPolygon);
        selectedPolygons.push_back(pPolygon);
    }

    pOutCopiedElements->Clear();
    for (int i = 0, iPolygonCount(selectedPolygons.size()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pClone = selectedPolygons[i]->Clone();
        mc.pModel->AddPolygonUnconditionally(pClone);
        SElement ei;
        ei.SetFace(mc.pObject, pClone);
        pOutCopiedElements->Add(ei);
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Copy, CD::eToolGroup_Edit, "Copy", CopyTool)