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
#include "MagnetTool.h"
#include "Core/Model.h"
#include "Tools/DesignerTool.h"
#include "Viewport.h"


MagnetTool::MagnetTool(CD::EDesignerTool tool)
    : SelectTool(tool)
    , m_Phase(eMTP_ChooseFirstPoint)
    , m_nSelectedSourceVertex(-1)
    , m_nSelectedUpVertex(-1)
    , m_bPickedTargetPos(false)
    , m_bSwitchedSides(false)
{
}

void MagnetTool::AddVertexToList(const BrushVec3& vertex, ColorB color, std::vector<SSourceVertex>& vertices)
{
    bool bHasSame = false;
    for (int i = 0, iVertexCount(vertices.size()); i < iVertexCount; ++i)
    {
        if (CD::IsEquivalent(vertices[i].position, vertex))
        {
            bHasSame = true;
            break;
        }
    }
    if (!bHasSame)
    {
        vertices.push_back(SSourceVertex(vertex, color));
    }
}

void MagnetTool::Enter()
{
    SelectTool::Enter();

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->GetCount() != 1)
    {
        CD::MessageBox("Warning", "only one face should be selected to use this tool");
        CD::GetDesigner()->SwitchToPrevTool();
        return;
    }

    ElementManager copiedSelected(*pSelected);
    for (int i = 0, iCount(pSelected->GetCount()); i < iCount; ++i)
    {
        if (!(*pSelected)[i].IsFace() || (*pSelected)[i].m_pPolygon == NULL)
        {
            copiedSelected.Erase((*pSelected)[i]);
        }
    }

    pSelected->Set(copiedSelected);
    PrepareChooseFirstPointStep();
    m_PickedPos = BrushVec3(0, 0, 0);
    m_vTargetUpDir = BrushVec3(0, 0, 1);
}

void MagnetTool::Leave()
{
    if (m_Phase == eMTP_ChooseMoveToTargetPoint)
    {
        GetIEditor()->AcceptUndo("Designer : Magnet Tool");
        GetModel()->MoveShelf(1, 0);
        UpdateAll();
    }

    m_Phase = eMTP_ChooseFirstPoint;
    m_SourceVertices.clear();
    m_nSelectedSourceVertex = -1;
    m_nSelectedUpVertex = -1;
    m_pInitPolygon = NULL;

    SelectTool::Leave();
}

void MagnetTool::PrepareChooseFirstPointStep()
{
    m_nSelectedSourceVertex = -1;
    m_nSelectedUpVertex = -1;
    m_Phase = eMTP_ChooseFirstPoint;

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty() || (*pSelected)[0].m_pPolygon == NULL)
    {
        return;
    }

    m_SourceVertices.clear();

    const BrushPlane& plane = (*pSelected)[0].m_pPolygon->GetPlane();
    for (int k = 0, iSelectedCount(pSelected->GetCount()); k < iSelectedCount; ++k)
    {
        const AABB& aabb((*pSelected)[k].m_pPolygon->GetBoundBox());

        AABB rectangleAABB;
        rectangleAABB.Reset();

        BrushVec2 v0 = plane.W2P(CD::ToBrushVec3(aabb.min));
        BrushVec2 v1 = plane.W2P(CD::ToBrushVec3(aabb.max));

        rectangleAABB.Add(Vec3(aznumeric_cast<float>(v0.x), 0, aznumeric_cast<float>(v0.y)));
        rectangleAABB.Add(Vec3(aznumeric_cast<float>(v1.x), 0, aznumeric_cast<float>(v1.y)));

        BrushVec3 step = CD::ToBrushVec3((rectangleAABB.max - rectangleAABB.min) * 0.5f);

        for (int i = 0; i <= 2; ++i)
        {
            for (int j = 0; j <= 2; ++j)
            {
                for (int k = 0; k <= 2; ++k)
                {
                    BrushVec3 v = CD::ToBrushVec3(rectangleAABB.min) + BrushVec3(i * step.x, j * step.y, k * step.z);
                    v = plane.P2W(BrushVec2(v.x, v.z));
                    AddVertexToList(v, ColorB(0xFF40FF40), m_SourceVertices);
                }
            }
        }

        (*pSelected)[k].m_bIsolated = true;
        for (int i = 0, iVertexCount((*pSelected)[k].m_pPolygon->GetVertexCount()); i < iVertexCount; ++i)
        {
            AddVertexToList((*pSelected)[k].m_pPolygon->GetPos(i), CD::kElementBoxColor, m_SourceVertices);
        }

        for (int i = 0, iEdgeCount((*pSelected)[k].m_pPolygon->GetEdgeCount()); i < iEdgeCount; ++i)
        {
            BrushEdge3D e = (*pSelected)[k].m_pPolygon->GetEdge(i);
            AddVertexToList(e.GetCenter(), CD::kElementBoxColor, m_SourceVertices);
        }

        AddVertexToList((*pSelected)[k].m_pPolygon->GetRepresentativePosition(), CD::kElementBoxColor, m_SourceVertices);
    }
}

void MagnetTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    if (m_Phase == eMTP_ChooseFirstPoint && m_nSelectedSourceVertex != -1)
    {
        m_nSelectedUpVertex = -1;
        m_Phase = eMTP_ChooseUpPoint;
        m_PickedPos = m_SourceVertices[m_nSelectedSourceVertex].position;
    }
    else if (m_Phase == eMTP_ChooseUpPoint)
    {
        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
        GetIEditor()->BeginUndo();
        GetModel()->RecordUndo("Designer : Magnet Tool", GetBaseObject());
        m_pInitPolygon = NULL;
        m_Phase = eMTP_ChooseMoveToTargetPoint;
        MODEL_SHELF_RECONSTRUCTOR(GetModel());
        m_pInitPolygon = (*pSelected)[0].m_pPolygon->Clone();
        GetModel()->SetShelf(0);
        GetModel()->RemovePolygon((*pSelected)[0].m_pPolygon);
        GetModel()->SetShelf(1);
        GetModel()->AddPolygon((*pSelected)[0].m_pPolygon, CD::eOpType_Add);
        UpdateAll(CD::eUT_Mesh);
        m_bPickedTargetPos = false;
        m_bSwitchedSides = false;
    }
    else if (m_Phase == eMTP_ChooseMoveToTargetPoint)
    {
        CD::GetDesigner()->SwitchToPrevTool();
    }
}

void MagnetTool::SwitchSides()
{
    DESIGNER_ASSERT(m_pInitPolygon);
    if (!m_pInitPolygon)
    {
        return;
    }
    BrushVec3 vPivot = m_SourceVertices[m_nSelectedSourceVertex].position;
    BrushVec3 vUpPos = m_SourceVertices[m_nSelectedUpVertex].position;
    BrushPlane mirrorPlane(vPivot, vUpPos, m_pInitPolygon->GetPlane().Normal() + vPivot);
    m_pInitPolygon->Mirror(mirrorPlane);
    InitializeSelectedPolygonBeforeTransform();

    if (m_bPickedTargetPos && !CheckVirtualKey(Qt::Key_Shift))
    {
        AlignSelectedPolygon();
    }
    else
    {
        CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
        UpdateShelf(1);
    }

    m_bSwitchedSides = !m_bSwitchedSides;
}

