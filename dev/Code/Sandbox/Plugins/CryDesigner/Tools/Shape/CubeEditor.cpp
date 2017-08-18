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
#include "Core/BSPTree3D.h"
#include "ViewManager.h"
#include "Util/PrimitiveShape.h"
#include "Material/MaterialManager.h"
#include "Util/HeightManipulator.h"
#include "CubeEditor.h"
#include "Tools/DesignerTool.h"
#include "ToolFactory.h"
#include "Core/BrushHelper.h"

using namespace CD;

namespace
{
    ICubeEditorPanel* s_pCubeEditorPanel = NULL;
}

void CubeEditor::Enter()
{
    BaseTool::Enter();
    m_BrushAABB = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0));
    m_CurMousePos = QPoint(-1, -1);
}

void CubeEditor::BeginEditParams()
{
    if (!s_pCubeEditorPanel)
    {
        s_pCubeEditorPanel = (ICubeEditorPanel*)uiPanelFactory::the().Create(Tool(), this);
    }
}

void CubeEditor::EndEditParams()
{
    CD::DestroyPanel(&s_pCubeEditorPanel);
}

BrushVec2 ConvertTwoPositionsToDirInViewport(CViewport* view, const Vec3& v0, const Vec3& v1)
{
    QPoint p0 = view->WorldToView(v0);
    QPoint p1 = view->WorldToView(v1);
    BrushVec2 vDir = BrushVec2(BrushFloat(p1.x() - p0.x()), BrushFloat(p1.y() - p0.y()));
    if (vDir.x != 0 || vDir.y != 0)
    {
        vDir.Normalize();
    }
    return vDir;
}

void CubeEditor::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    BaseTool::OnLButtonDown(view, nFlags, point);
    if (nFlags & MK_SHIFT)
    {
        m_DS.m_bPressingShift = true;
        BrushVec3 vPickedPos, vNormal;
        GetBrushPos(view, point, m_DS.m_StartingPos, vPickedPos, &m_DS.m_StartingNormal);
        m_BrushAABB = GetBrushBox(view, point);
    }
    else
    {
        m_DS.m_bPressingShift = false;
    }
}

void CubeEditor::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!m_DS.m_bPressingShift)
    {
        m_BrushAABB = GetBrushBox(view, point);
        AddBrush(m_BrushAABB);
    }

    m_DS.m_bPressingShift = false;
    m_CurMousePos = point;

    if (m_BrushAABBs.empty() || GetBaseObject() == NULL)
    {
        return;
    }

    CUndo undo("Designer : CubeEditor");
    GetModel()->RecordUndo("Designer : CubeEditor", GetBaseObject());

    bool bEmptyDesigner = GetModel()->IsEmpty();

    for (int i = 0, iBrushAABBCount(m_BrushAABBs.size()); i < iBrushAABBCount; ++i)
    {
        if (GetEditMode() == eCubeEditorMode_Add)
        {
            AddCube(m_BrushAABBs[i]);
        }
        else if (GetEditMode() == eCubeEditorMode_Remove)
        {
            RemoveCube(m_BrushAABBs[i]);
        }
        else if (GetEditMode() == eCubeEditorMode_Paint)
        {
            PaintCube(m_BrushAABBs[i]);
        }
    }

    if (bEmptyDesigner)
    {
        AABB aabb = GetModel()->GetBoundBox();
        CD::PivotToPos(GetBaseObject(), GetModel(), aabb.min);
    }

    UpdateAll();

    m_BrushAABBs.clear();
}

BrushFloat CubeEditor::GetCubeSize() const
{
    return s_pCubeEditorPanel ? s_pCubeEditorPanel->GetCubeSize() : 1.0f;
}

bool CubeEditor::IsAddMode() const
{
    return s_pCubeEditorPanel && s_pCubeEditorPanel->IsAddButtonChecked();
}

bool CubeEditor::IsRemoveMode() const
{
    return s_pCubeEditorPanel && s_pCubeEditorPanel->IsRemoveButtonChecked();
}

