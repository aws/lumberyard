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
#include "ExtrudeTool.h"
#include "ViewManager.h"
#include "Core/Model.h"
#include "Core/BrushHelper.h"
#include "Util/HeightManipulator.h"
#include "Util/ExtrusionSnappingHelper.h"
#include "Tools/DesignerTool.h"

using namespace CD;

void ExtrudeTool::Enter()
{
    BaseTool::Enter();
    m_ec.pushPull = ePP_None;
    m_LButtonInfo = SLButtonInfo();
    GetIEditor()->ShowTransformManipulator(false);
}

void ExtrudeTool::Leave()
{
    m_pScaledPolygon->Init();
    BaseTool::Leave();
}

bool ExtrudeTool::StartPushPull(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_ec.pArgumentModel = NULL;
    m_ec.pushPull = m_ec.initPushPull = ePP_None;

    if (GetPickedPolygon())
    {
        GetIEditor()->BeginUndo();
        GetModel()->RecordUndo("Designer : Extrusion", GetBaseObject());

        BrushVec3 localRaySrc;
        BrushVec3 localRayDir;
        GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);
        BrushVec3 vPivot;
        if (GetModel()->QueryPosition(localRaySrc, localRayDir, vPivot))
        {
            s_HeightManipulator.Init(GetPlane(), vPivot);
        }
        else
        {
            s_HeightManipulator.Init(GetPlane(), GetPickedPolygon()->GetCenterPosition());
        }

        m_ec.pCompiler = GetCompiler();
        m_ec.pObject = GetBaseObject();
        m_ec.pModel = GetModel();
        m_ec.pPolygon = GetPickedPolygon();

        return PrepareExtrusion(m_ec);
    }
    return false;
}

bool ExtrudeTool::PrepareExtrusion(SExtrusionContext& ec)
{
    DESIGNER_ASSERT(ec.pPolygon);
    if (!ec.pPolygon)
    {
        return false;
    }

    ec.backupPolygons.clear();
    ec.pModel->QueryPerpendicularPolygons(ec.pPolygon, ec.backupPolygons);
    RemovePolygonWithSpecificFlagsFromList(ec.pModel, ec.backupPolygons, ePolyFlag_Mirrored);
    if (ec.pModel->CheckModeFlag(eDesignerMode_Mirror))
    {
        ec.mirroredBackupPolygons.clear();
        ec.pModel->QueryPerpendicularPolygons(ec.pPolygon->Clone()->Mirror(ec.pModel->GetMirrorPlane()), ec.mirroredBackupPolygons);
        RemovePolygonWithoutSpecificFlagsFromList(ec.pModel, ec.mirroredBackupPolygons, ePolyFlag_Mirrored);
    }

    MakeArgumentBrush(ec);
    if (ec.pArgumentModel)
    {
        s_SnappingHelper.SearchForOppositePolygons(ec.pArgumentModel->GetCapPolygon());
        MODEL_SHELF_RECONSTRUCTOR(ec.pModel);
        ec.pModel->SetShelf(0);
        ec.pModel->DrillPolygon(ec.pPolygon);
        DrillMirroredPolygon(ec.pModel, ec.pPolygon);
        ec.bFirstUpdate = true;
        return true;
    }

    return false;
}

void ExtrudeTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp))
    {
        return;
    }

    s_SnappingHelper.Init(GetModel());
    m_ec.pArgumentModel = NULL;
    m_LButtonInfo.m_LastAction = CD::eMouseAction_LButtonDown;
    m_LButtonInfo.m_DownTimeStamp = GetTickCount();
    m_LButtonInfo.m_DownPos = point;
    m_ec.bTouchedMirrorPlane = false;
}

