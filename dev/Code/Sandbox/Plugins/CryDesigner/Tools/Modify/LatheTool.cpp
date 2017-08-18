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
#include "LatheTool.h"
#include "Tools/DesignerTool.h"
#include "Tools/Select/SelectTool.h"
#include "Viewport.h"
#include "Core/BrushHelper.h"

using namespace CD;

void LatheTool::Enter()
{
    BaseTool::Enter();
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        CD::GetDesigner()->SwitchToPrevTool();
    }
}

void LatheTool::OnLButtonDown(CViewport* view, UINT nFlags, const QPoint& point)
{
    BrushVec3 localRaySrc, localRayDir;
    GetLocalViewRay(GetBaseObject()->GetWorldTM(), view, point, localRaySrc, localRayDir);
    int nPickedPolygon(-1);
    BrushVec3 outPos;
    if (!GetModel()->QueryPolygon(localRaySrc, localRayDir, nPickedPolygon))
    {
        return;
    }

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->GetCount() == 1 && (*pSelected)[0].m_pPolygon)
    {
        m_pPathPolygon = (*pSelected)[0].m_pPolygon;
    }
    else
    {
        m_pPathPolygon = NULL;
    }

    ELatheErrorCode errorCode = CreateShapeAlongPath(GetModel()->GetPolygon(nPickedPolygon));
    if (errorCode == eLEC_Success)
    {
        return;
    }

    switch (errorCode)
    {
    case eLEC_NoPath:
        CD::MessageBox("Warning", "Edges or a face have to be selected to be used as a path.");
        break;
    case eLEC_InappropriateProfileShape:
        CD::MessageBox("Warning", "The profile face is not appropriate.");
        break;
    case eLEC_ProfileShapeTooBig:
        CD::MessageBox("Warning", "The profile face is too big or located at a wrong position. You should reduce the width of the profile face or move it.");
        break;
    }

    CD::GetDesigner()->SwitchToPrevTool();
}

std::vector<CD::SVertex> LatheTool::ExtractPathFromSelectedElements(bool& bOutClosed)
{
    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    std::vector<CD::SVertex> vPath;
    bOutClosed = true;
    if (pSelected->GetCount() != 1 || !(*pSelected)[0].IsFace())
    {
        int iSelectedElementCount(pSelected->GetCount());
        for (int i = 0; i < iSelectedElementCount; ++i)
        {
            if (!(*pSelected)[i].IsEdge())
            {
                continue;
            }
            BrushEdge3D edge = (*pSelected)[i].GetEdge();
            vPath.push_back(SVertex(edge.m_v[0]));
        }

        BrushEdge3D firstEdge = (*pSelected)[0].GetEdge();
        BrushEdge3D lastEdge = (*pSelected)[iSelectedElementCount - 1].GetEdge();
        bOutClosed = iSelectedElementCount >= 3 && (IsEquivalent(firstEdge.m_v[0], lastEdge.m_v[1]) || IsEquivalent(firstEdge.m_v[1], lastEdge.m_v[0]));
        if (!bOutClosed)
        {
            vPath.push_back(SVertex(lastEdge.m_v[1]));
        }
    }
    else
    {
        if ((*pSelected)[0].m_pPolygon->IsOpen())
        {
            (*pSelected)[0].m_pPolygon->GetLinkedVertices(vPath);
            bOutClosed = false;
        }
        else
        {
            std::vector<PolygonPtr> outSeparatedPolygons;
            (*pSelected)[0].m_pPolygon->GetSeparatedPolygons(outSeparatedPolygons, eSP_OuterHull);
            if (outSeparatedPolygons.size() == 1)
            {
                outSeparatedPolygons[0]->GetLinkedVertices(vPath);
            }
        }
    }

    return vPath;
}

std::vector<BrushPlane> LatheTool::CreatePlanesAtEachPointOfPath(const std::vector<CD::SVertex>& vPath, bool bPathClosed)
{
    std::vector<BrushPlane> planeAtEveryIntersection;
    int iPathCount = vPath.size();
    planeAtEveryIntersection.resize(iPathCount);

    int nPathCount = bPathClosed ? iPathCount : iPathCount - 2;
    for (int i = 0; i < nPathCount; ++i)
    {
        const BrushVec3& vCommon = vPath[(i + 1) % iPathCount].pos;
        BrushVec3 vDir = (vCommon - vPath[i].pos).GetNormalized();
        BrushVec3 vDirNext = (vPath[(i + 2) % iPathCount].pos - vCommon).GetNormalized();
        planeAtEveryIntersection[(i + 1) % iPathCount] = BrushPlane(vCommon, vCommon - vDir.Cross(vDirNext), vCommon - vDir + vDirNext);
    }

    if (!bPathClosed)
    {
        BrushVec3 vDir = (vPath[0].pos - vPath[1].pos).GetNormalized();
        planeAtEveryIntersection[0] = BrushPlane(vDir, -vDir.Dot(vPath[0].pos));
        vDir = (vPath[iPathCount - 1].pos - vPath[iPathCount - 2].pos).GetNormalized();
        planeAtEveryIntersection[iPathCount - 1] = BrushPlane(vDir, -vDir.Dot(vPath[iPathCount - 1].pos));
    }

    return planeAtEveryIntersection;
}

