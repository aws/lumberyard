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
#include "SphereTool.h"
#include "Core/Model.h"
#include "Util/PrimitiveShape.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

void SphereTool::Enter()
{
    ShapeTool::Enter();
    SetEditMode(eEditMode_Beginning);
}

void SphereTool::Leave()
{
    if (GetEditMode() == eEditMode_Done)
    {
        GetIEditor()->AcceptUndo("Designer : Create a Sphere");
        FreezeModel();
    }
    else
    {
        MODEL_SHELF_RECONSTRUCTOR(GetModel());
        GetModel()->SetShelf(1);
        GetModel()->Clear();
        UpdateShelf(1);
        GetIEditor()->CancelUndo();
    }
    ShapeTool::Leave();
}

void SphereTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    int bKeepInitPlane = GetEditMode() == eEditMode_Editing ? true : false;
    int bSearchAllShelves = GetEditMode() == eEditMode_Done ? true : false;
    if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, bKeepInitPlane, bSearchAllShelves))
    {
        return;
    }

    if (GetPickedPolygon())
    {
        SetPlane(GetPickedPolygon()->GetPlane());
    }

    if (GetEditMode() == eEditMode_Editing)
    {
        BrushVec2 vSpotPos2D = GetPlane().W2P(GetCurrentSpotPos());

        m_SphereParameter.m_Radius = aznumeric_cast<float>((vSpotPos2D - m_vCenterOnPlane).GetLength());
        const BrushFloat kSmallestRadius = 0.05f;
        if (m_SphereParameter.m_Radius < kSmallestRadius)
        {
            m_SphereParameter.m_Radius = kSmallestRadius;
        }

        m_fAngle = CD::ComputeAnglePointedByPos(m_vCenterOnPlane, vSpotPos2D);

        UpdateSphere();
        GetPanel()->Update();
    }
}

void SphereTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (GetEditMode() == eEditMode_Beginning)
    {
        if (ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false, true))
        {
            SetStartSpot(GetCurrentSpot());
            SetEditMode(eEditMode_Editing);
            SetPlane(GetCurrentSpot().m_Plane);
            SetTempPolygon(GetCurrentSpot().m_pPolygon);
            m_vCenterOnPlane = GetPlane().W2P(GetCurrentSpotPos());
            GetIEditor()->BeginUndo();
            GetModel()->RecordUndo("Designer : Create a Sphere", GetBaseObject());
            StoreSeparateStatus();
            m_MatTo001 = CD::ToBrushMatrix33(Matrix33::CreateRotationV0V1(GetPlane().Normal(), Vec3(0, 0, 1)));
            m_MatTo001.SetTranslation(-GetCurrentSpotPos());
        }
    }
    else if (GetEditMode() == eEditMode_Editing)
    {
        SetEditMode(eEditMode_Done);
    }
}

void SphereTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (GetEditMode() == eEditMode_Done)
    {
        SetEditMode(eEditMode_Beginning);
        if (GetIntermediatePolygon()->IsValid())
        {
            FreezeModel();
        }
        SetIntermediatePolygon(NULL);
        GetIEditor()->AcceptUndo("Designer : Create a Sphere");
    }
}

bool SphereTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (GetEditMode() == eEditMode_Editing)
        {
            CancelCreation();
        }
        else if (GetEditMode() == eEditMode_Done)
        {
            SetPlane(BrushPlane(BrushVec3(0, 0, 0), 0));
            FreezeModel();
            SetEditMode(eEditMode_Beginning);
            GetIEditor()->AcceptUndo("Designer : Create a Sphere");
            return true;
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void SphereTool::Display(DisplayContext& dc)
{
    DisplayCurrentSpot(dc);
    if (GetEditMode() == eEditMode_Editing || GetEditMode() == eEditMode_Done)
    {
        DrawIntermediatePolygon(dc);
    }

    DisplayDimensionHelper(dc, 1);
    if (GetStartSpot().m_pPolygon)
    {
        DisplayDimensionHelper(dc);
    }
}

void SphereTool::UpdateSphere()
{
    UpdateHelperDisc(m_SphereParameter.m_Radius, m_SphereParameter.m_NumOfSubdivision);

    AABB aabb;
    aabb.Reset();
    for (int i = 0, iVertexCount(GetIntermediatePolygon()->GetVertexCount()); i < iVertexCount; ++i)
    {
        aabb.Add(m_MatTo001.TransformPoint(GetIntermediatePolygon()->GetPos(i)));
    }

    aabb.max.z = aabb.max.z + m_SphereParameter.m_Radius;
    aabb.min.z = aabb.min.z - m_SphereParameter.m_Radius;

    if (aabb.IsReset())
    {
        return;
    }

    m_SpherePolygons.clear();
    PrimitiveShape bp;
    bp.CreateSphere(aabb.min, aabb.max, m_SphereParameter.m_NumOfSubdivision, &m_SpherePolygons);

    if (GetTempPolygon())
    {
        for (int i = 0, iPolygonCount(m_SpherePolygons.size()); i < iPolygonCount; ++i)
        {
            m_SpherePolygons[i]->SetMaterialID(GetTempPolygon()->GetMaterialID());
            m_SpherePolygons[i]->SetTexInfo(GetTempPolygon()->GetTexInfo());
        }
    }

    UpdateDesignerBasedOnSpherePolygons(BrushMatrix34::CreateIdentity());
}

void SphereTool::UpdateHelperDisc(float fRadius, int nSubdivisionNum)
{
    std::vector<BrushVec2> vertices2D;
    CD::MakeSectorOfCircle(fRadius, m_vCenterOnPlane, m_fAngle, CD::PI2, nSubdivisionNum + 1, vertices2D);
    vertices2D.erase(vertices2D.begin());
    CD::STexInfo texInfo = GetTexInfo();
    CD::PolygonPtr pPolygon = GetIntermediatePolygon();
    if (!pPolygon)
    {
        pPolygon = new CD::Polygon(vertices2D, GetPlane(), GetMatID(), &texInfo, true);
        SetIntermediatePolygon(pPolygon);
    }
    else
    {
        *pPolygon = CD::Polygon(vertices2D, GetPlane(), GetMatID(), &texInfo, true);
    }
}

void SphereTool::UpdateDesignerBasedOnSpherePolygons(const BrushMatrix34& tm)
{
    BrushMatrix34 matFrom001 = m_MatTo001.GetInverted();

    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);
    GetModel()->Clear();
    for (int i = 0, iPolygonCount(m_SpherePolygons.size()); i < iPolygonCount; ++i)
    {
        m_SpherePolygons[i]->Transform(matFrom001);
        CD::AddPolygonWithSubMatID(m_SpherePolygons[i]);
    }
    UpdateAll(CD::eUT_Mirror);
    UpdateShelf(1);
}

void SphereTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginUndoRedo:
    case eNotify_OnBeginSceneSave:
        if (GetEditMode() == eEditMode_Done)
        {
            GetIEditor()->AcceptUndo("Designer : Create a Sphere");
            FreezeModel();
        }
        else
        {
            GetIEditor()->CancelUndo();
            CancelCreation();
        }
        SetEditMode(eEditMode_Beginning);
    }
}

void SphereTool::FreezeModel()
{
    SetPlane(BrushPlane(BrushVec3(0, 0, 0), 0));
    ShapeTool::FreezeModel();
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Sphere, CD::eToolGroup_Shape, "Sphere", SphereTool, PropertyTreePanel<SphereTool>)