void MagnetTool::AlignSelectedPolygon()
{
    std::vector<BrushEdge3D> edges;
    bool bEdgeExist = false;
    BrushEdge3D dirEdge;
    if (GetModel()->QueryEdgesHavingVertex(m_TargetPos, edges))
    {
        int iEdgeCount(edges.size());
        for (int i = 0; i < iEdgeCount; ++i)
        {
            if (CD::IsEquivalent(edges[i].m_v[0], m_TargetPos))
            {
                dirEdge = edges[i];
                bEdgeExist = true;
                break;
            }
            else if (CD::IsEquivalent(edges[i].m_v[1], m_TargetPos))
            {
                dirEdge = edges[i].GetInverted();
                bEdgeExist = true;
                break;
            }
        }
    }
    if (bEdgeExist)
    {
        InitializeSelectedPolygonBeforeTransform();

        DESIGNER_ASSERT(m_pInitPolygon);
        if (!m_pInitPolygon)
        {
            return;
        }

        BrushVec3 vSelectionNormal = m_pInitPolygon->GetPlane().Normal();
        BrushVec3 vEdgeDir = (dirEdge.m_v[1] - dirEdge.m_v[0]).GetNormalized();
        BrushMatrix34 tmRot1 = CD::ToBrushMatrix33(Matrix33::CreateRotationV0V1(vSelectionNormal, -vEdgeDir));
        BrushMatrix34 tmRot2 = BrushMatrix34::CreateIdentity();
        if (m_nSelectedUpVertex != -1)
        {
            BrushVec3 vUpDir = (m_SourceVertices[m_nSelectedUpVertex].position - m_SourceVertices[m_nSelectedSourceVertex].position).GetNormalized();
            vUpDir = tmRot1.TransformVector(vUpDir);
            BrushVec3 vIntermediateDir = vEdgeDir.Cross(vUpDir);
            tmRot2 = CD::ToBrushMatrix33(Matrix33::CreateRotationV0V1(vIntermediateDir, m_vTargetUpDir) * Matrix33::CreateRotationV0V1(vUpDir, vIntermediateDir));
        }
        BrushMatrix34 tmRot = tmRot2 * tmRot1;
        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
        (*pSelected)[0].m_pPolygon->Move(-m_TargetPos);
        (*pSelected)[0].m_pPolygon->Transform(tmRot);
        (*pSelected)[0].m_pPolygon->Move(m_TargetPos);

        CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
        UpdateShelf(1);
    }
}

bool MagnetTool::OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        if (m_Phase == eMTP_ChooseMoveToTargetPoint || m_Phase == eMTP_ChooseUpPoint)
        {
            ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
            if (m_bSwitchedSides)
            {
                SwitchSides();
            }

            if (m_pInitPolygon)
            {
                *((*pSelected)[0].m_pPolygon) = *m_pInitPolygon;
            }

            GetModel()->MoveShelf(1, 0);
            UpdateAll(CD::eUT_Mesh);
            GetIEditor()->CancelUndo();
            PrepareChooseFirstPointStep();
            CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
        }
        else if (m_Phase == eMTP_ChooseFirstPoint)
        {
            return CD::GetDesigner()->EndCurrentEditSession();
        }
    }
    else if (nChar == VK_CONTROL && m_Phase == eMTP_ChooseMoveToTargetPoint)
    {
        SwitchSides();
    }
    return true;
}

void MagnetTool::OnMouseMove(CViewport* view, UINT nFlags, const QPoint& point)
{
    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);

    Vec3 raySrc, rayDir;
    view->ViewToWorldRay(point, raySrc, rayDir);

    if (m_Phase == eMTP_ChooseFirstPoint || m_Phase == eMTP_ChooseUpPoint)
    {
        BrushFloat fLeastDist = (BrushFloat)3e10;

        int nSelectedVertex = -1;
        for (int i = 0, iVertexCount(m_SourceVertices.size()); i < iVertexCount; ++i)
        {
            const BrushVec3& v(m_SourceVertices[i].position);
            BrushFloat dist;
            BrushVec3 vWorldPos = GetWorldTM().TransformPoint(v);
            BrushVec3 vBoxSize = CD::GetElementBoxSize(view, view->GetType() != ET_ViewportCamera, vWorldPos);
            if (!CD::GetIntersectionOfRayAndAABB(CD::ToBrushVec3(raySrc), CD::ToBrushVec3(rayDir), AABB(vWorldPos - vBoxSize, vWorldPos + vBoxSize), &dist))
            {
                continue;
            }

            if (dist > 0 && dist < fLeastDist)
            {
                fLeastDist = dist;
                nSelectedVertex = i;
            }
        }

        if (nSelectedVertex != -1)
        {
            if (m_Phase == eMTP_ChooseFirstPoint)
            {
                m_nSelectedSourceVertex = nSelectedVertex;
            }
            else
            {
                if (nSelectedVertex != m_nSelectedSourceVertex)
                {
                    m_nSelectedUpVertex = nSelectedVertex;
                }
                else
                {
                    m_nSelectedUpVertex = -1;
                }
                m_PickedPos = m_SourceVertices[nSelectedVertex].position;
            }
        }
        else
        {
            GetModel()->QueryPosition(localRaySrc, localRayDir, m_PickedPos);
        }
    }
    else
    {
        BrushVec3 vPickedPos;
        m_bPickedTargetPos = !(nFlags & MK_SHIFT) ? m_bPickedTargetPos : false;
        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
        if (pSelected->QueryNearestVertex(GetBaseObject(), GetModel(), view, point, localRaySrc, localRayDir, vPickedPos, &m_vTargetUpDir))
        {
            m_TargetPos = vPickedPos;
            InitializeSelectedPolygonBeforeTransform();
            CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(GetMainContext());
            UpdateShelf(1);

            m_bPickedTargetPos = true;
        }
        if (m_bPickedTargetPos && !(nFlags & MK_SHIFT))
        {
            AlignSelectedPolygon();
        }
    }
}