void ExtrudeTool::RaiseHeight(const QPoint& point, CViewport* view, int nFlags)
{
    BrushFloat fHeight = s_HeightManipulator.UpdateHeight(GetWorldTM(), view, point);

    m_ec.pArgumentModel->SetHeight(fHeight);
    m_ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
    m_ec.bTouchedMirrorPlane = false;

    if (!GetModel()->CheckModeFlag(eDesignerMode_Mirror))
    {
        return;
    }

    PolygonPtr pCapPolygon = m_ec.pArgumentModel->GetCapPolygon();
    for (int i = 0, iVertexCount(pCapPolygon->GetVertexCount()); i < iVertexCount; ++i)
    {
        const BrushVec3& v = pCapPolygon->GetPos(i);
        BrushFloat distFromVertexToMirrorPlane = GetModel()->GetMirrorPlane().Distance(v);
        if (distFromVertexToMirrorPlane <= -kDesignerEpsilon)
        {
            continue;
        }
        BrushFloat distFromVertexMirrorPlaneInCapNormalDir = 0;
        GetModel()->GetMirrorPlane().HitTest(v, v + pCapPolygon->GetPlane().Normal(), &distFromVertexMirrorPlaneInCapNormalDir);
        m_ec.pArgumentModel->SetHeight(fHeight + distFromVertexMirrorPlaneInCapNormalDir);
        m_ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
        m_ec.bTouchedMirrorPlane = true;
        break;
    }
}

void ExtrudeTool::RaiseLowerPolygon(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!(nFlags & MK_LBUTTON))
    {
        return;
    }

    BrushFloat fPrevHeight = m_ec.pArgumentModel->GetHeight();

    if (nFlags & MK_SHIFT)
    {
        // When the mouse cursor is in the selected polygon, push/pull should be done.
        // And the cursor is outside the selected polygon and it is located on another polygon, the height of the selected polygon should be same as the height of the another polygon.
        BrushVec3 localRaySrc;
        BrushVec3 localRayDir;
        GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);
        BrushFloat outT(0);
        if (!m_ec.pArgumentModel->GetCapPolygon()->IsPassed(localRaySrc, localRayDir, outT))
        {
            AlignHeight(m_ec.pArgumentModel->GetCapPolygon(), view, point);
        }
    }
    else
    {
        RaiseHeight(point, view, nFlags);
    }

    BrushFloat fHeight = m_ec.pArgumentModel->GetHeight();
    BrushFloat fHeightDifference(fHeight - fPrevHeight);
    if (fHeightDifference > 0)
    {
        m_ec.pushPull = ePP_Pull;
    }
    else if (fHeightDifference < 0)
    {
        m_ec.pushPull = ePP_Push;
    }

    if (m_ec.initPushPull == ePP_None)
    {
        m_ec.initPushPull = m_ec.pushPull;
    }

    CheckBoundary(m_ec);
    UpdateDesigner(m_ec);
}

void ExtrudeTool::ResizePolygon(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (!GetPickedPolygon() || GetPickedPolygon()->CheckFlags(ePolyFlag_NonplanarQuad))
    {
        return;
    }

    if (m_ResizeStatus == eRS_None)
    {
        m_ResizeStartScreenPos = BrushVec2(point.x(), point.y());
        m_ResizeStatus = eRS_Resizing;
        m_ec.fScale = 0;
    }

    BrushVec3 localRaySrc, localRayDir;
    GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);

    BrushVec3 crossPoint;
    if (!GetPickedPolygon()->GetPlane().HitTest(localRaySrc, localRaySrc + localRayDir, NULL, &crossPoint))
    {
        return;
    }

    BrushFloat fPrevDifference(m_ec.fScale);

    m_ec.fScale = (m_ResizeStartScreenPos.y - (BrushFloat)point.y()) * 0.01f;
    if (std::abs(m_ec.fScale) < 0.1f)
    {
        m_ec.fScale = 0;
    }

    CD::Polygon polygon = *GetPickedPolygon();
    if (polygon.Scale(m_ec.fScale, true))
    {
        *m_pScaledPolygon = polygon;
    }
    else
    {
        BrushFloat fAdjustedScale(m_ec.fScale);
        BinarySearchForScale(fPrevDifference, m_ec.fScale, 0, polygon, fAdjustedScale);
        if (polygon.Scale(fAdjustedScale, true))
        {
            if (fAdjustedScale > 0)
            {
                m_ec.fScale = fAdjustedScale - kDesignerEpsilon * 100.0f;
            }
            else
            {
                m_ec.fScale = fAdjustedScale + kDesignerEpsilon * 100.0f;
            }
            *m_pScaledPolygon = polygon;
        }
        else
        {
            DESIGNER_ASSERT(0);
        }
    }
}