bool CubeEditor::IsPaintMode() const
{
    return s_pCubeEditorPanel && s_pCubeEditorPanel->IsPaintButtonChecked();
}

void CubeEditor::SelectPrevBrush() const
{
    if (s_pCubeEditorPanel)
    {
        s_pCubeEditorPanel->SelectPrevBrush();
    }
}

void CubeEditor::SelectNextBrush() const
{
    if (s_pCubeEditorPanel)
    {
        s_pCubeEditorPanel->SelectNextBrush();
    }
}

int CubeEditor::GetSubMatID() const
{
    return s_pCubeEditorPanel ? s_pCubeEditorPanel->GetSubMatID() : 0;
}

bool CubeEditor::IsSideMerged() const
{
    return s_pCubeEditorPanel && s_pCubeEditorPanel->IsSidesMerged();
}

BrushVec3 CubeEditor::Snap(const BrushVec3& vPos) const
{
    BrushVec3 vCorrectedPos(CD::CorrectVec3(vPos));
    BrushFloat fCubeSize = GetCubeSize();

    std::vector<BrushFloat> fCutUnits;
    if (std::abs(fCubeSize - (BrushFloat)0.125) < kDesignerEpsilon)
    {
        fCutUnits.push_back((BrushFloat)0.125);
        fCutUnits.push_back((BrushFloat)0.25);
        fCutUnits.push_back((BrushFloat)0.375);
        fCutUnits.push_back((BrushFloat)0.5);
        fCutUnits.push_back((BrushFloat)0.625);
        fCutUnits.push_back((BrushFloat)0.75);
        fCutUnits.push_back((BrushFloat)0.875);
    }
    else if (std::abs(fCubeSize - (BrushFloat)0.25) < kDesignerEpsilon)
    {
        fCutUnits.push_back((BrushFloat)0.25);
        fCutUnits.push_back((BrushFloat)0.5);
        fCutUnits.push_back((BrushFloat)0.75);
    }
    else if (std::abs(fCubeSize - (BrushFloat)0.5) < kDesignerEpsilon)
    {
        fCutUnits.push_back((BrushFloat)0.5);
    }

    for (int i = 0; i < 3; ++i)
    {
        for (int k = 0, iCutUnitCount(fCutUnits.size()); k < iCutUnitCount; ++k)
        {
            BrushFloat fGreatedLessThan = std::floor(vCorrectedPos[i]) + fCutUnits[k];
            if (std::abs(vCorrectedPos[i] - fGreatedLessThan) < kDesignerEpsilon)
            {
                vCorrectedPos[i] = fGreatedLessThan;
                break;
            }
        }
    }

    BrushVec3 snapped;
    snapped.x = std::floor(vCorrectedPos.x / fCubeSize) * fCubeSize;
    snapped.y = std::floor(vCorrectedPos.y / fCubeSize) * fCubeSize;
    snapped.z = std::floor(vCorrectedPos.z / fCubeSize) * fCubeSize;

    return snapped;
}

bool CubeEditor::GetBrushPos(CViewport* view, const QPoint& point, BrushVec3& outPos, BrushVec3& outPickedPos, BrushVec3* pOutNormal)
{
    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(GetWorldTM(), view, point, localRaySrc, localRayDir);
    BrushPlane plane;
    if (GetModel()->QueryPosition(localRaySrc, localRayDir, outPickedPos, &plane))
    {
        if (pOutNormal)
        {
            *pOutNormal = plane.Normal();
        }
        outPos = Snap(outPickedPos);
        return true;
    }

    if (!IsAddMode())
    {
        return false;
    }

    Vec3 vPickedPosInWorld;
    if (CD::PickPosFromWorld(view, point, vPickedPosInWorld))
    {
        BrushVec3 vLocalPickedPos = GetWorldTM().GetInverted().TransformPoint(ToBrushVec3(vPickedPosInWorld));
        if (pOutNormal)
        {
            *pOutNormal = BrushVec3(0, 0, 1);
        }
        outPickedPos = ToVec3(vPickedPosInWorld);
        outPos = Snap(vLocalPickedPos);
        return true;
    }

    return false;
}