void MagnetTool::InitializeSelectedPolygonBeforeTransform()
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    const BrushVec3& vInitPivot = m_SourceVertices[m_nSelectedSourceVertex].position;
    BrushMatrix34 tmDelta = BrushMatrix34::CreateIdentity();
    tmDelta.SetTranslation(m_TargetPos - vInitPivot);

    DESIGNER_ASSERT(m_pInitPolygon);
    if (!m_pInitPolygon)
    {
        return;
    }

    *((*pSelected)[0].m_pPolygon) = *m_pInitPolygon;
    (*pSelected)[0].m_pPolygon->Transform(tmDelta);
}

void MagnetTool::Display(DisplayContext& dc)
{
    dc.PopMatrix();
    if (m_Phase == eMTP_ChooseFirstPoint || m_Phase == eMTP_ChooseUpPoint)
    {
        for (int i = 0, iVertexCount(m_SourceVertices.size()); i < iVertexCount; ++i)
        {
            dc.SetColor(m_SourceVertices[i].color);
            const BrushVec3& v = m_SourceVertices[i].position;
            if (m_nSelectedSourceVertex == i || m_nSelectedUpVertex == i)
            {
                continue;
            }
            BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(v);
            BrushVec3 vVertexBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, vWorldVertexPos);
            dc.DrawSolidBox(CD::ToVec3(vWorldVertexPos - vVertexBoxSize), CD::ToVec3(vWorldVertexPos + vVertexBoxSize));
        }

        if (m_nSelectedSourceVertex != -1)
        {
            BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_SourceVertices[m_nSelectedSourceVertex].position);
            BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, vWorldVertexPos);
            dc.SetColor(CD::kSelectedColor);
            dc.DrawSolidBox(CD::ToVec3(vWorldVertexPos - vBoxSize), CD::ToVec3(vWorldVertexPos + vBoxSize));

            if (m_Phase == eMTP_ChooseUpPoint)
            {
                dc.SetColor(ColorB(0, 150, 214, 255));
                dc.SetLineWidth(4);
                dc.DrawLine(GetWorldTM().TransformPoint(m_SourceVertices[m_nSelectedSourceVertex].position), GetWorldTM().TransformPoint(m_PickedPos));
            }
        }
        if (m_nSelectedUpVertex != -1)
        {
            BrushVec3 vWorldVertexPos = GetWorldTM().TransformPoint(m_SourceVertices[m_nSelectedUpVertex].position);
            BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, vWorldVertexPos);
            dc.SetColor(ColorB(0, 150, 214, 255));
            dc.DrawSolidBox(CD::ToVec3(vWorldVertexPos - vBoxSize), CD::ToVec3(vWorldVertexPos + vBoxSize));
        }
    }
    else if (m_Phase == eMTP_ChooseMoveToTargetPoint)
    {
        std::vector<BrushVec3> excludedVertices;
        if (m_bPickedTargetPos)
        {
            excludedVertices.push_back(m_TargetPos);
            dc.SetColor(CD::kSelectedColor);
            BrushVec3 vWorldPos = GetWorldTM().TransformPoint(m_TargetPos);
            BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, vWorldPos);
            dc.DrawSolidBox(CD::ToVec3(vWorldPos - vBoxSize), CD::ToVec3(vWorldPos + vBoxSize));
        }

        dc.PushMatrix(GetWorldTM());
        ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
        pSelected->DisplayVertexElements(GetBaseObject(), GetModel(), dc, 0, &excludedVertices);
        dc.PopMatrix();
    }
    dc.PushMatrix(GetWorldTM());
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Magnet, CD::eToolGroup_Modify, "Magnet", MagnetTool)