void ExtrudeTool::SelectPolygon(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (GetBaseObject() == NULL)
    {
        return;
    }

    m_ResizeStatus = eRS_None;

    BrushVec3 localRaySrc, localRayDir;
    GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);

    int nPolygonIndex(0);
    if (GetModel()->QueryPolygon(localRaySrc, localRayDir, nPolygonIndex))
    {
        PolygonPtr pCandidatePolygon = GetModel()->GetPolygon(nPolygonIndex);
        if (pCandidatePolygon != GetPickedPolygon() && !pCandidatePolygon->CheckFlags(ePolyFlag_Mirrored) && !pCandidatePolygon->IsOpen())
        {
            SetPickedPolygon(pCandidatePolygon);
            SetPlane(pCandidatePolygon->GetPlane());
            *m_pScaledPolygon = *GetPickedPolygon();
            m_ec.fScale = 0;
            CD::GetDesigner()->UpdateSelectionMesh(GetPickedPolygon(), GetCompiler(), GetBaseObject());
        }
    }
    else
    {
        SetPickedPolygon(NULL);
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject());
    }
}

void ExtrudeTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (nFlags & MK_LBUTTON)
    {
        if (m_LButtonInfo.m_LastAction == CD::eMouseAction_LButtonDown && (IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) || !IsTwoPointEquivalent(m_LButtonInfo.m_DownPos, point)))
        {
            if (m_ec.pArgumentModel)
            {
                RaiseLowerPolygon(view, nFlags, point);
            }
            else if (m_LButtonInfo.m_LastAction == CD::eMouseAction_LButtonDown)
            {
                if (StartPushPull(view, nFlags, point))
                {
                    CD::GetDesigner()->UpdateSelectionMesh(m_pScaledPolygon, GetCompiler(), GetBaseObject());
                }
            }
        }
    }
    else
    {
        if (nFlags & MK_SHIFT)
        {
            ResizePolygon(view, nFlags, point);
        }
        else
        {
            SelectPolygon(view, nFlags, point);
        }
    }
}

void ExtrudeTool::Display(DisplayContext& dc)
{
    BaseTool::Display(dc);

    if (m_ResizeStatus == eRS_Resizing)
    {
        dc.SetColor(kResizedPolygonColor);
        for (int i = 0, iEdgeSize(m_pScaledPolygon->GetEdgeCount()); i < iEdgeSize; ++i)
        {
            BrushEdge3D edge = m_pScaledPolygon->GetEdge(i);
            dc.DrawLine(ToVec3(edge.m_v[0]), ToVec3(edge.m_v[1]));
        }
    }

    if (m_ec.pArgumentModel)
    {
        if (gDesignerSettings.bDisplayDimensionHelper)
        {
            Matrix34 poppedTM = dc.GetMatrix();
            dc.PopMatrix();
            GetBaseObject()->DrawDimensionsImpl(dc, m_ec.pArgumentModel->GetBoundBox());
            dc.PushMatrix(poppedTM);
        }
    }

    s_HeightManipulator.Display(dc);
}

void ExtrudeTool::CheckBoundary(SExtrusionContext& ec)
{
    if (ec.pushPull == ePP_None || ec.pArgumentModel == NULL)
    {
        return;
    }

    ec.bIsLocatedAtOpposite = false;

    if (s_SnappingHelper.IsOverOppositePolygon(ec.pArgumentModel->GetCapPolygon(), ec.pushPull))
    {
        ec.pArgumentModel->SetHeight(s_SnappingHelper.GetNearestDistanceToOpposite(ec.pushPull));
        ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
        ec.bIsLocatedAtOpposite = true;
    }
}

