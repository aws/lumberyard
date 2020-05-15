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
#include "ShapeTool.h"
#include "Tools/DesignerTool.h"
#include "ViewManager.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "Tools/Edit/SeparateTool.h"
#include "Tools/Misc/ResetXFormTool.h"
#include "Tools/Select/SelectTool.h"
#include "Core/SmoothingGroupManager.h"
#include "Objects/DesignerObject.h"

void ShapeTool::Enter()
{
    BaseTool::Enter();
    SpotManager::ResetAllSpots();
    m_EditMode = eEditMode_None;
}

void ShapeTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_EditMode != eEditMode_None)
    {
        return;
    }

    SetStartSpot(GetCurrentSpot());
    m_EditMode = eEditMode_Beginning;
}

void ShapeTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_EditMode != eEditMode_Editing || GetIntermediatePolygon() == NULL)
    {
        return;
    }

    if (GetIntermediatePolygon()->IsValid())
    {
        CUndo undo("Designer : Draw a Shape");
        GetModel()->RecordUndo("Draw a Shape", GetBaseObject());

        CD::EOperationType opType = CD::eOpType_Split;
        if (nFlags & MK_CONTROL)
        {
            opType = CD::eOpType_Union;
        }

        GetModel()->AddPolygon(GetIntermediatePolygon(), opType);
        CD::UpdateMirroredPartWithPlane(GetModel(), GetIntermediatePolygon()->GetPlane());
        UpdateAll(CD::eUT_ExceptMirror);

        GetIntermediatePolygon()->Init();
        m_EditMode = eEditMode_None;
    }
}

bool ShapeTool::UpdateCurrentSpotPosition(CViewport* view, UINT nFlags, const QPoint& point, bool bKeepInitialPlane, bool bSearchAllShelves)
{
    if (nFlags & MK_SHIFT)
    {
        EnableMagnetic(false);
    }
    else
    {
        EnableMagnetic(true);
    }

    if (!SpotManager::UpdateCurrentSpotPosition(GetModel(), GetBaseObject()->GetWorldTM(), GetPlane(), view, point, bKeepInitialPlane, bSearchAllShelves))
    {
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());
        return false;
    }

    CD::GetDesigner()->UpdateSelectionMesh(GetCurrentSpot().m_pPolygon, GetCompiler(), GetBaseObject());
    SetPickedPolygon(GetCurrentSpot().m_pPolygon);

    return true;
}

void ShapeTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false))
    {
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());
        return;
    }

    SetPlane(GetCurrentSpot().m_Plane);

    if (m_EditMode == eEditMode_Beginning)
    {
        m_EditMode = eEditMode_Editing;
    }
}

bool ShapeTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_EditMode == eEditMode_Beginning || m_EditMode == eEditMode_Editing)
        {
            m_EditMode = eEditMode_None;
        }
        else
        {
            return CD::GetDesigner()->EndCurrentEditSession();
        }
    }

    return true;
}

void ShapeTool::Display(DisplayContext& dc)
{
    BaseTool::Display(dc);

    DisplayCurrentSpot(dc);
    if (m_EditMode == eEditMode_Editing || m_EditMode == eEditMode_Done)
    {
        DrawIntermediatePolygon(dc);
    }
}

void ShapeTool::DisplayCurrentSpot(DisplayContext& dc)
{
    dc.SetFillMode(e_FillModeSolid);
    DrawCurrentSpot(dc, GetWorldTM());
}

void ShapeTool::DrawIntermediatePolygon(DisplayContext& dc)
{
    float oldThickness = dc.GetLineWidth();
    dc.SetLineWidth(CD::kLineThickness);
    dc.SetColor(CD::PolygonLineColor);

    if (GetIntermediatePolygon())
    {
        GetIntermediatePolygon()->Display(dc);
    }

    dc.SetLineWidth(oldThickness);
}

CD::STexInfo ShapeTool::GetTexInfo() const
{
    if (GetPickedPolygon())
    {
        return GetPickedPolygon()->GetTexInfo();
    }
    return CD::STexInfo();
}

int ShapeTool::GetMatID() const
{
    if (GetPickedPolygon())
    {
        return GetPickedPolygon()->GetMaterialID();
    }
    return 0;
}

void ShapeTool::UpdateDrawnPolygon(const BrushVec2& p0, const BrushVec2& p1)
{
    BrushVec2 sp0(p0), sp1(p1);
    for (int i = 0; i < 2; ++i)
    {
        if (sp0[i] > sp1[i])
        {
            std::swap(sp0[i], sp1[i]);
        }
    }
    std::vector<BrushVec3> vertices;
    CD::MakeRectangle(GetPlane(), sp0, sp1, vertices);
    CD::STexInfo texInfo = GetTexInfo();

    if (m_pIntermediatePolygon)
    {
        *m_pIntermediatePolygon = CD::Polygon(vertices, GetPlane(), GetMatID(), &texInfo, true);
    }
    else
    {
        SetIntermediatePolygon(new CD::Polygon(vertices, GetPlane(), GetMatID(), &texInfo, true));
    }
}


