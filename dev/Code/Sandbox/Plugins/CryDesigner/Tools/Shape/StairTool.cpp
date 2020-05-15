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
#include "StairTool.h"
#include "Core/Model.h"
#include "Viewport.h"
#include "Objects/DesignerObject.h"
#include "Util/HeightManipulator.h"
#include "Util/ExtrusionSnappingHelper.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

namespace
{
    BrushFloat s_MinimumSize = (BrushFloat)0.001;
}

void StairTool::Enter()
{
    ShapeTool::Enter();
    m_StairMode = eStairMode_PlaceFirstPoint;
}

void StairTool::Leave()
{
    if (m_StairMode == eStairMode_Done)
    {
        FreezeModel();
        AcceptUndo();
    }
    else
    {
        if (GetModel())
        {
            MODEL_SHELF_RECONSTRUCTOR(GetModel());
            GetModel()->SetShelf(1);
            GetModel()->Clear();
            UpdateShelf(1);
        }
    }

    m_pUndoModel = NULL;

    ShapeTool::Leave();
}

void StairTool::AcceptUndo()
{
    if (m_pUndoModel == NULL)
    {
        return;
    }
    GetIEditor()->BeginUndo();
    m_pUndoModel->RecordUndo("Designer : Create a Stair", GetBaseObject());
    GetIEditor()->AcceptUndo("Designer : Create a Stair");
    m_pUndoModel = NULL;
}

void StairTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_StairMode == eStairMode_PlaceFirstPoint)
    {
        if (!UpdateCurrentSpotPosition(view, nFlags, point, false, false))
        {
            return;
        }

        m_StairMode = eStairMode_CreateRectangle;
        m_pUndoModel = new CD::Model(*GetModel());
        m_fBoxWidth = m_fBoxDepth = 0;

        SetPlane(GetCurrentSpot().m_Plane);
        m_vStartPos = m_vEndPos = GetCurrentSpotPos();
        SetTempPolygon(GetCurrentSpot().m_pPolygon);
        StoreSeparateStatus();

        s_SnappingHelper.Init(GetModel());
    }
    else if (m_StairMode == eStairMode_CreateBox)
    {
        if (m_fBoxHeight >= s_MinimumSize)
        {
            m_StairMode = eStairMode_Done;
            PlaceFirstPoint(view, nFlags, point);
        }
    }
}

void StairTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_StairMode == eStairMode_CreateRectangle)
    {
        BrushVec2 startSpotPos = GetPlane().W2P(m_vStartPos);
        BrushVec2 currentSpotPos = GetPlane().W2P(m_vEndPos);
        if (std::abs(startSpotPos.x - currentSpotPos.x) < s_MinimumSize || std::abs(startSpotPos.y - currentSpotPos.y) < s_MinimumSize)
        {
            return;
        }

        m_fPrevBoxHegith = 0;
        m_StairMode = eStairMode_CreateBox;

        s_HeightManipulator.Init(GetPlane(), m_vStartPos);

        if (startSpotPos.x > currentSpotPos.x)
        {
            std::swap(startSpotPos.x, currentSpotPos.x);
        }
        if (startSpotPos.y > currentSpotPos.y)
        {
            std::swap(startSpotPos.y, currentSpotPos.y);
        }

        std::vector<BrushVec3> vList;
        vList.push_back(GetPlane().P2W(startSpotPos));
        vList.push_back(GetPlane().P2W(BrushVec2(startSpotPos.x, currentSpotPos.y)));
        vList.push_back(GetPlane().P2W(currentSpotPos));
        vList.push_back(GetPlane().P2W(BrushVec2(currentSpotPos.x, startSpotPos.y)));

        m_pFloorPolygon = new CD::Polygon(vList);
        s_SnappingHelper.SearchForOppositePolygons(m_pFloorPolygon);

        m_FloorPlane = GetPlane();
        CreateBox(view, nFlags, point);
    }

    if (m_StairMode == eStairMode_Done)
    {
        FreezeModel();
        AcceptUndo();
        m_StairMode = eStairMode_PlaceFirstPoint;
    }
}

void StairTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_StairMode == eStairMode_PlaceFirstPoint || m_StairMode == eStairMode_Done)
    {
        PlaceFirstPoint(view, nFlags, point);
    }
    else if (m_StairMode == eStairMode_CreateRectangle)
    {
        CreateRectangle(view, nFlags, point);
    }
    else if (m_StairMode == eStairMode_CreateBox)
    {
        CreateBox(view, nFlags, point);
        UpdateStair();
    }
}

bool StairTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_StairMode == eStairMode_Done)
        {
            FreezeModel();
            AcceptUndo();
            m_StairMode = eStairMode_PlaceFirstPoint;
            return true;
        }
        else if (m_StairMode == eStairMode_CreateBox)
        {
            MODEL_SHELF_RECONSTRUCTOR(GetModel());
            GetModel()->SetShelf(1);
            GetModel()->Clear();
            UpdateShelf(1);
            if (m_StairMode == eStairMode_CreateBox)
            {
                m_StairMode = eStairMode_PlaceFirstPoint;
            }
            else
            {
                m_StairMode = eStairMode_CreateBox;
            }
        }
        return CD::GetDesigner()->EndCurrentEditSession();
    }
    return true;
}

void StairTool::GetRectangleVertices(BrushVec3& outV0, BrushVec3& outV1, BrushVec3& outV2, BrushVec3& outV3)
{
    BrushVec2 p0 = GetPlane().W2P(m_vStartPos);
    BrushVec2 p2 = GetPlane().W2P(m_vEndPos);
    BrushVec2 p1(p2.x, p0.y);
    BrushVec2 p3(p0.x, p2.y);

    outV0 = GetPlane().P2W(p0);
    outV1 = GetPlane().P2W(p1);
    outV2 = GetPlane().P2W(p2);
    outV3 = GetPlane().P2W(p3);
}

void StairTool::Display(DisplayContext& dc)
{
    if (m_StairMode == eStairMode_CreateRectangle || m_StairMode == eStairMode_PlaceFirstPoint || m_StairMode == eStairMode_Done)
    {
        DrawCurrentSpot(dc, GetWorldTM());
    }

    if (m_StairMode == eStairMode_CreateRectangle)
    {
        dc.SetColor(ColorB(0, 0, 0, 255));
        BrushVec3 v[4];
        GetRectangleVertices(v[0], v[1], v[2], v[3]);
        for (int i = 0; i < 4; ++i)
        {
            dc.DrawLine(v[i], v[(i + 1) % 4]);
        }
    }
    else if (m_StairMode == eStairMode_CreateBox)
    {
        dc.SetColor(ColorB(0, 0, 0, 255));
        for (int i = 0; i < 4; ++i)
        {
            dc.DrawLine(m_BottomVertices[i], m_BottomVertices[(i + 1) % 4]);
            dc.DrawLine(m_TopVertices[i], m_TopVertices[(i + 1) % 4]);
            dc.DrawLine(m_BottomVertices[i], m_TopVertices[i]);
        }
    }

    DisplayDimensionHelper(dc, 1);
    s_HeightManipulator.Display(dc);
}

void StairTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginUndoRedo:
    case eNotify_OnBeginSceneSave:
        if (m_StairMode == eStairMode_Done)
        {
            AcceptUndo();
            FreezeModel();
        }
        else
        {
            GetIEditor()->CancelUndo();
            CancelCreation();
        }
        m_StairMode = eStairMode_PlaceFirstPoint;
        break;
    }
}

void StairTool::PlaceFirstPoint(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, false, true))
    {
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());
        return;
    }
    if (m_StairMode == eStairMode_PlaceFirstPoint)
    {
        SetPlane(GetCurrentSpot().m_Plane);
    }
}

void StairTool::CreateRectangle(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!ShapeTool::UpdateCurrentSpotPosition(view, nFlags, point, true))
    {
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());
        return;
    }

    m_vStartPos = GetCurrentSpotPos();

    BrushVec2 startSpotPos = GetPlane().W2P(m_vStartPos);
    BrushVec2 endSpotPos = GetPlane().W2P(m_vEndPos);

    m_fBoxWidth = endSpotPos.x - startSpotPos.x;
    m_fBoxDepth = endSpotPos.y - startSpotPos.y;

    m_bXDirection = std::abs(m_fBoxWidth) >= std::abs(m_fBoxDepth);

    m_StairParameter.m_Width = aznumeric_cast<float>(std::abs(m_fBoxWidth));
    m_StairParameter.m_Height = 0;
    m_StairParameter.m_Depth = aznumeric_cast<float>(std::abs(m_fBoxDepth));

    GetPanel()->Update();
}