void CubeEditor::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!m_DS.m_bPressingShift)
    {
        m_BrushAABB = GetBrushBox(view, point);
    }

    if (nFlags & MK_LBUTTON)
    {
        if (m_DS.m_bPressingShift)
        {
            BrushVec3 vWorldPivot = GetWorldTM().TransformPoint(m_DS.m_StartingPos);
            BrushVec2 vRightDirInViewport = ConvertTwoPositionsToDirInViewport(view, vWorldPivot, vWorldPivot + GetBaseObject()->GetWorldTM().GetColumn0());
            BrushVec2 vForwardDirInViewport = ConvertTwoPositionsToDirInViewport(view, vWorldPivot, vWorldPivot + GetBaseObject()->GetWorldTM().GetColumn1());
            BrushVec2 vUpDirInViewport = ConvertTwoPositionsToDirInViewport(view, vWorldPivot, vWorldPivot + GetBaseObject()->GetWorldTM().GetColumn2());

            QPoint startPosInViewport = view->WorldToView(vWorldPivot);

            BrushVec2 dir = BrushVec2(point.x() - startPosInViewport.x(), point.y() - startPosInViewport.y()).GetNormalized();

            BrushFloat angleToUp = dir.Dot(vUpDirInViewport);
            BrushFloat angleToRight = dir.Dot(vRightDirInViewport);
            BrushFloat angleToForward = dir.Dot(vForwardDirInViewport);

            BrushFloat fInvertUp = 1, fInvertRight = 1, fInvertForward = 1;
            if (angleToUp < 0)
            {
                angleToUp = dir.Dot(-vUpDirInViewport);
                fInvertUp = -1;
            }
            if (angleToRight < 0)
            {
                angleToRight = dir.Dot(-vRightDirInViewport);
                fInvertRight = -1;
            }
            if (angleToForward < 0)
            {
                angleToForward = dir.Dot(-vForwardDirInViewport);
                fInvertForward = -1;
            }

            if (angleToUp > angleToRight && angleToUp > angleToForward)
            {
                m_DS.m_StraightDir = GetWorldTM().GetColumn2() * fInvertUp;
            }
            else if (angleToRight > angleToUp && angleToRight > angleToForward)
            {
                m_DS.m_StraightDir = GetWorldTM().GetColumn0() * fInvertRight;
            }
            else
            {
                m_DS.m_StraightDir = GetWorldTM().GetColumn1() * fInvertForward;
            }

            HeightManipulator adjustHeightHelper;
            adjustHeightHelper.Init(BrushPlane(m_DS.m_StraightDir, -m_DS.m_StraightDir.Dot(m_DS.m_StartingPos)), m_DS.m_StartingPos);
            BrushFloat fLength = adjustHeightHelper.UpdateHeight(GetWorldTM(), view, point);
            BrushFloat fCubeSize = GetCubeSize();

            int nNumber = fLength / fCubeSize;
            m_BrushAABBs.clear();
            for (int i = 0; i <= nNumber; ++i)
            {
                BrushVec3 vPickedPos = m_DS.m_StartingPos + m_DS.m_StraightDir * fCubeSize * i;
                AddBrush(GetBrushBox(Snap(vPickedPos), vPickedPos, m_DS.m_StartingNormal));
            }

            m_DS.m_StraightDir *= fLength;
        }
        AddBrush(m_BrushAABB);
    }
    m_CurMousePos = point;
}

void CubeEditor::OnMouseWheel(CViewport* view, UINT nFlags, const QPoint& point)
{
    short zDelta = (short)nFlags;

    if (zDelta > 0)
    {
        SelectPrevBrush();
    }
    else if (zDelta < 0)
    {
        SelectNextBrush();
    }

    if (m_CurMousePos.x() == -1)
    {
        m_CurMousePos = point;
    }

    m_BrushAABB = GetBrushBox(view, m_CurMousePos);
}

