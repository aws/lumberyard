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
#include "Pivot2BottomTool.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "Core/BrushHelper.h"

void Pivot2BottomTool::Enter()
{
    CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();
    CUndo undo("Designer : Pivot Tool");

    for (int i = 0, iCount(pSelectionGroup->GetCount()); i < iCount; ++i)
    {
        CBaseObject* pBaseObject = pSelectionGroup->GetObject(i);
        if (!qobject_cast<DesignerObject*>(pBaseObject))
        {
            continue;
        }

        DesignerObject* pDesignerObj = (DesignerObject*)pBaseObject;
        CD::SMainContext mc(pDesignerObj->MainContext());
        if (!mc.IsValid())
        {
            continue;
        }

        if (!mc.pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
        {
            mc.pModel->RecordUndo("Designer : Pivot", mc.pObject);
            CD::PivotToCenter(mc.pObject, mc.pModel);
            CD::UpdateAll(mc, CD::eUT_ExceptMirror);
        }
        else if (iCount == 1)
        {
            CD::MessageBox("Warning", "This tool can't be used in Mirror mode");
        }
    }

    CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Pivot2Bottom, CD::eToolGroup_Misc, "Pivot2Bottom", Pivot2BottomTool)