void StairTool::CreateBox(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_fBoxHeight = s_HeightManipulator.UpdateHeight(GetWorldTM(), view, point);

    CD::PolygonPtr pPolygon = m_pFloorPolygon->Clone();
    pPolygon->UpdatePlane(BrushPlane(pPolygon->GetPlane().Normal(), m_pFloorPolygon->GetPlane().Distance() - m_fBoxHeight));

    m_bIsOverOpposite = false;

    if (nFlags & MK_SHIFT)
    {
        CD::PolygonPtr pAlignedPolygon = s_SnappingHelper.FindAlignedPolygon(m_pFloorPolygon, GetWorldTM(), view, point);
        if (pAlignedPolygon)
        {
            m_fBoxHeight = GetPlane().Distance() - pAlignedPolygon->GetPlane().Distance();
        }
        pPolygon->UpdatePlane(BrushPlane(pPolygon->GetPlane().Normal(), m_pFloorPolygon->GetPlane().Distance() - m_fBoxHeight));
    }
    else if (m_fBoxHeight - m_fPrevBoxHegith > kDesignerEpsilon)
    {
        m_bIsOverOpposite = s_SnappingHelper.IsOverOppositePolygon(pPolygon, CD::ePP_Pull, true);
        if (pPolygon && m_bIsOverOpposite)
        {
            m_fBoxHeight = s_SnappingHelper.GetNearestDistanceToOpposite(CD::ePP_Pull);
        }
    }

    BrushVec3 vNormal = aznumeric_cast<float>(m_fBoxHeight) * GetPlane().Normal();
    GetRectangleVertices(m_BottomVertices[0], m_BottomVertices[1], m_BottomVertices[2], m_BottomVertices[3]);

    m_StairParameter.m_Width = aznumeric_cast<float>(std::abs(m_fBoxWidth));
    m_StairParameter.m_Height = aznumeric_cast<float>(m_fBoxHeight);
    m_StairParameter.m_Depth = aznumeric_cast<float>(std::abs(m_fBoxDepth));

    GetPanel()->Update();

    for (int i = 0; i < 4; ++i)
    {
        m_TopVertices[i] = m_BottomVertices[i] + vNormal;
    }

    m_fPrevBoxHegith = m_fBoxHeight;
}

CD::PolygonPtr StairTool::CreatePolygon(const std::vector<BrushVec3>& vList, bool bFlip, CD::PolygonPtr pBasePolygon)
{
    if (vList.size() < 3)
    {
        return NULL;
    }
    CD::PolygonPtr pPolygon = new CD::Polygon(vList);
    if (pPolygon->IsOpen())
    {
        return NULL;
    }
    if (bFlip)
    {
        pPolygon->Flip();
    }
    if (pBasePolygon)
    {
        pPolygon->SetTexInfo(pBasePolygon->GetTexInfo());
        pPolygon->SetMaterialID(pBasePolygon->GetMaterialID());
    }
    return pPolygon;
}