void CubeEditor::Display(DisplayContext& dc)
{
    BaseTool::Display(dc);
    DisplayBrush(dc);
}

void CubeEditor::DisplayBrush(DisplayContext& dc)
{
    for (int i = 0, iBrushCount(m_BrushAABBs.size()); i < iBrushCount; ++i)
    {
        if (m_BrushAABBs[i].IsReset())
        {
            continue;
        }
        dc.SetColor(0.43f, 0.43f, 0, 0.43f);
        dc.DrawSolidBox(ToVec3(m_BrushAABBs[i].min), ToVec3(m_BrushAABBs[i].max));
    }

    if (!m_BrushAABB.IsReset())
    {
        dc.SetColor(0.8392f, 0.58f, 0, 0.43f);
        dc.DrawSolidBox(ToVec3(m_BrushAABB.min), ToVec3(m_BrushAABB.max));
    }
}

AABB CubeEditor::GetBrushBox(CViewport* view, const QPoint& point)
{
    BrushVec3 vBrushPos, vPickedPos, vNormal;
    if (!GetBrushPos(view, point, vBrushPos, vPickedPos, &vNormal))
    {
        AABB brushAABB;
        brushAABB.Reset();
        return brushAABB;
    }
    return GetBrushBox(vBrushPos, vPickedPos, vNormal);
}

AABB CubeEditor::GetBrushBox(const BrushVec3& vSnappedPos, const BrushVec3& vPickedPos, const BrushVec3& vNormal)
{
    AABB brushAABB;
    brushAABB.Reset();

    int nElement = -1;
    if (std::abs(vNormal.x) > (BrushFloat)0.999)
    {
        nElement = 0;
    }
    else if (std::abs(vNormal.y) > (BrushFloat)0.999)
    {
        nElement = 1;
    }
    else if (std::abs(vNormal.z) > (BrushFloat)0.999)
    {
        nElement = 2;
    }

    const float fCubeSize = ToFloat(GetCubeSize());
    Vec3 vSize(fCubeSize, fCubeSize, fCubeSize);

    AABB aabb;
    aabb.Reset();
    aabb.Add(ToVec3(vSnappedPos));
    aabb.Add(ToVec3(vSnappedPos + vSize));
    aabb.Expand(Vec3(-kDesignerEpsilon, -kDesignerEpsilon, -kDesignerEpsilon));
    if (aabb.IsContainPoint(ToVec3(vPickedPos)))
    {
        nElement = -1;
    }

    if (nElement != -1)
    {
        int nSign = vNormal[nElement] > 0 ? 1 : -1;
        if (GetEditMode() != eCubeEditorMode_Add)
        {
            nSign = -nSign;
        }
        vSize[nElement] = vSize[nElement] * nSign;
    }

    brushAABB.Add(ToVec3(vSnappedPos));
    brushAABB.Add(ToVec3(vSnappedPos + vSize));

    return brushAABB;
}

std::vector<CD::PolygonPtr> CubeEditor::GetBrushPolygons(const AABB& aabb) const
{
    std::vector<CD::PolygonPtr> brushPolygons;
    if (!aabb.IsReset())
    {
        PrimitiveShape bp;
        bp.CreateBox(aabb.min, aabb.max, &brushPolygons);

        int nMatID = GetSubMatID();
        for (int i = 0, iPolygonCount(brushPolygons.size()); i < iPolygonCount; ++i)
        {
            brushPolygons[i]->SetMaterialID(nMatID);
        }
    }
    return brushPolygons;
}