void ShapeTool::Freeze2DModel()
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    Separate1stStep();

    bool bEmptyDesigner = GetModel()->IsEmpty(0);

    GetModel()->SetShelf(1);
    GetModel()->Clear();

    if (GetIntermediatePolygon())
    {
        GetModel()->SetShelf(0);
        if (bEmptyDesigner)
        {
            GetModel()->AddPolygon(GetIntermediatePolygon()->Clone(), CD::eOpType_Split);
        }
        else
        {
            GetModel()->AddPolygon(GetIntermediatePolygon(), CD::eOpType_Split);
        }
        CD::UpdateMirroredPartWithPlane(GetModel(), GetIntermediatePolygon()->GetPlane());
    }

    GetModel()->GetSmoothingGroupMgr()->InvalidateAll();

    if (bEmptyDesigner || gDesignerSettings.bKeepPivotCenter)
    {
        UpdateAll();
    }
    else
    {
        UpdateAll(CD::eUT_ExceptCenterPivot);
    }

    if (!bEmptyDesigner && IsSeparateStatus())
    {
        Separate2ndStep();
    }
    else
    {
        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
        pSelected->Clear();
    }

    if (GetIntermediatePolygon())
    {
        GetIntermediatePolygon()->Init();
    }
}

void ShapeTool::Separate1stStep()
{
    if (!IsSeparateStatus() || GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror))
    {
        return;
    }

    if (GetBaseObject()->GetType() != OBJTYPE_SOLID)
    {
        return;
    }

    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Clear();
    for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
        SElement de;
        de.SetFace(GetBaseObject(), pPolygon);
        pSelected->Add(de);
    }
}

void ShapeTool::Separate2ndStep()
{
    if (!IsSeparateStatus() || GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror))
    {
        return;
    }

    if (GetBaseObject()->GetType() != OBJTYPE_SOLID)
    {
        return;
    }

    DesignerObject* pSparatedObj = SeparateTool::Separate(GetMainContext());
    if (pSparatedObj)
    {
        CD::GetDesigner()->SetBaseObject(pSparatedObj);
    }
}

void ShapeTool::FreezeModel()
{
    if (!GetModel())
    {
        return;
    }

    Separate1stStep();

    bool bEmptyModel = IsModelEmpty();

    GetModel()->SetShelf(1);
    GetModel()->RemovePolygonsWithSpecificFlagsPlane(CD::ePolyFlag_Mirrored);

    bool bUniquePolygon = GetModel()->GetPolygonCount() == 1 ? true : false;

    CD::PolygonList polygons;
    GetModel()->GetPolygonList(polygons);
    CD::PolygonPtr pPolygonTouchingFloor = NULL;

    BrushPlane floorPlane = GetTempPolygon() ? GetTempPolygon()->GetPlane() : GetPlane();
    BrushPlane invFloorPlane = floorPlane.GetInverted();

    for (int i = 0, iPolygonCount(polygons.size()); i < iPolygonCount; ++i)
    {
        const BrushPlane& plane = polygons[i]->GetPlane();
        if (!plane.IsEquivalent(floorPlane) && !plane.IsEquivalent(invFloorPlane))
        {
            continue;
        }
        GetModel()->RemovePolygon(polygons[i]);
        if (plane.IsEquivalent(invFloorPlane))
        {
            pPolygonTouchingFloor = polygons[i]->Flip();
            break;
        }
    }

    GetModel()->MoveShelf(1, 0);
    GetModel()->SetShelf(0);

    if (pPolygonTouchingFloor)
    {
        bool bAdded = false;
        if (!IsSeparateStatus())
        {
            if (GetTempPolygon() && pPolygonTouchingFloor->IncludeAllEdges(GetTempPolygon()))
            {
                GetModel()->RemovePolygon(GetModel()->QueryEquivalentPolygon(GetTempPolygon()));
                CD::PolygonPtr pClonedPolygon = pPolygonTouchingFloor->Clone();
                pClonedPolygon->Subtract(GetTempPolygon());
                if (pClonedPolygon->IsValid())
                {
                    GetModel()->AddPolygon(pClonedPolygon->Flip(), CD::eOpType_Add);
                }
                bAdded = true;
            }
            else if (GetModel()->HasIntersection(pPolygonTouchingFloor))
            {
                GetModel()->AddPolygon(pPolygonTouchingFloor, CD::eOpType_ExclusiveOR);
                GetModel()->SeparatePolygons(pPolygonTouchingFloor->GetPlane());
                bAdded = true;
            }
        }
        if (!bAdded)
        {
            GetModel()->AddPolygon(pPolygonTouchingFloor->Flip(), CD::eOpType_Add);
        }
    }

    BrushVec3 vNewPivot = GetModel()->GetBoundBox().min;
    GetModel()->GetSmoothingGroupMgr()->InvalidateAll();

    if (!bEmptyModel && IsSeparateStatus())
    {
        Separate2ndStep();
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    pSelected->Clear();

    int nUpdateFlags = CD::eUT_All;
    if (!gDesignerSettings.bKeepPivotCenter)
    {
        nUpdateFlags = CD::eUT_ExceptCenterPivot;
        if (!bEmptyModel && IsSeparateStatus())
        {
            AABB aabb;
            GetBaseObject()->GetLocalBounds(aabb);
            CD::PivotToPos(GetBaseObject(), GetModel(), aabb.min);
        }
    }

    UpdateAll(nUpdateFlags);
}

bool ShapeTool::IsSeparateStatus() const
{
    if (GetBaseObject()->GetType() != OBJTYPE_SOLID)
    {
        return false;
    }
    return m_bSeparatedNewShape;
}

void ShapeTool::CancelCreation()
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);
    GetModel()->Clear();
    UpdateShelf(1);
}