void StairTool::CreateStair(const BrushVec3& vStartPos,
    const BrushVec3& vEndPos,
    BrushFloat fBoxWidth,
    BrushFloat fBoxDepth,
    BrushFloat fBoxHeight,
    const BrushPlane& floorPlane,
    float fStepRise,
    bool bXDirection,
    bool bMirrored,
    bool bRotationBy90Degree,
    CD::PolygonPtr pBasePolygon,
    SOutputParameterForStair& out)
{
    if (bRotationBy90Degree)
    {
        bXDirection = !bXDirection;
    }

    int nCompleteStepNum = (int)(fBoxHeight / fStepRise);
    BrushFloat fRestStepRise = fBoxHeight - fStepRise * nCompleteStepNum;
    BrushFloat fRatioRestStepLengthToFullLength = fRestStepRise / fBoxHeight;

    BrushVec2 startSpotPos = floorPlane.W2P(vStartPos);
    BrushVec2 endSpotPos = floorPlane.W2P(vEndPos);

    endSpotPos.x = startSpotPos.x + fBoxWidth;
    endSpotPos.y = startSpotPos.y + fBoxDepth;

    if (bMirrored)
    {
        std::swap(startSpotPos, endSpotPos);
        fBoxDepth = -fBoxDepth;
        fBoxWidth = -fBoxWidth;
    }

    BrushFloat fStairSize = bXDirection ? fBoxWidth : fBoxDepth;

    BrushFloat fStepTread = nCompleteStepNum == 0 ? 0 : ((1 - fRatioRestStepLengthToFullLength) * std::abs(fStairSize)) / nCompleteStepNum;
    BrushFloat fRestStepTread = std::abs(fStairSize) - fStepTread * nCompleteStepNum;

    int nWidthSign = fBoxWidth > 0 ? 1 : -1;
    int nDepthSign = fBoxDepth > 0 ? 1 : -1;

    std::vector<BrushVec3> vSideList[2];
    std::vector<BrushVec3> vBackList;
    std::vector<BrushVec3> vBottomList;

    bool bFlip = bXDirection ? nWidthSign * nDepthSign == -1 : nWidthSign * nDepthSign == 1;

    for (int i = 0; i <= nCompleteStepNum; ++i)
    {
        BrushFloat x = startSpotPos.x + i * fStepTread * nWidthSign;
        BrushFloat next_x = i < nCompleteStepNum ? startSpotPos.x + (i + 1) * fStepTread * nWidthSign : x + fRestStepTread * nWidthSign;

        BrushFloat z = startSpotPos.y + i * fStepTread * nDepthSign;
        BrushFloat next_z = i < nCompleteStepNum ? startSpotPos.y + (i + 1) * fStepTread * nDepthSign : z + fRestStepTread * nDepthSign;

        BrushFloat y = i * fStepRise;
        BrushFloat next_y = i < nCompleteStepNum ? (i + 1) * fStepRise : y + fRestStepRise;

        std::vector<BrushVec3> vStepRiseList(4);
        if (bXDirection)
        {
            vStepRiseList[0] = floorPlane.P2W(BrushVec2(x, startSpotPos.y));
            vStepRiseList[1] = floorPlane.P2W(BrushVec2(x, endSpotPos.y));
            vStepRiseList[0] += floorPlane.Normal() * y;
            vStepRiseList[1] += floorPlane.Normal() * y;

            vStepRiseList[2] = floorPlane.P2W(BrushVec2(x, endSpotPos.y));
            vStepRiseList[3] = floorPlane.P2W(BrushVec2(x, startSpotPos.y));
            vStepRiseList[2] += floorPlane.Normal() * next_y;
            vStepRiseList[3] += floorPlane.Normal() * next_y;
        }
        else
        {
            vStepRiseList[0] = floorPlane.P2W(BrushVec2(startSpotPos.x, z));
            vStepRiseList[1] = floorPlane.P2W(BrushVec2(endSpotPos.x, z));
            vStepRiseList[0] += floorPlane.Normal() * y;
            vStepRiseList[1] += floorPlane.Normal() * y;

            vStepRiseList[2] = floorPlane.P2W(BrushVec2(endSpotPos.x, z));
            vStepRiseList[3] = floorPlane.P2W(BrushVec2(startSpotPos.x, z));
            vStepRiseList[2] += floorPlane.Normal() * next_y;
            vStepRiseList[3] += floorPlane.Normal() * next_y;
        }
        CD::PolygonPtr pStepRisePolygon = CreatePolygon(vStepRiseList, bFlip, pBasePolygon);
        if (pStepRisePolygon)
        {
            out.polygons.push_back(pStepRisePolygon);
        }

        if (i == 0)
        {
            vBottomList.push_back(vStepRiseList[1]);
            vBottomList.push_back(vStepRiseList[0]);

            if (pStepRisePolygon)
            {
                out.polygonsNeedPostProcess.push_back(pStepRisePolygon);
            }
        }

        vSideList[0].push_back(vStepRiseList[0]);
        vSideList[0].push_back(vStepRiseList[3]);

        vSideList[1].push_back(vStepRiseList[1]);
        vSideList[1].push_back(vStepRiseList[2]);

        std::vector<BrushVec3> vStepTreadList(4);
        if (bXDirection)
        {
            vStepTreadList[0] = floorPlane.P2W(BrushVec2(x, startSpotPos.y));
            vStepTreadList[1] = floorPlane.P2W(BrushVec2(x, endSpotPos.y));
            vStepTreadList[2] = floorPlane.P2W(BrushVec2(next_x, endSpotPos.y));
            vStepTreadList[3] = floorPlane.P2W(BrushVec2(next_x, startSpotPos.y));

            vStepTreadList[0] += floorPlane.Normal() * next_y;
            vStepTreadList[1] += floorPlane.Normal() * next_y;
            vStepTreadList[2] += floorPlane.Normal() * next_y;
            vStepTreadList[3] += floorPlane.Normal() * next_y;
        }
        else
        {
            vStepTreadList[0] = floorPlane.P2W(BrushVec2(startSpotPos.x, z));
            vStepTreadList[1] = floorPlane.P2W(BrushVec2(endSpotPos.x, z));
            vStepTreadList[2] = floorPlane.P2W(BrushVec2(endSpotPos.x, next_z));
            vStepTreadList[3] = floorPlane.P2W(BrushVec2(startSpotPos.x, next_z));

            vStepTreadList[0] += floorPlane.Normal() * next_y;
            vStepTreadList[1] += floorPlane.Normal() * next_y;
            vStepTreadList[2] += floorPlane.Normal() * next_y;
            vStepTreadList[3] += floorPlane.Normal() * next_y;
        }
        CD::PolygonPtr pStepTreadPolygon = CreatePolygon(vStepTreadList, bFlip, pBasePolygon);
        if (pStepTreadPolygon)
        {
            out.polygons.push_back(pStepTreadPolygon);
        }

        if (i >= nCompleteStepNum - 1)
        {
            if (pStepTreadPolygon)
            {
                out.pCapPolygon = pStepTreadPolygon;
            }
        }

        if (i == nCompleteStepNum)
        {
            vSideList[0].push_back(vStepTreadList[3]);
            if (bXDirection)
            {
                vSideList[0].push_back(floorPlane.P2W(BrushVec2(next_x, startSpotPos.y)));
            }
            else
            {
                vSideList[0].push_back(floorPlane.P2W(BrushVec2(startSpotPos.x, next_z)));
            }
            CD::PolygonPtr pSidePolygon0 = CreatePolygon(vSideList[0], bFlip, pBasePolygon);
            if (pSidePolygon0)
            {
                out.polygons.push_back(pSidePolygon0);
                out.polygonsNeedPostProcess.push_back(pSidePolygon0);
            }

            vSideList[1].push_back(vStepTreadList[2]);
            if (bXDirection)
            {
                vSideList[1].push_back(floorPlane.P2W(BrushVec2(next_x, endSpotPos.y)));
            }
            else
            {
                vSideList[1].push_back(floorPlane.P2W(BrushVec2(endSpotPos.x, next_z)));
            }
            CD::PolygonPtr pSidePolygon1 = CreatePolygon(vSideList[1], !bFlip, pBasePolygon);
            if (pSidePolygon1)
            {
                out.polygons.push_back(pSidePolygon1);
                out.polygonsNeedPostProcess.push_back(pSidePolygon1);
            }

            vBackList.push_back(vSideList[0][vSideList[0].size() - 1]);
            vBackList.push_back(vSideList[0][vSideList[0].size() - 2]);
            vBackList.push_back(vSideList[1][vSideList[1].size() - 2]);
            vBackList.push_back(vSideList[1][vSideList[1].size() - 1]);
            CD::PolygonPtr pBackPolygon = CreatePolygon(vBackList, bFlip, pBasePolygon);
            if (pBackPolygon)
            {
                out.polygons.push_back(pBackPolygon);
                out.polygonsNeedPostProcess.push_back(pBackPolygon);
            }

            vBottomList.push_back(vBackList[0]);
            vBottomList.push_back(vBackList[3]);
            out.polygons.push_back(CreatePolygon(vBottomList, bFlip, pBasePolygon));
        }
    }
}