void CubeEditor::AddCube(const AABB& brushAABB)
{
    std::vector<CD::PolygonPtr> brushPolygons = GetBrushPolygons(brushAABB);
    if (brushPolygons.empty())
    {
        return;
    }

    AABB reducedBrushAABB = brushAABB;
    AABB enlargedBrushAABB = brushAABB;
    const float kOffset = (float)kDesignerEpsilon;
    reducedBrushAABB.Expand(Vec3(-kOffset, -kOffset, -kOffset));
    enlargedBrushAABB.Expand(Vec3(kOffset, kOffset, kOffset));
    std::vector<CD::PolygonPtr> intersectedPolygons;
    if (GetModel()->QueryIntersectedPolygonsByAABB(reducedBrushAABB, intersectedPolygons))
    {
        std::vector<CD::PolygonPtr> originalIntersectedPolygons(intersectedPolygons);

        std::vector<CD::PolygonPtr>::iterator ii = intersectedPolygons.begin();
        for (; ii != intersectedPolygons.end(); )
        {
            if (brushAABB.ContainsBox((*ii)->GetBoundBox()))
            {
                GetModel()->RemovePolygon(*ii);
                ii = intersectedPolygons.erase(ii);
            }
            else
            {
                ++ii;
            }
        }

        if (!intersectedPolygons.empty())
        {
            _smart_ptr<CD::BSPTree3D> pBSPTreeForBrush = new CD::BSPTree3D(brushPolygons);
            for (ii = intersectedPolygons.begin(); ii != intersectedPolygons.end(); ++ii)
            {
                CD::SOutputPolygons output;
                pBSPTreeForBrush->GetPartitions(*ii, output);
                if (!output.negList.empty())
                {
                    GetModel()->RemovePolygon(*ii);
                    for (int i = 0, iPosPolygonCount(output.posList.size()); i < iPosPolygonCount; ++i)
                    {
                        GetModel()->AddPolygon(output.posList[i], CD::eOpType_Union);
                    }
                }
            }

            CD::Model brushModel(brushPolygons);
            _smart_ptr<CD::BSPTree3D> pBSPTree = new CD::BSPTree3D(originalIntersectedPolygons);
            for (int i = 0, iPolygonCount(brushPolygons.size()); i < iPolygonCount; ++i)
            {
                CD::SOutputPolygons output;
                pBSPTree->GetPartitions(brushPolygons[i], output);
                if (!output.negList.empty())
                {
                    brushModel.RemovePolygon(brushPolygons[i]);
                    for (int k = 0, iPosPolygonCount(output.posList.size()); k < iPosPolygonCount; ++k)
                    {
                        brushModel.AddPolygon(output.posList[k], CD::eOpType_Union);
                    }
                }
            }
            brushPolygons.clear();
            brushModel.GetPolygonList(brushPolygons);
        }
    }

    for (int i = 0, iPolygonCount(brushPolygons.size()); i < iPolygonCount; ++i)
    {
        if (brushPolygons[i] == NULL)
        {
            continue;
        }
        const BrushPlane& plane = brushPolygons[i]->GetPlane();
        std::vector<CD::PolygonPtr> candidatePolygons;
        GetModel()->QueryPolygons(plane, candidatePolygons);
        if (!candidatePolygons.empty())
        {
            CD::PolygonPtr pPolygon = brushPolygons[i];
            std::vector<CD::PolygonPtr> touchedPolygons;
            GetModel()->QueryIntersectionByPolygon(pPolygon, touchedPolygons);
            if (!touchedPolygons.empty())
            {
                if (IsSideMerged())
                {
                    GetModel()->AddPolygon(pPolygon, CD::eOpType_Union);
                }
                else
                {
                    GetModel()->AddPolygon(pPolygon, CD::eOpType_Split);
                }
                continue;
            }
        }

        BrushPlane invPlane = plane.GetInverted();
        candidatePolygons.clear();
        GetModel()->QueryPolygons(invPlane, candidatePolygons);
        if (!candidatePolygons.empty())
        {
            CD::PolygonPtr pInvertedPolygon = brushPolygons[i]->Clone()->Flip();
            std::vector<CD::PolygonPtr> touchedPolygons;
            GetModel()->QueryIntersectionByPolygon(pInvertedPolygon, touchedPolygons);
            if (!touchedPolygons.empty())
            {
                for (int k = 0, iTouchedPolygonCount(touchedPolygons.size()); k < iTouchedPolygonCount; ++k)
                {
                    if (enlargedBrushAABB.ContainsBox(touchedPolygons[k]->GetBoundBox()))
                    {
                        GetModel()->RemovePolygon(touchedPolygons[k]);
                        std::vector<CD::PolygonPtr> outsidePolygons;
                        if (touchedPolygons[k]->GetSeparatedPolygons(outsidePolygons, CD::eSP_OuterHull))
                        {
                            for (int a = 0, iOutsidePolygonCount(outsidePolygons.size()); a < iOutsidePolygonCount; ++a)
                            {
                                pInvertedPolygon->Subtract(outsidePolygons[a]);
                            }
                        }
                    }
                    else if (Polygon::HasIntersection(touchedPolygons[k], pInvertedPolygon) == CD::eIT_Intersection)
                    {
                        std::vector<CD::PolygonPtr> outsidePolygons;
                        touchedPolygons[k]->GetSeparatedPolygons(outsidePolygons, CD::eSP_OuterHull);

                        touchedPolygons[k]->Subtract(pInvertedPolygon);

                        for (int a = 0, iOutsidePolygonCount(outsidePolygons.size()); a < iOutsidePolygonCount; ++a)
                        {
                            pInvertedPolygon->Subtract(outsidePolygons[a]);
                        }

                        if (touchedPolygons[k]->IsValid())
                        {
                            GetModel()->AddPolygonSeparately(touchedPolygons[k], true);
                        }
                        else
                        {
                            GetModel()->RemovePolygon(touchedPolygons[k]);
                        }
                    }
                }
                if (pInvertedPolygon->IsValid() && !pInvertedPolygon->IsOpen())
                {
                    GetModel()->AddPolygonSeparately(pInvertedPolygon->Flip());
                }
                continue;
            }
        }

        if (IsSideMerged())
        {
            GetModel()->AddPolygon(brushPolygons[i], CD::eOpType_Union);
        }
        else
        {
            GetModel()->AddPolygon(brushPolygons[i], CD::eOpType_Add);
        }
    }
}