void LatheTool::AddPolygonToDesigner(Model* pModel, const std::vector<BrushVec3>& vList, PolygonPtr pInitPolygon, bool bFlip)
{
    PolygonPtr pPolygon = pInitPolygon->Clone();

    for (int i = 0, iVertexCount(vList.size()); i < iVertexCount; ++i)
    {
        pPolygon->SetPos(i, vList[i]);
    }

    BrushPlane plane;
    if (pPolygon->GetComputedPlane(plane))
    {
        pPolygon->SetPlane(plane);
        if (bFlip)
        {
            pPolygon->Flip();
        }
        pPolygon->SetMaterialID(CD::GetDesigner()->GetCurrentSubMatID());
        GetModel()->AddPolygonUnconditionally(pPolygon);
    }
}

bool LatheTool::GluePolygons(const std::vector<PolygonPtr>& polygons)
{
    for (int i = 0, iPolygonCount(polygons.size()); i < iPolygonCount; ++i)
    {
        BrushPlane invertedPlane = polygons[i]->GetPlane().GetInverted();

        std::vector<PolygonPtr> candidatedPolygons;
        if (!GetModel()->QueryPolygons(invertedPlane, candidatedPolygons) || candidatedPolygons.empty())
        {
            continue;
        }

        PolygonPtr pFlipedPolygon = polygons[i]->Clone()->Flip();
        bool bSubtracted = false;
        std::vector<PolygonPtr> intersectedPolygons;
        for (int k = 0, iCandidateCount(candidatedPolygons.size()); k < iCandidateCount; ++k)
        {
            if (Polygon::HasIntersection(candidatedPolygons[k], pFlipedPolygon) == eIT_Intersection)
            {
                intersectedPolygons.push_back(candidatedPolygons[k]);
            }
        }

        if (intersectedPolygons.empty())
        {
            continue;
        }

        for (int k = 0, iIntersectCount(intersectedPolygons.size()); k < iIntersectCount; ++k)
        {
            if (m_pPathPolygon == intersectedPolygons[k])
            {
                continue;
            }
            bSubtracted = true;
            intersectedPolygons[k]->Subtract(pFlipedPolygon);
        }

        if (bSubtracted)
        {
            GetModel()->RemovePolygon(polygons[i]);
            return true;
        }
    }
    return false;
}

