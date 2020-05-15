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
#include "SliceTool.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "ViewManager.h"
#include "ITransformManipulator.h"
#include "Objects/DesignerObject.h"
#include "Objects/AreaSolidObject.h"

#include <AzCore/Casting/numeric_cast.h>

void SliceTool::Enter()
{
    BaseTool::Enter();
    AABB aabb;
    GetBaseObject()->GetLocalBounds(aabb);
    m_SlicePlane = BrushPlane(BrushVec3(0, 0, 1), -aabb.GetCenter().z);
    m_NumberSlicePlane = 0;
    GenerateLoop(m_SlicePlane, m_MainTraverseLines);
    UpdateGizmo();
    CenterPivot();
}

void SliceTool::Leave()
{
    GetIEditor()->ShowTransformManipulator(false);
    BaseTool::Leave();
}

void SliceTool::Display(DisplayContext& dc)
{
    dc.SetDrawInFrontMode(true);

    DrawOutlines(dc);

    Matrix34 worldTM = dc.GetMatrix();
    Vec3 vScale = worldTM.GetInverted().TransformVector(Vec3(4.0f, 4.0f, 4.0f));
    float fScale = (vScale.x + vScale.y + vScale.z) / 3.0f;

    dc.DepthTestOff();
    dc.DrawArrow(m_CursorPos, m_CursorPos + m_SlicePlane.Normal() * fScale, 2.0f);
    dc.DepthTestOn();

    dc.SetDrawInFrontMode(false);
}

void SliceTool::DrawOutlines(DisplayContext& dc)
{
    float oldLineWidth = dc.GetLineWidth();
    dc.SetLineWidth(3);

    dc.SetColor(ColorB(99, 99, 99, 255));
    for (int i = 0, iSize(m_RestTraverseLineSet.size()); i < iSize; ++i)
    {
        DrawOutline(dc, m_RestTraverseLineSet[i]);
    }

    dc.SetColor(ColorB(50, 200, 50, 255));
    DrawOutline(dc, m_MainTraverseLines);

    dc.SetLineWidth(oldLineWidth);
}

void SliceTool::DrawOutline(DisplayContext& dc, TraverseLineList& lineList)
{
    for (int i = 0, iSize(lineList.size()); i < iSize; ++i)
    {
        dc.DrawLine(lineList[i].m_Edge.m_v[0], lineList[i].m_Edge.m_v[1]);
    }
}

void SliceTool::GenerateLoop(const BrushPlane& SlicePlane, TraverseLineList& outLineList) const
{
    outLineList.clear();

    for (int i = 0, iPolygonSize(GetModel()->GetPolygonCount()); i < iPolygonSize; ++i)
    {
        CD::PolygonPtr pPolygon = GetModel()->GetPolygon(i);
        BrushLine3D intersectedLine;
        if (!SlicePlane.IntersectionLine(pPolygon->GetPlane(), intersectedLine))
        {
            continue;
        }

        std::vector<BrushEdge3D> sortedIntersectedEdges;
        if (!pPolygon->QueryIntersections(SlicePlane, intersectedLine, sortedIntersectedEdges))
        {
            continue;
        }

        for (int i = 0, iEdgeSize(sortedIntersectedEdges.size()); i < iEdgeSize; ++i)
        {
            outLineList.push_back(ETraverseLineInfo(pPolygon, sortedIntersectedEdges[i], SlicePlane));
        }
    }
}

BrushVec3 SliceTool::GetLoopPivotPoint() const
{
    AABB aabb;
    aabb.Reset();
    for (int i = 0, iSize(m_MainTraverseLines.size()); i < iSize; ++i)
    {
        aabb.Add(m_MainTraverseLines[i].m_Edge.m_v[0]);
        aabb.Add(m_MainTraverseLines[i].m_Edge.m_v[1]);
    }
    return aabb.GetCenter();
}

void SliceTool::SliceFrontPart()
{
    _smart_ptr<CD::Model> frontPart;
    _smart_ptr<CD::Model> backPart;

    CUndo undo("Designer : Slice Front");

    if (GetModel()->Clip(m_SlicePlane, frontPart, backPart, true) == CD::eCPR_CLIPFAILED)
    {
        return;
    }

    GetModel()->RecordUndo("Designer : Slice Front", GetBaseObject());

    if (backPart)
    {
        (*GetModel()) = *backPart;
        UpdateRestTraverseLineSet();
        UpdateAll();
    }
}