void CubeEditor::RemoveCube(const AABB& brushAABB)
{
    std::vector<CD::PolygonPtr> brushPolygons = GetBrushPolygons(brushAABB);
    if (brushPolygons.empty())
    {
        return;
    }

    std::vector<CD::PolygonPtr> intersectedPolygons;
    if (!GetModel()->QueryIntersectedPolygonsByAABB(brushAABB, intersectedPolygons))
    {
        return;
    }

    if (intersectedPolygons.empty())
    {
        return;
    }

    std::vector<CD::PolygonPtr> brushPolygonsForBSPTree;
    CD::CopyPolygons(brushPolygons, brushPolygonsForBSPTree);
    _smart_ptr<CD::BSPTree3D> pBrushBSP3D = new CD::BSPTree3D(brushPolygonsForBSPTree);

    std::vector<CD::PolygonPtr> boundaryPolygons;
    CD::CopyPolygons(intersectedPolygons, boundaryPolygons);

    std::vector<CD::PolygonPtr>::iterator ii = boundaryPolygons.begin();
    for (; ii != boundaryPolygons.end(); )
    {
        bool bBoundaryPolygon = true;
        for (int k = 0, iBrushPolygonCount(brushPolygons.size()); k < iBrushPolygonCount; ++k)
        {
            if (brushPolygons[k]->GetPlane().GetInverted().IsEquivalent((*ii)->GetPlane()))
            {
                bBoundaryPolygon = false;
                break;
            }
        }
        if (!bBoundaryPolygon)
        {
            ii = boundaryPolygons.erase(ii);
        }
        else
        {
            ++ii;
        }
    }

    if (boundaryPolygons.empty())
    {
        return;
    }

    _smart_ptr<CD::BSPTree3D> pBoundaryBSPTree = new CD::BSPTree3D(boundaryPolygons);

    CD::Model model;
    std::vector<CD::PolygonPtr>::iterator iBrush = brushPolygons.begin();
    for (; iBrush != brushPolygons.end(); )
    {
        bool bSubtracted = false;
        std::vector<CD::PolygonPtr>::iterator ii = intersectedPolygons.begin();
        for (; ii != intersectedPolygons.end(); )
        {
            if (!(*iBrush)->IsPlaneEquivalent(*ii) || Polygon::HasIntersection(*iBrush, *ii) != CD::eIT_Intersection)
            {
                ++ii;
                continue;
            }
            bSubtracted = true;
            CD::PolygonPtr pClone = (*ii)->Clone();
            (*ii)->Subtract(*iBrush);
            (*iBrush)->Subtract(pClone);
            GetModel()->RemovePolygon(*ii);
            if (!(*ii)->IsValid())
            {
                ii = intersectedPolygons.erase(ii);
            }
            else
            {
                model.AddPolygon(*ii, CD::eOpType_Union);
                ++ii;
            }
        }

        if (bSubtracted && !(*iBrush)->IsValid())
        {
            iBrush = brushPolygons.erase(iBrush);
        }
        else
        {
            ++iBrush;
        }
    }

    for (std::vector<CD::PolygonPtr>::iterator ii = intersectedPolygons.begin(); ii != intersectedPolygons.end(); )
    {
        if (brushAABB.ContainsBox((*ii)->GetBoundBox()))
        {
            bool bTouched = false;
            for (int i = 0, iBrushPolygonCount(brushPolygons.size()); i < iBrushPolygonCount; ++i)
            {
                BrushPlane invertedBrushPlane = brushPolygons[i]->GetPlane().GetInverted();
                if (invertedBrushPlane.IsEquivalent((*ii)->GetPlane()))
                {
                    bTouched = true;
                    break;
                }
            }
            if (!bTouched)
            {
                GetModel()->RemovePolygon(*ii);
                ii = intersectedPolygons.erase(ii);
            }
            else
            {
                ++ii;
            }
        }
        else if (brushAABB.IsIntersectBox((*ii)->GetBoundBox()))
        {
            CD::SOutputPolygons output;
            pBrushBSP3D->GetPartitions(*ii, output);
            if (!output.negList.empty())
            {
                GetModel()->RemovePolygon(*ii);
                for (int i = 0, iPosCount(output.posList.size()); i < iPosCount; ++i)
                {
                    model.AddPolygon(output.posList[i], CD::eOpType_Union);
                }
                ii = intersectedPolygons.erase(ii);
            }
            else
            {
                ++ii;
            }
        }
        else
        {
            ++ii;
        }
    }

    for (int i = 0, iBrushPolygonCount(brushPolygons.size()); i < iBrushPolygonCount; ++i)
    {
        CD::PolygonPtr pInvertedPolygon = brushPolygons[i]->Clone()->Flip();

        CD::SOutputPolygons output;
        pBoundaryBSPTree->GetPartitions(pInvertedPolygon, output);
        if (output.negList.empty())
        {
            continue;
        }

        if (!output.posList.empty() || !output.coSameList.empty() || !output.coDiffList.empty())
        {
            for (int k = 0, negListCount(output.negList.size()); k < negListCount; ++k)
            {
                model.AddPolygon(output.negList[k], CD::eOpType_Union);
            }
        }
        else
        {
            model.AddPolygon(pInvertedPolygon, CD::eOpType_Union);
        }
    }

    for (int i = 0, iPolygonCount(model.GetPolygonCount()); i < iPolygonCount; ++i)
    {
        if (IsSideMerged() || !pBoundaryBSPTree->IsValidTree())
        {
            GetModel()->AddPolygon(model.GetPolygon(i), CD::eOpType_Union);
        }
        else
        {
            GetModel()->AddPolygonSeparately(model.GetPolygon(i));
        }
    }
}