LatheTool::ELatheErrorCode LatheTool::CreateShapeAlongPath(PolygonPtr pInitProfilePolygon)
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    ElementManager* pSelected = CD::GetDesigner()->GetSelectedElements();
    if (pSelected->IsEmpty())
    {
        CD::GetDesigner()->SwitchToPrevTool();
        return eLEC_NoPath;
    }

    bool bClosed = true;
    std::vector<CD::SVertex> vPath = ExtractPathFromSelectedElements(bClosed);
    if (vPath.empty())
    {
        CD::GetDesigner()->SwitchToPrevTool();
        return eLEC_NoPath;
    }

    std::vector<BrushPlane> planeAtEveryIntersection = CreatePlanesAtEachPointOfPath(vPath, bClosed);

    int iPathCount = vPath.size();
    int nStartIndex = 0;
    BrushFloat fNearestDist = (BrushFloat)3e10;
    BrushVec3 vProfilePos = pInitProfilePolygon->GetRepresentativePosition();
    for (int i = 0; i < iPathCount; ++i)
    {
        BrushFloat fDist = vProfilePos.GetDistance(vPath[i].pos);
        if (fDist < fNearestDist)
        {
            fNearestDist = fDist;
            nStartIndex = i;
        }
    }

    CUndo undo("Designer : Lathe Tool");
    GetModel()->RecordUndo("Designer : Lathe Tool", GetBaseObject());

    int nVertexCount = pInitProfilePolygon->GetVertexCount();
    int iEdgeCount = pInitProfilePolygon->GetEdgeCount();

    std::vector<BrushVec3> prevVertices(nVertexCount);
    std::vector<BrushPlane> prevPlanes(iEdgeCount);

    BrushVec3 vEdgeDir = (vPath[(nStartIndex + 1) % iPathCount].pos - vPath[nStartIndex].pos).GetNormalized();
    for (int k = 0; k < nVertexCount; ++k)
    {
        const BrushVec3& v = pInitProfilePolygon->GetPos(k);

        bool bHitTest0 = planeAtEveryIntersection[nStartIndex].HitTest(v, v + vEdgeDir, NULL, &prevVertices[k]);
        DESIGNER_ASSERT(bHitTest0);
        if (!bHitTest0)
        {
            return eLEC_InappropriateProfileShape;
        }
    }

    for (int k = 0; k < iEdgeCount; ++k)
    {
        prevPlanes[k] = BrushPlane(BrushVec3(0, 0, 0), 0);
    }

    GetModel()->SetShelf(1);

    if (!bClosed)
    {
        AddPolygonToDesigner(GetModel(), prevVertices, pInitProfilePolygon, false);
    }

    int nPathCount = bClosed ? iPathCount : iPathCount - 1;
    for (int i = 0; i < nPathCount; ++i)
    {
        int nPathIndex = (nStartIndex + i) % iPathCount;
        int nNextPathIndex = (nPathIndex + 1) % iPathCount;

        const BrushPlane& planeNext = planeAtEveryIntersection[nNextPathIndex];
        vEdgeDir = (vPath[nNextPathIndex].pos - vPath[nPathIndex].pos).GetNormalized();

        std::vector<BrushVec3> vertices(nVertexCount);
        for (int k = 0; k < nVertexCount; ++k)
        {
            if (planeNext.Distance(prevVertices[k]) > kDesignerEpsilon)
            {
                GetModel()->Clear();
                UpdateAll(eUT_Mesh);
                undo.Cancel();
                return eLEC_ProfileShapeTooBig;
            }

            bool bHitTest0 = planeNext.HitTest(prevVertices[k], prevVertices[k] + vEdgeDir,  NULL, &vertices[k]);
            DESIGNER_ASSERT(bHitTest0);
            if (!bHitTest0)
            {
                continue;
            }
        }

        for (int k = 0; k < iEdgeCount; ++k)
        {
            const SEdge& e = pInitProfilePolygon->GetEdgeIndexPair(k);

            std::vector<BrushVec3> vList(4);

            vList[0] = prevVertices[e.m_i[1]];
            vList[1] = prevVertices[e.m_i[0]];
            vList[2] = vertices[e.m_i[0]];
            vList[3] = vertices[e.m_i[1]];

            PolygonPtr pSidePolygon = new CD::Polygon(vList);

            if (prevPlanes[k].Normal().IsZero())
            {
                prevPlanes[k] = pSidePolygon->GetPlane();
            }
            else
            {
                BrushFloat d0 = std::abs(prevPlanes[k].Distance(vPath[nPathIndex].pos));
                BrushFloat d1 = std::abs(prevPlanes[k].Distance(vPath[nNextPathIndex].pos));
                if (std::abs(d0 - d1) < kDesignerEpsilon && pSidePolygon->GetPlane().IsSameFacing(prevPlanes[k]))
                {
                    pSidePolygon->SetPlane(prevPlanes[k]);
                }
            }

            AddPolygonWithSubMatID(pSidePolygon);
        }

        prevVertices = vertices;
    }

    if (!bClosed)
    {
        AddPolygonToDesigner(GetModel(), prevVertices, pInitProfilePolygon, true);
    }

    std::vector<PolygonPtr> newPolygons;
    for (int i = 0, iPolygonCount(GetModel()->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        newPolygons.push_back(GetModel()->GetPolygon(i));
    }

    GetModel()->SetShelf(0);
    GetModel()->RemovePolygon(pInitProfilePolygon);

    GetModel()->MoveShelf(1, 0);

    bool bSubtractedFloor = GluePolygons(newPolygons);

    for (int i = 0, iSelectedElementCount(pSelected->GetCount()); i < iSelectedElementCount; ++i)
    {
        if ((*pSelected)[i].IsEdge())
        {
            GetModel()->EraseEdge((*pSelected)[i].GetEdge());
        }
        else if (!bSubtractedFloor && (*pSelected)[i].IsFace())
        {
            GetModel()->RemovePolygon((*pSelected)[i].m_pPolygon);
        }
    }

    pSelected->Clear();
    for (int i = 0, iCount(newPolygons.size()); i < iCount; ++i)
    {
        pSelected->Add(SElement(GetBaseObject(), newPolygons[i]));
    }

    UpdateAll();
    CD::GetDesigner()->SwitchTool(eDesigner_Face);

    return eLEC_Success;
}

REGISTER_DESIGNER_TOOL(CD::eDesigner_Lathe, CD::eToolGroup_Modify, "Lathe", LatheTool)