void StairTool::UpdateStair()
{
    if (m_fBoxHeight < 0 || m_StairMode == eStairMode_PlaceFirstPoint)
    {
        return;
    }

    float fStepRise = m_StairParameter.m_StepRise;
    bool bXDirection = m_bXDirection;
    bool bMirrored = m_StairParameter.m_bMirror;
    bool bRotationBy90Degree = m_StairParameter.m_bRotation90Degree;

    SOutputParameterForStair output;
    CreateStair(m_vStartPos, m_vEndPos, m_fBoxWidth, m_fBoxDepth, m_fBoxHeight, m_FloorPlane, fStepRise, m_bXDirection, bMirrored, bRotationBy90Degree, GetTempPolygon(), output);

    m_pCapPolygon = output.pCapPolygon;

    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(1);
    GetModel()->Clear();

    m_PolygonsNeedPostProcess = output.polygonsNeedPostProcess;
    for (int i = 0, iPolygonCount(output.polygons.size()); i < iPolygonCount; ++i)
    {
        CD::AddPolygonWithSubMatID(output.polygons[i]);
    }

    UpdateAll(CD::eUT_Mirror);
    UpdateShelf(1);
}

void StairTool::UpdateStair(BrushFloat fWidth, BrushFloat fHeight, BrushFloat fDepth)
{
    m_fBoxHeight = fHeight;
    m_fBoxWidth = m_fBoxWidth >= 0 ? fWidth : -fWidth;
    m_fBoxDepth = m_fBoxDepth >= 0 ? fDepth : -fDepth;
    UpdateStair();
}