void SliceTool::SliceBackPart()
{
    _smart_ptr<CD::Model> frontPart;
    _smart_ptr<CD::Model> backPart;

    CUndo undo("Designer : Slice Back");

    if (GetModel()->Clip(m_SlicePlane, frontPart, backPart, true) == CD::eCPR_CLIPFAILED)
    {
        return;
    }

    GetModel()->RecordUndo("Designer : Slice Back", GetBaseObject());

    if (frontPart)
    {
        (*GetModel()) = *frontPart;
        UpdateRestTraverseLineSet();
        UpdateAll();
    }
}

void SliceTool::Divide()
{
    TraverseLineLists traverseLineListSet;
    if (m_RestTraverseLineSet.empty())
    {
        traverseLineListSet.push_back(m_MainTraverseLines);
    }
    else
    {
        traverseLineListSet = m_RestTraverseLineSet;
    }

    CUndo undo("Designer : Divide");
    GetModel()->RecordUndo("Designer : Divide", GetBaseObject());

    for (int i = 0, iSize(traverseLineListSet.size()); i < iSize; ++i)
    {
        const TraverseLineList& lineList = traverseLineListSet[i];
        for (int k = 0, iLineListSize(lineList.size()); k < iLineListSize; ++k)
        {
            CD::PolygonPtr pSlicePolygon = new CD::Polygon;
            pSlicePolygon->SetPlane(lineList[k].m_pPolygon->GetPlane());
            pSlicePolygon->AddEdge(lineList[k].m_Edge);
            GetModel()->AddOpenPolygon(pSlicePolygon, false);
            GetModel()->RemovePolygon(lineList[k].m_pPolygon);
        }
    }
    UpdateAll();
}

void SliceTool::Clip()
{
    _smart_ptr<CD::Model> frontPart;
    _smart_ptr<CD::Model> backPart;

    CUndo undo("Designer : Slip");

    if (GetModel()->Clip(m_SlicePlane, frontPart, backPart, true) == CD::eCPR_CLIPFAILED)
    {
        return;
    }

    GetModel()->RecordUndo("Designer : Slip", GetBaseObject());

    if (!frontPart || !backPart)
    {
        if (frontPart)
        {
            (*GetModel()) = *frontPart;
        }
        else if (backPart)
        {
            (*GetModel()) = *backPart;
        }
    }
    else
    {
        (*GetModel()) = *frontPart;

        CBaseObject* pClonedObject = GetIEditor()->GetObjectManager()->CloneObject(GetBaseObject());
        if (qobject_cast<DesignerObject*>(pClonedObject))
        {
            DesignerObject* pBackObj = ((DesignerObject*)pClonedObject);
            pBackObj->SetModel(backPart);
            pBackObj->GetCompiler()->Compile(pBackObj, pBackObj->GetModel());
        }
        else if (qobject_cast<AreaSolidObject*>(pClonedObject))
        {
            AreaSolidObject* pBackObj = ((AreaSolidObject*)pClonedObject);
            pBackObj->SetModel(backPart);
            pBackObj->GetCompiler()->Compile(pBackObj, pBackObj->GetModel());
        }
        else
        {
            DESIGNER_ASSERT(0);
        }
    }

    UpdateRestTraverseLineSet();
    UpdateAll();
}

void SliceTool::UpdateSlicePlane()
{
    GenerateLoop(m_SlicePlane, m_MainTraverseLines);
    UpdateRestTraverseLineSet();
}

void SliceTool::SetNumberSlicePlane(int numberSlicePlane)
{
    if (numberSlicePlane < 0 || numberSlicePlane > 100)
    {
        return;
    }

    m_NumberSlicePlane = numberSlicePlane;
    UpdateRestTraverseLineSet();
}