void ExtrudeTool::MakeArgumentBrush(SExtrusionContext& ec)
{
    if (ec.pPolygon == NULL)
    {
        return;
    }

    ec.pArgumentModel = new ArgumentModel(ec.pPolygon, ec.fScale, ec.pObject, &ec.backupPolygons, ec.pModel->GetDB());
    ec.pArgumentModel->SetHeight(0);
    ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
}

void ExtrudeTool::FinishPushPull(SExtrusionContext& ec)
{
    if (!ec.pArgumentModel)
    {
        return;
    }

    if (!ec.bIsLocatedAtOpposite)
    {
        ec.pModel->MoveShelf(1, 0);
    }
    else
    {
        s_SnappingHelper.ApplyOppositePolygons(ec.pArgumentModel->GetCapPolygon(), ec.pushPull);
        ec.pModel->MoveShelf(1, 0);
        if (CD::GetDesigner())
        {
            CD::GetDesigner()->UpdateSelectionMesh(NULL, ec.pCompiler, ec.pObject, true);
        }
        UpdateMirroredPartWithPlane(ec.pModel, ec.pArgumentModel->GetCapPolygon()->GetPlane());
        UpdateMirroredPartWithPlane(ec.pModel, ec.pArgumentModel->GetCapPolygon()->GetPlane().GetInverted());
        ec.bIsLocatedAtOpposite = false;
    }

    ec.pModel->Optimize();
    CD::UpdateAll(ec, eUT_ExceptSyncPrefab);
    ec.pArgumentModel = NULL;
}

bool ExtrudeTool::AlignHeight(PolygonPtr pCapPolygon, CViewport* view, const QPoint& point)
{
    PolygonPtr pAlignedPolygon = s_SnappingHelper.FindAlignedPolygon(pCapPolygon, GetWorldTM(), view, point);
    if (!pAlignedPolygon)
    {
        return false;
    }

    BrushFloat updatedHeight = m_ec.pArgumentModel->GetBasePlane().Distance() - pAlignedPolygon->GetPlane().Distance();
    if (updatedHeight != m_ec.pArgumentModel->GetHeight())
    {
        m_ec.pArgumentModel->SetHeight(updatedHeight);
        m_ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
        m_ec.pArgumentModel->GetCapPolygon()->UpdatePlane(m_ec.pArgumentModel->GetCurrentCapPlane());
    }

    return true;
}

void ExtrudeTool::OnLButtonUp(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_LButtonInfo.m_LastAction == CD::eMouseAction_LButtonDoubleClick)
    {
        return;
    }

    m_LButtonInfo.m_LastAction = CD::eMouseAction_LButtonUp;

    if (!IsOverDoubleClickTime(m_LButtonInfo.m_DownTimeStamp) && IsTwoPointEquivalent(m_LButtonInfo.m_DownPos, point))
    {
        return;
    }

    if (m_ec.pArgumentModel)
    {
        if (m_ec.pushPull != ePP_None)
        {
            m_PrevAction.m_Type = m_ec.pushPull;
            m_PrevAction.m_Distance = m_ec.pArgumentModel ? m_ec.pArgumentModel->GetHeight() : 0;
        }

        CheckBoundary(m_ec);
        if (m_ec.pArgumentModel)
        {
            FinishPushPull(m_ec);
            GetIEditor()->AcceptUndo("Designer : Extrude");
        }
        else
        {
            GetIEditor()->CancelUndo();
        }
        UpdateAll(eUT_SyncPrefab);
    }
}

void ExtrudeTool::OnLButtonDblClk(CViewport* view, UINT nFlags, const QPoint& point)
{
    m_LButtonInfo.m_LastAction = CD::eMouseAction_LButtonDoubleClick;
    if (m_ec.pArgumentModel)
    {
        m_ec.pArgumentModel = NULL;
        return;
    }

    if (!StartPushPull(view, nFlags, point))
    {
        return;
    }

    if (std::abs(m_PrevAction.m_Distance) > kDesignerEpsilon && m_PrevAction.m_Type != ePP_None)
    {
        m_ec.pushPull = m_PrevAction.m_Type;
        m_ec.pArgumentModel->SetHeight(m_PrevAction.m_Distance);
        m_ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
    }

    m_ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
    CheckBoundary(m_ec);
    UpdateDesigner(m_ec);
    if (m_ec.pArgumentModel)
    {
        FinishPushPull(m_ec);
        GetIEditor()->AcceptUndo("Designer : Extrude");
    }
    else
    {
        GetIEditor()->CancelUndo();
    }
    UpdateAll(eUT_SyncPrefab);

    m_ec.pArgumentModel = NULL;
}