void CubeEditor::PaintCube(const AABB& brushAABB)
{
    std::vector<CD::PolygonPtr> brushPolygons = GetBrushPolygons(brushAABB);
    if (brushPolygons.empty())
    {
        return;
    }

    std::vector<CD::PolygonPtr> candidatePolygons;
    GetModel()->QueryIntersectedPolygonsByAABB(brushAABB, candidatePolygons);
    if (candidatePolygons.empty())
    {
        return;
    }

    std::vector<CD::PolygonPtr>::iterator ii = candidatePolygons.begin();
    for (; ii != candidatePolygons.end(); )
    {
        if (brushAABB.ContainsBox((*ii)->GetBoundBox()))
        {
            (*ii)->SetMaterialID(GetSubMatID());
            ii = candidatePolygons.erase(ii);
        }
        else
        {
            ++ii;
        }
    }

    if (candidatePolygons.empty())
    {
        return;
    }

    _smart_ptr<CD::BSPTree3D> pBrushTree = new CD::BSPTree3D(brushPolygons);

    CD::Model designer[2];
    for (int i = 0, iCandidatePolygonCount(candidatePolygons.size()); i < iCandidatePolygonCount; ++i)
    {
        CD::SOutputPolygons output;
        pBrushTree->GetPartitions(candidatePolygons[i], output);

        std::vector<CD::PolygonPtr> polygonsInside;
        polygonsInside.insert(polygonsInside.end(), output.negList.begin(), output.negList.end());
        polygonsInside.insert(polygonsInside.end(), output.coSameList.begin(), output.coSameList.end());
        polygonsInside.insert(polygonsInside.end(), output.coDiffList.begin(), output.coDiffList.end());

        if (polygonsInside.empty())
        {
            continue;
        }

        GetModel()->RemovePolygon(candidatePolygons[i]);
        for (int k = 0, iPosPolygonCount(output.posList.size()); k < iPosPolygonCount; ++k)
        {
            designer[0].AddPolygon(output.posList[k], CD::eOpType_Union);
        }

        for (int k = 0, iPolygonCountInside(polygonsInside.size()); k < iPolygonCountInside; ++k)
        {
            polygonsInside[k]->SetMaterialID(GetSubMatID());
            designer[1].AddPolygon(polygonsInside[k], CD::eOpType_Union);
        }
    }

    for (int i = 0; i < 2; ++i)
    {
        for (int k = 0, iPolygonCount(designer[i].GetPolygonCount()); k < iPolygonCount; ++k)
        {
            GetModel()->AddPolygonSeparately(designer[i].GetPolygon(k));
        }
    }
}