void SliceTool::UpdateRestTraverseLineSet()
{
    m_RestTraverseLineSet.clear();

    if (m_NumberSlicePlane <= 0)
    {
        return;
    }

    AABB localBoundbox;
    GetBaseObject()->GetLocalBounds(localBoundbox);

    Vec3 apexes[] = {
        Vec3(localBoundbox.min.x, localBoundbox.min.y, localBoundbox.min.z),
        Vec3(localBoundbox.max.x, localBoundbox.min.y, localBoundbox.min.z),
        Vec3(localBoundbox.min.x, localBoundbox.max.y, localBoundbox.min.z),
        Vec3(localBoundbox.max.x, localBoundbox.max.y, localBoundbox.min.z),
        Vec3(localBoundbox.min.x, localBoundbox.min.y, localBoundbox.max.z),
        Vec3(localBoundbox.max.x, localBoundbox.min.y, localBoundbox.max.z),
        Vec3(localBoundbox.min.x, localBoundbox.max.y, localBoundbox.max.z),
        Vec3(localBoundbox.max.x, localBoundbox.max.y, localBoundbox.max.z)
    };

    int nFarthestFrontApex = -1;
    float fFarthestFrontDistance = -1;
    int nFarthestBackApex = -1;
    float fFarthestBackDistance = -1;

    BrushPlane planeWithZeroDistance(m_SlicePlane.Normal(), 0);
    for (int i = 0, iSize(sizeof(apexes) / sizeof(apexes[0])); i < iSize; ++i)
    {
        float fDistance = aznumeric_cast<float>(planeWithZeroDistance.Distance(apexes[i]));
        if (fDistance >= 0)
        {
            if (fDistance > fFarthestFrontDistance)
            {
                fFarthestFrontDistance = fDistance;
                nFarthestFrontApex = i;
            }
        }
        else if (fDistance < 0)
        {
            if (-fDistance > -fFarthestBackDistance)
            {
                fFarthestBackDistance = fDistance;
                nFarthestBackApex = i;
            }
        }
    }

    if (nFarthestFrontApex == -1 || nFarthestBackApex == -1)
    {
        return;
    }

    int nRealNumSlicePlane = m_NumberSlicePlane + 2;

    m_RestTraverseLineSet.resize(nRealNumSlicePlane - 2);
    float fGap = (fFarthestFrontDistance - fFarthestBackDistance) / (nRealNumSlicePlane - 1);

    for (int i = 1; i < nRealNumSlicePlane - 1; ++i)
    {
        BrushPlane plane(m_SlicePlane.Normal(), fFarthestBackDistance + fGap * i);
        GenerateLoop(plane, m_RestTraverseLineSet[i - 1]);
    }
}

void SliceTool::AlignSlicePlane(const BrushVec3& normal)
{
    BrushPlane plane(normal, 0);
    m_SlicePlane = BrushPlane(normal, -plane.Distance(m_CursorPos));
    GenerateLoop(m_SlicePlane, m_MainTraverseLines);
    UpdateRestTraverseLineSet();
}

void SliceTool::InvertSlicePlane()
{
    m_SlicePlane.Invert();
    GenerateLoop(m_SlicePlane, m_MainTraverseLines);
    UpdateRestTraverseLineSet();
}

void SliceTool::OnManipulatorDrag(CViewport* pView, ITransformManipulator* pManipulator, QPoint& p0, QPoint& p1, const BrushVec3& value)
{
    if (GetIEditor()->GetEditMode() == eEditModeScale)
    {
        return;
    }

    BrushVec3 vDelta = value - m_PrevGizmoPos;
    if (CD::IsEquivalent(vDelta, BrushVec3(0, 0, 0)))
    {
        return;
    }

    BrushMatrix34 offsetTM = CD::GetOffsetTM(pManipulator, vDelta, GetWorldTM());
    bool bUpdatedManipulator = false;
    bool bDesignerMirrorMode = GetModel()->CheckModeFlag(CD::eDesignerMode_Mirror);

    if (bDesignerMirrorMode)
    {
        bUpdatedManipulator = UpdateManipulatorInMirrorMode(offsetTM);
    }

    if (!bUpdatedManipulator)
    {
        if (GetIEditor()->GetEditMode() == eEditModeMove)
        {
            m_GizmoPos += vDelta;
            m_CursorPos = m_GizmoPos;
            AlignSlicePlane(m_SlicePlane.Normal());
        }
        else if (!bDesignerMirrorMode && GetIEditor()->GetEditMode() == eEditModeRotate)
        {
            AlignSlicePlane(offsetTM.TransformVector(m_SlicePlane.Normal()));
        }
    }

    CD::GetDesigner()->UpdateTMManipulator(m_GizmoPos, BrushVec3(0, 0, 1));
    m_PrevGizmoPos = value;
}

void SliceTool::OnManipulatorMouseEvent(CViewport* pView, ITransformManipulator* pManipulator, EMouseEvent event, QPoint& point, int flags, bool bHitGizmo)
{
    if (event == eMouseLDown)
    {
        m_PrevGizmoPos = BrushVec3(0, 0, 0);
    }
}

void SliceTool::UpdateGizmo()
{
    m_GizmoPos = m_CursorPos;
    CD::GetDesigner()->UpdateTMManipulator(m_GizmoPos, BrushVec3(0, 0, 1));
}

void SliceTool::CenterPivot()
{
    AABB bbox;
    if (!GetBaseObject())
    {
        return;
    }
    GetBaseObject()->GetLocalBounds(bbox);
    BrushVec3 vCenter = bbox.GetCenter();
    m_CursorPos = vCenter;
    AlignSlicePlane(m_SlicePlane.Normal());
    UpdateGizmo();
}

#include "UIs/SlicePanel.h"
REGISTER_DESIGNER_TOOL_WITH_PANEL(CD::eDesigner_Slice, CD::eToolGroup_Modify, "Slice", SliceTool, SlicePanel)