void ExtrudeTool::Extrude(SMainContext& mc, PolygonPtr pPolygon, float fHeight, float fScale)
{
    SExtrusionContext ec;
    ec.pObject = mc.pObject;
    ec.pCompiler = mc.pCompiler;
    ec.pModel = mc.pModel;
    ec.pPolygon = pPolygon;
    ec.pushPull = ec.initPushPull = fHeight > 0 ? ePP_Pull : ePP_Push;
    ec.fScale = fScale;
    ec.bUpdateBrush = false;
    s_SnappingHelper.Init(ec.pModel);
    PrepareExtrusion(ec);
    ec.pArgumentModel->SetHeight(fHeight);
    ec.pArgumentModel->Update(ArgumentModel::eBAU_None);
    CheckBoundary(ec);
    UpdateDesigner(ec);
    FinishPushPull(ec);
}

void ExtrudeTool::UpdateDesigner(SExtrusionContext& ec)
{
    MODEL_SHELF_RECONSTRUCTOR(ec.pModel);

    if (!ec.pArgumentModel)
    {
        return;
    }

    if (ec.bFirstUpdate)
    {
        ec.pModel->SetShelf(0);
        for (int i = 0, iPolygonCount(ec.backupPolygons.size()); i < iPolygonCount; ++i)
        {
            ec.pModel->RemovePolygon(ec.backupPolygons[i]);
        }
        for (int i = 0, iPolygonCount(ec.mirroredBackupPolygons.size()); i < iPolygonCount; ++i)
        {
            ec.pModel->RemovePolygon(ec.mirroredBackupPolygons[i]);
        }
    }

    ec.pModel->SetShelf(1);
    ec.pModel->Clear();
    for (int i = 0, iPolygonCount(ec.backupPolygons.size()); i < iPolygonCount; ++i)
    {
        ec.pModel->AddPolygon(ec.backupPolygons[i]->Clone(), eOpType_Add);
    }

    const BrushPlane& mirrorPlane = ec.pModel->GetMirrorPlane();
    BrushPlane invertedMirrorPlane = mirrorPlane.GetInverted();

    std::vector<PolygonPtr> sidePolygons;
    if (ec.pArgumentModel->GetSidePolygonList(sidePolygons))
    {
        for (int i = 0, iSidePolygonSize(sidePolygons.size()); i < iSidePolygonSize; ++i)
        {
            if (mirrorPlane.IsEquivalent(sidePolygons[i]->GetPlane()) || invertedMirrorPlane.IsEquivalent(sidePolygons[i]->GetPlane()))
            {
                continue;
            }

            bool bOperated = false;
            PolygonPtr pSidePolygon = sidePolygons[i];
            PolygonPtr pSideFlipedPolygon = sidePolygons[i]->Clone()->Flip();

            std::set<int> removedBackUpPolygons;

            for (int k = 0, iBackupPolygonSize(ec.backupPolygons.size()); k < iBackupPolygonSize; ++k)
            {
                bool bIncluded = pSidePolygon->IncludeAllEdges(ec.backupPolygons[k]);
                bool bFlippedIncluded = pSideFlipedPolygon->IncludeAllEdges(ec.backupPolygons[k]);
                if (!bIncluded && !bFlippedIncluded)
                {
                    continue;
                }
                ec.pModel->RemovePolygon(ec.pModel->QueryEquivalentPolygon(ec.backupPolygons[k]));
                if (bIncluded)
                {
                    pSidePolygon->Subtract(ec.backupPolygons[k]);
                    pSideFlipedPolygon->Subtract(ec.backupPolygons[k]->Clone()->Flip());
                }
                if (bFlippedIncluded)
                {
                    pSideFlipedPolygon->Subtract(ec.backupPolygons[k]);
                    pSidePolygon->Subtract(ec.backupPolygons[k]->Clone()->Flip());
                }
                removedBackUpPolygons.insert(k);
            }

            if (!pSidePolygon->IsValid() || !pSideFlipedPolygon->IsValid())
            {
                continue;
            }

            for (int k = 0, iBackupPolygonSize(ec.backupPolygons.size()); k < iBackupPolygonSize; ++k)
            {
                if (removedBackUpPolygons.find(k) != removedBackUpPolygons.end())
                {
                    continue;
                }

                EIntersectionType it = Polygon::HasIntersection(ec.backupPolygons[k], pSidePolygon);
                EIntersectionType itFliped = Polygon::HasIntersection(ec.backupPolygons[k], pSideFlipedPolygon);

                if (it == eIT_None && itFliped == eIT_None)
                {
                    continue;
                }

                PolygonPtr pThisSidePolygon = pSidePolygon;
                if (itFliped == eIT_Intersection || itFliped == eIT_JustTouch && ec.pArgumentModel->GetHeight() < 0)
                {
                    it = itFliped;
                    pThisSidePolygon = pSideFlipedPolygon;
                }

                if (it == eIT_Intersection || it == eIT_JustTouch && ec.backupPolygons[k]->HasOverlappedEdges(pThisSidePolygon))
                {
                    if (ec.pArgumentModel->GetHeight() > 0 && CheckVirtualKey(Qt::Key_Control))
                    {
                        ec.pModel->AddPolygon(pThisSidePolygon->Clone(), eOpType_Add);
                    }
                    else
                    {
                        ec.pModel->AddPolygon(pThisSidePolygon, eOpType_ExclusiveOR);
                    }
                    bOperated = true;
                    break;
                }
            }
            if (!bOperated)
            {
                if (ec.pArgumentModel->GetHeight() < 0)
                {
                    PolygonPtr pFlipPolygon = sidePolygons[i]->Clone()->Flip();
                    ec.pModel->AddPolygon(pFlipPolygon, eOpType_Add);
                }
                else
                {
                    ec.pModel->AddPolygon(sidePolygons[i]->Clone(), eOpType_Add);
                }
            }
        }
    }

    PolygonPtr pCapPolygon(ec.pArgumentModel->GetCapPolygon());
    if (pCapPolygon)
    {
        if (!ec.bTouchedMirrorPlane || !IsEquivalent(ec.pModel->GetMirrorPlane().Normal(), pCapPolygon->GetPlane().Normal()))
        {
            ec.pModel->AddPolygon(pCapPolygon->Clone(), eOpType_Add);
        }
    }

    if (CD::GetDesigner())
    {
        Matrix34 worldTM = ec.pObject->GetWorldTM();
        worldTM.SetTranslation(worldTM.GetTranslation() + worldTM.TransformVector(ec.pPolygon->GetPlane().Normal() * (ec.pArgumentModel->GetHeight() + 0.01f)));
        CD::GetDesigner()->GetSelectionMesh()->SetWorldTM(worldTM);
    }

    CD::UpdateAll(ec, eUT_Mirror);

    if (ec.bUpdateBrush)
    {
        if (ec.bFirstUpdate)
        {
            CD::UpdateAll(ec, eUT_Mesh);
            ec.bFirstUpdate = false;
        }
        else
        {
            ec.pCompiler->Compile(ec.pObject, ec.pModel, 1);
        }
    }
}

void ExtrudeTool::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    BaseTool::OnEditorNotifyEvent(event);
    switch (event)
    {
    case eNotify_OnEndUndoRedo:
        CD::GetDesigner()->UpdateSelectionMesh(NULL, GetCompiler(), GetBaseObject(), true);
        break;
    }
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Extrude, CD::eToolGroup_Edit, "Extrude", ExtrudeTool)