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
#include "BooleanTool.h"
#include "Tools/DesignerTool.h"
#include "Serialization/Decorators/EditorActionButton.h"
#include "Core/BrushHelper.h"
#include "Objects/DesignerObject.h"

using Serialization::ActionButton;

void BooleanTool::Enter()
{
    BaseTool::Enter();

    CSelectionGroup* pGroup = GetIEditor()->GetSelection();
    int nDesignerObjectCount = 0;
    int nOtherObjectCount = 0;
    for (int i = 0, iObjectCount(pGroup->GetCount()); i < iObjectCount; ++i)
    {
        CBaseObject* pObject = pGroup->GetObject(i);
        if (pObject->GetType() == OBJTYPE_SOLID)
        {
            ++nDesignerObjectCount;
        }
        else
        {
            ++nOtherObjectCount;
        }
    }

    if (nOtherObjectCount > 0 || nDesignerObjectCount < 2)
    {
        if (nOtherObjectCount > 0)
        {
            CD::MessageBox("Warning", "Only designer objects should be selected to use this tool");
        }
        else if (nDesignerObjectCount < 2)
        {
            CD::MessageBox("Warning", "At least two designer objects should be selected to use this tool");
        }
        CD::GetDesigner()->SwitchTool(CD::eDesigner_Object);
    }
}

void BooleanTool::BooleanOperation(CD::EBooleanOperationEnum booleanType)
{
    CSelectionGroup* pSelection = GetIEditor()->GetObjectManager()->GetSelection();

    std::vector< _smart_ptr<DesignerObject> > designerObjList;
    for (int i = 0, iSelectionCount(pSelection->GetCount()); i < iSelectionCount; ++i)
    {
        CBaseObject* pObj = pSelection->GetObject(iSelectionCount - i - 1);
        if (!qobject_cast<DesignerObject*>(pObj))
        {
            continue;
        }
        designerObjList.push_back((DesignerObject*)pObj);
    }

    if (designerObjList.size() < 2)
    {
        CD::MessageBox("Warning", "More than 2 designer objects should be selected to be operated.");
        return;
    }

    QString undoMsg("Designer : Boolean.");

    if (booleanType == CD::eBOE_Union)
    {
        undoMsg += "Union";
    }
    else if (booleanType == CD::eBOE_Intersection)
    {
        undoMsg += "Intersection";
    }
    else if (booleanType == CD::eBOE_Difference)
    {
        undoMsg += "Difference";
    }

    CUndo undo(undoMsg.toUtf8().data());
    designerObjList[0]->StoreUndo(undoMsg.toUtf8().data());
    CD::ResetXForm(designerObjList[0], designerObjList[0]->GetModel());

    CD::GetDesigner()->SetBaseObject(NULL);

    for (int i = 1, iDesignerObjCount(designerObjList.size()); i < iDesignerObjCount; ++i)
    {
        BrushVec3 offset = designerObjList[i]->GetWorldTM().GetTranslation() - designerObjList[0]->GetWorldTM().GetTranslation();
        Matrix34 targetTM(designerObjList[i]->GetWorldTM());
        targetTM.SetTranslation(offset);
        designerObjList[i]->StoreUndo(undoMsg.toUtf8().data());
        designerObjList[i]->GetModel()->Transform(targetTM);

        if (booleanType == CD::eBOE_Union)
        {
            designerObjList[0]->GetModel()->Union(designerObjList[i]->GetModel());
        }
        else if (booleanType == CD::eBOE_Difference)
        {
            designerObjList[0]->GetModel()->Subtract(designerObjList[i]->GetModel());
        }
        else if (booleanType == CD::eBOE_Intersection)
        {
            designerObjList[0]->GetModel()->Intersect(designerObjList[i]->GetModel());
        }

        GetIEditor()->DeleteObject(designerObjList[i]);
    }

    CD::UpdateAll(designerObjList[0]->MainContext());
    GetIEditor()->SelectObject(designerObjList[0]);

    DesignerTool* pDesignerTool = CD::GetDesigner();
    if (pDesignerTool)
    {
        pDesignerTool->SetBaseObject(designerObjList[0]);
        pDesignerTool->SwitchTool(CD::eDesigner_Object);
    }
}

void BooleanTool::Serialize(Serialization::IArchive& ar)
{
    ar(ActionButton([this] {Union();
            }), "Union", "Union");
    ar(ActionButton([this] {Subtract();
            }), "Subtract", "Subtract");
    ar(ActionButton([this] {Intersection();
            }), "Intersection", "Intersection");
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Boolean, CD::eToolGroup_Modify, "Boolean", BooleanTool, PropertyTreePanel<BooleanTool>)