void StairTool::FreezeModel()
{
    if (m_bIsOverOpposite)
    {
        UpdateStair();
        s_SnappingHelper.ApplyOppositePolygons(m_pCapPolygon, CD::ePP_Pull, true);
    }

    std::vector<CD::PolygonPtr> candidatePolygons;
    for (int k = 0, iCount(m_PolygonsNeedPostProcess.size()); k < iCount; ++k)
    {
        MODEL_SHELF_RECONSTRUCTOR(GetModel());

        CD::PolygonPtr pPostPolygon = m_PolygonsNeedPostProcess[k];
        CD::PolygonPtr pFlipedPostPolygon = pPostPolygon->Clone()->Flip();

        GetModel()->SetShelf(0);
        std::vector<CD::PolygonPtr> intersectedPolygons;

        for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
            if (CD::Polygon::HasIntersection(pPolygon, pFlipedPostPolygon))
            {
                intersectedPolygons.push_back(pPolygon);
                candidatePolygons.push_back(pPolygon);
            }
        }

        if (!intersectedPolygons.empty())
        {
            GetModel()->SetShelf(1);
            GetModel()->RemovePolygon(pPostPolygon);
            for (int i = 0, iPolygonCount(intersectedPolygons.size()); i < iPolygonCount; ++i)
            {
                CD::PolygonPtr pCopiedFlipedPostPolygon = pFlipedPostPolygon->Clone();
                pFlipedPostPolygon->Subtract(intersectedPolygons[i]);
                intersectedPolygons[i]->Subtract(pCopiedFlipedPostPolygon);
            }
            if (pFlipedPostPolygon->IsValid() && !pFlipedPostPolygon->IsOpen())
            {
                GetModel()->AddPolygon(pFlipedPostPolygon->Flip(), CD::eOpType_Add);
            }
        }
    }

    GetModel()->SetShelf(0);
    for (int i = 0, iCount(candidatePolygons.size()); i < iCount; ++i)
    {
        GetModel()->SeparatePolygons(candidatePolygons[i]->GetPlane());
    }

    ShapeTool::FreezeModel();
}

#include "UIs/PropertyTreePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Stair, CD::eToolGroup_Shape, "Stair", StairTool, PropertyTreePanel<StairTool>)
