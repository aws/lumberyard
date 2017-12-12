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
#include "SeparateTool.h"
#include "Tools/Select/SelectTool.h"
#include "Core/Model.h"
#include "Objects/DesignerObject.h"
#include "Tools/DesignerTool.h"

void SeparateTool::Enter()
{
    CUndo undo("Designer : Separate a Part");
    GetModel()->RecordUndo("Designer : Separate a Part", GetBaseObject());
    DesignerObject* pNewObj = Separate(GetMainContext());
    if (pNewObj)
    {
        CD::GetDesigner()->SetBaseObject(pNewObj);
        GetIEditor()->ShowTransformManipulator(false);
    }
    CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
}

DesignerObject* SeparateTool::Separate(CD::SMainContext& mc)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();

    DesignerObject* pNewObj = (DesignerObject*)GetIEditor()->NewObject("Designer", "");
    DESIGNER_ASSERT(pNewObj);
    if (!pNewObj)
    {
        return NULL;
    }

    pNewObj->SetWorldTM(mc.pObject->GetWorldTM());

    _smart_ptr<CD::Model> pNewModel = pNewObj->GetModel();
    pNewModel->SetModeFlag(mc.pModel->GetModeFlag());
    pNewModel->SetSubdivisionLevel(mc.pModel->GetSubdivisionLevel());

    MODEL_SHELF_RECONSTRUCTOR_POSTFIX(pNewModel, 0);
    MODEL_SHELF_RECONSTRUCTOR_POSTFIX(mc.pModel, 1);

    pNewModel->SetShelf(0);
    mc.pModel->SetShelf(0);

    for (int i = 0, iElementCount(pSelected->GetCount()); i < iElementCount; ++i)
    {
        const SElement& elementInfo = (*pSelected)[i];
        if (!elementInfo.IsFace())
        {
            continue;
        }

        DESIGNER_ASSERT(elementInfo.m_pPolygon);
        if (!elementInfo.m_pPolygon)
        {
            continue;
        }

        mc.pModel->RemovePolygon(mc.pModel->QueryEquivalentPolygon(elementInfo.m_pPolygon));
        pNewModel->AddPolygon(elementInfo.m_pPolygon->Clone(), CD::eOpType_Add);
    }

    CD::UpdateAll(mc);
    CD::UpdateAll(pNewObj->MainContext());

    pNewObj->SetMaterial(mc.pObject->GetMaterial());

    GetIEditor()->SelectObject(pNewObj);
    GetIEditor()->GetObjectManager()->UnselectObject(mc.pObject);

    return pNewObj;
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Separate, CD::eToolGroup_Edit, "Separate", SeparateTool)