ECubeEditorMode CubeEditor::GetEditMode() const
{
    if (IsAddMode())
    {
        return eCubeEditorMode_Add;
    }
    else if (IsRemoveMode())
    {
        return eCubeEditorMode_Remove;
    }
    else if (IsPaintMode())
    {
        return eCubeEditorMode_Paint;
    }

    return eCubeEditorMode_Invalid;
}

void CubeEditor::MaterialChanged()
{
    if (s_pCubeEditorPanel)
    {
        s_pCubeEditorPanel->UpdateSubMaterialComboBox();
    }
}

void CubeEditor::SetSubMatID(int nSubMatID)
{
    if (s_pCubeEditorPanel)
    {
        s_pCubeEditorPanel->SetSubMatID(nSubMatID + 1);
    }
}

void CubeEditor::AddBrush(const AABB& aabb)
{
    if (aabb.IsReset())
    {
        return;
    }
    bool bHaveSame = false;
    for (int i = 0, iCount(m_BrushAABBs.size()); i < iCount; ++i)
    {
        if (CD::IsEquivalent(m_BrushAABBs[i].min, aabb.min) && CD::IsEquivalent(m_BrushAABBs[i].max, aabb.max))
        {
            bHaveSame = true;
            break;
        }
    }
    if (!bHaveSame)
    {
        m_BrushAABBs.push_back(aabb);
    }
}

void CubeEditor::OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event)
{
    if (pItem == NULL)
    {
        return;
    }

    if (event == EDB_ITEM_EVENT_SELECTED)
    {
        if (s_pCubeEditorPanel)
        {
            s_pCubeEditorPanel->SetMaterial((CMaterial*)pItem);
        }
    }
}

#include "UIs/CubeEditorPanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_CubeEditor, CD::eToolGroup_Shape, "Cube Editor", CubeEditor, CubeEditorPanel)