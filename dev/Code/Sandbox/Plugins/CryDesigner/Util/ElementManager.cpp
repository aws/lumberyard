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
#include "ElementManager.h"
#include "Core/Model.h"
#include "Viewport.h"
#include "Core/BrushHelper.h"

#include <AzCore/Casting/numeric_cast.h>

using namespace CD;

SElement SElement::GetMirroredElement(const BrushPlane& mirrorPlane) const
{
    DESIGNER_ASSERT(!m_Vertices.empty());

    SElement mirroredElement(*this);

    for (int i = 0, iSize(m_Vertices.size()); i < iSize; ++i)
    {
        mirroredElement.m_Vertices[i] = mirrorPlane.MirrorVertex(m_Vertices[i]);
    }

    return mirroredElement;
}

bool SElement::operator == (const SElement& info)
{
    if (m_Vertices.size() != info.m_Vertices.size())
    {
        return false;
    }

    if (IsEdge())
    {
        DESIGNER_ASSERT(m_Vertices.size() == 2 && info.m_Vertices.size() == 2);
        BrushEdge3D edge(m_Vertices[0], m_Vertices[1]);
        BrushEdge3D infoEdge(info.m_Vertices[0], info.m_Vertices[1]);
        if (edge.IsEquivalent(infoEdge) || edge.IsEquivalent(infoEdge.GetInverted()))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    if (IsFace() || IsVertex())
    {
        if (m_Vertices.size() != info.m_Vertices.size() || m_pPolygon != info.m_pPolygon)
        {
            return false;
        }
        for (int i = 0, iSize(m_Vertices.size()); i < iSize; ++i)
        {
            bool bSameExist = false;
            for (int k = 0; k < iSize; ++k)
            {
                int nIndex = (i + k) % iSize;
                if (CD::IsEquivalent(m_Vertices[nIndex], info.m_Vertices[nIndex]))
                {
                    bSameExist = true;
                    break;
                }
            }
            if (!bSameExist)
            {
                return false;
            }
        }
        return true;
    }

    return false;
}

void SElement::Invalidate()
{
    m_Vertices.clear();
    m_pPolygon = NULL;
    m_pObject = NULL;
}

bool SElement::IsEquivalent(const SElement& elementInfo) const
{
    if (m_Vertices.size() != elementInfo.m_Vertices.size())
    {
        return false;
    }

    if (IsFace())
    {
        if (m_pPolygon != NULL && m_pPolygon == elementInfo.m_pPolygon)
        {
            return true;
        }
        return false;
    }

    for (int i = 0, iVertexSize(m_Vertices.size()); i < iVertexSize; ++i)
    {
        bool bFoundEquivalent = false;
        for (int k = 0; k < iVertexSize; ++k)
        {
            if (CD::IsEquivalent(m_Vertices[i], elementInfo.m_Vertices[(k + i) % iVertexSize]))
            {
                bFoundEquivalent = true;
                break;
            }
        }
        if (!bFoundEquivalent)
        {
            return false;
        }
    }

    return true;
}

bool ElementManager::Add(ElementManager& elements)
{
    if (elements.IsEmpty())
    {
        return false;
    }
    for (int i = 0, iElementSize(elements.GetCount()); i < iElementSize; ++i)
    {
        bool bEquivalentExist = false;
        for (int k = 0, iSelectedElementSize(m_Elements.size()); k < iSelectedElementSize; ++k)
        {
            if (m_Elements[k].IsEquivalent(elements[i]))
            {
                bEquivalentExist = true;
                break;
            }
        }
        if (!bEquivalentExist)
        {
            m_Elements.push_back(elements[i]);
        }
    }
    return true;
}

bool ElementManager::Add(const SElement& element)
{
    for (int k = 0, iSelectedElementSize(m_Elements.size()); k < iSelectedElementSize; ++k)
    {
        if (m_Elements[k].IsEquivalent(element))
        {
            return false;
        }
    }
    m_Elements.push_back(element);
    return true;
}

bool ElementManager::Pick(CBaseObject* pObject, CD::Model* pModel, CViewport* viewport, const QPoint& point, int nFlag, bool bOnlyIncludeCube, BrushVec3* pOutPickedPos)
{
    BrushVec3 localRaySrc, localRayDir;
    CD::GetLocalViewRay(pObject->GetWorldTM(), viewport, point, localRaySrc, localRayDir);

    BrushVec3 outNearestPos;
    if (nFlag & CD::ePF_Vertex)
    {
        if (QueryNearestVertex(pObject, pModel, viewport, point, localRaySrc, localRayDir, outNearestPos))
        {
            Clear();
            SElement element;
            element.m_Vertices.push_back(outNearestPos);
            element.m_pObject = pObject;
            element.m_pPolygon = NULL;
            if (pOutPickedPos)
            {
                *pOutPickedPos = outNearestPos;
            }
            Add(element);
            return true;
        }
    }

    std::vector<CD::SQueryEdgeResult> queryResults;
    if (nFlag & CD::ePF_Edge)
    {
        std::vector< std::pair<BrushEdge3D, BrushVec3> > edges;
        QSize selectionAreaSize(10, 10);
        QSize halfSelectionAreaSize = selectionAreaSize / 2;
        QRect selectionRect(QPoint(point.x() - halfSelectionAreaSize.width(), point.y() - halfSelectionAreaSize.height()), selectionAreaSize);
        CD::PolygonPtr pRectPolygon = CD::MakePolygonFromRectangle(selectionRect);
        pModel->QueryIntersectionEdgesWith2DRect(viewport, pObject->GetWorldTM(), pRectPolygon, false, edges);
        if (!edges.empty())
        {
            Clear();
            std::vector<BrushEdge3D> nearestEdges = CD::FindNearestEdges(viewport, pObject->GetWorldTM(), edges);
            int nShortEdgeIndex = CD::FindShortestEdge(nearestEdges);
            BrushEdge3D shortestEdge = nearestEdges[nShortEdgeIndex];
            SElement element;
            element.m_Vertices.push_back(shortestEdge.m_v[0]);
            element.m_Vertices.push_back(shortestEdge.m_v[1]);
            element.m_pObject = pObject;
            element.m_pPolygon = NULL;
            if (pOutPickedPos)
            {
                *pOutPickedPos = shortestEdge.GetCenter();
            }
            Add(element);
            return true;
        }
    }

    if (nFlag & CD::ePF_Face)
    {
        int nPickedPolygon(-1);
        BrushVec3 hitPos;
        if (!bOnlyIncludeCube && pModel->QueryPolygon(localRaySrc, localRayDir, nPickedPolygon))
        {
            pModel->GetPolygon(nPickedPolygon)->GetPlane().HitTest(localRaySrc, localRaySrc + localRayDir, NULL, &hitPos);
            if (pOutPickedPos)
            {
                *pOutPickedPos = hitPos;
            }
        }

        BrushVec3 vPickedPosFromBox;
        CD::PolygonPtr pPickedPolygonFromBox = PickPolygonFromRepresentativeBox(pObject, pModel, viewport, point, localRaySrc, localRayDir, vPickedPosFromBox);

        if (nPickedPolygon != -1)
        {
            if (pPickedPolygonFromBox)
            {
                if (pPickedPolygonFromBox != pModel->GetPolygon(nPickedPolygon))
                {
                    BrushFloat distToBox = vPickedPosFromBox.GetDistance(localRaySrc);
                    BrushFloat distToFace = hitPos.GetDistance(localRaySrc);
                    if (distToFace < distToBox)
                    {
                        pPickedPolygonFromBox = pModel->GetPolygon(nPickedPolygon);
                    }
                    else
                    {
                        if (pOutPickedPos)
                        {
                            *pOutPickedPos = vPickedPosFromBox;
                        }
                    }
                }
            }
            else
            {
                pPickedPolygonFromBox = pModel->GetPolygon(nPickedPolygon);
            }
        }

        if (pPickedPolygonFromBox)
        {
            Clear();
            SElement element;
            element.SetFace(pObject, pPickedPolygonFromBox);
            Add(element);
            return true;
        }
    }

    return false;
}

void ElementManager::Display(CBaseObject* pObject, DisplayContext& dc) const
{
    float nOldLineWidth = dc.GetLineWidth();
    dc.SetColor(CD::kSelectedColor);
    dc.SetLineWidth(CD::kChosenLineThickness);

    dc.PopMatrix();

    if (CheckVirtualKey(Qt::Key_Space))
    {
        dc.DepthTestOff();
    }

    for (int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i)
    {
        if (m_Elements[i].IsVertex())
        {
            BrushVec3 worldVertexPos = pObject->GetWorldTM().TransformPoint(m_Elements[i].m_Vertices[0]);
            BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, worldVertexPos);
            dc.DrawSolidBox(CD::ToVec3(worldVertexPos - vBoxSize), CD::ToVec3(worldVertexPos + vBoxSize));
        }
    }
    dc.PushMatrix(pObject->GetWorldTM());

    for (int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i)
    {
        if (m_Elements[i].IsEdge())
        {
            dc.DrawLine(m_Elements[i].m_Vertices[0], m_Elements[i].m_Vertices[1]);
        }
        else if (m_Elements[i].IsFace() && m_Elements[i].m_pPolygon->IsOpen())
        {
            for (int k = 0, iEdgeCount(m_Elements[i].m_pPolygon->GetEdgeCount()); k < iEdgeCount; ++k)
            {
                BrushEdge3D e = m_Elements[i].m_pPolygon->GetEdge(k);
                dc.DrawLine(e.m_v[0], e.m_v[1]);
            }
        }
    }

    dc.SetLineWidth(nOldLineWidth);
    dc.DepthTestOn();
}

void ElementManager::DisplayHighlightElements(CBaseObject* pObject, CD::Model* pModel, DisplayContext& dc, int nPickFlag) const
{
    if (CheckVirtualKey(Qt::Key_Space))
    {
        dc.DepthTestOff();
    }

    if (nPickFlag & CD::ePF_Vertex)
    {
        DisplayVertexElements(pObject, pModel, dc);
    }
    if (nPickFlag & CD::ePF_Edge)
    {
        pModel->Display(dc, aznumeric_cast<int>(CD::kElementEdgeThickness), CD::kElementBoxColor);
    }
    if (nPickFlag & CD::ePF_Face)
    {
        DisplayFaceElements(pObject, pModel, dc);
    }

    dc.DepthTestOn();
}

void ElementManager::DisplayVertexElements(CBaseObject* pObject, CD::Model* pModel, DisplayContext& dc, int nShelf, std::vector<BrushVec3>* pExcludedVertices) const
{
    dc.PopMatrix();

    MODEL_SHELF_RECONSTRUCTOR(pModel);
    for (int k = 0; k < 2; ++k)
    {
        if (nShelf != -1 && nShelf != k)
        {
            continue;
        }
        pModel->SetShelf(k);
        for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
            if (pPolygon->CheckFlags(CD::ePolyFlag_Hidden | CD::ePolyFlag_Mirrored))
            {
                continue;
            }
            for (int a = 0, iVertexCount(pPolygon->GetVertexCount()); a < iVertexCount; ++a)
            {
                const BrushVec3& v = pPolygon->GetPos(a);
                if (HasVertex(v))
                {
                    continue;
                }
                if (pExcludedVertices)
                {
                    bool bFound = false;
                    for (int b = 0, iExcludedVertexCount(pExcludedVertices->size()); b < iExcludedVertexCount; ++b)
                    {
                        if (CD::IsEquivalent((*pExcludedVertices)[b], v))
                        {
                            bFound = true;
                            break;
                        }
                    }
                    if (bFound)
                    {
                        continue;
                    }
                }
                BrushVec3 vWorldVertexPos = pObject->GetWorldTM().TransformPoint(v);
                BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, vWorldVertexPos);
                dc.SetColor(CD::kElementBoxColor);
                if (pPolygon->IsOpen())
                {
                    std::vector<int> edgeIndices;
                    if (pPolygon->QueryEdgesHavingVertex(v, edgeIndices) && edgeIndices.size() == 1)
                    {
                        BrushEdge3D e = pPolygon->GetEdge(edgeIndices[0]);
                        if (CD::IsEquivalent(e.m_v[0], v))
                        {
                            dc.SetColor(ColorB(0xFFFFAAFF));
                        }
                    }
                }
                dc.DrawSolidBox(CD::ToVec3(vWorldVertexPos - vBoxSize), CD::ToVec3(vWorldVertexPos + vBoxSize));
            }
        }
    }

    dc.PushMatrix(pObject->GetWorldTM());
}

void ElementManager::DisplayFaceElements(CBaseObject* pObject, CD::Model* pModel, DisplayContext& dc) const
{
    dc.PopMatrix();

    MODEL_SHELF_RECONSTRUCTOR(pModel);
    for (int k = 0; k < 2; ++k)
    {
        pModel->SetShelf(k);
        for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
        {
            CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
            if (!pPolygon->IsValid() || pPolygon->CheckFlags(CD::ePolyFlag_Hidden | CD::ePolyFlag_Mirrored))
            {
                continue;
            }
            if (HasPolygonSelected(pPolygon))
            {
                dc.SetColor(CD::kSelectedColor);
            }
            else
            {
                dc.SetColor(CD::kElementBoxColor);
            }
            BrushVec3 pos = pObject->GetWorldTM().TransformPoint(pPolygon->GetRepresentativePosition());
            BrushVec3 vBoxSize = CD::GetElementBoxSize(dc.view, dc.flags & DISPLAY_2D, pos);
            dc.DrawSolidBox(CD::ToVec3(pos - vBoxSize), CD::ToVec3(pos + vBoxSize));
        }
    }

    dc.PushMatrix(pObject->GetWorldTM());
}

void ElementManager::PickAdjacentCurvedEdges(CBaseObject* pObject, CD::PolygonPtr pPolygon, const BrushEdge3D& edge, std::vector<SElement>* outPickInfo) const
{
    int nInitialIndex = pPolygon->GetEdgeIndex(edge);
    int nEdgeIndex(nInitialIndex);
    if (nEdgeIndex != -1)
    {
        while (1)
        {
            int nPrevEdgeIndex = -1;
            if (!pPolygon->GetAdjacentEdgesByEdgeIndex(nEdgeIndex, &nPrevEdgeIndex, NULL) || nPrevEdgeIndex == -1 || nInitialIndex == nPrevEdgeIndex)
            {
                break;
            }

            BrushEdge3D currentEdge = pPolygon->GetEdge(nEdgeIndex);
            BrushEdge3D prevEdge = pPolygon->GetEdge(nPrevEdgeIndex);
            if (currentEdge.GetDirection().Dot(-prevEdge.GetDirection()) >= 0)
            {
                break;
            }

            SElement element;
            element.m_Vertices.push_back(prevEdge.m_v[0]);
            element.m_Vertices.push_back(prevEdge.m_v[1]);
            element.m_pObject = pObject;
            outPickInfo->push_back(element);
            nEdgeIndex = nPrevEdgeIndex;
        }

        nEdgeIndex = nInitialIndex;

        while (1)
        {
            int nNextEdgeIndex = -1;
            if (!pPolygon->GetAdjacentEdgesByEdgeIndex(nEdgeIndex, NULL, &nNextEdgeIndex) || nNextEdgeIndex == -1 || nInitialIndex == nNextEdgeIndex)
            {
                break;
            }

            BrushEdge3D currentEdge = pPolygon->GetEdge(nEdgeIndex);
            BrushEdge3D nextEdge = pPolygon->GetEdge(nNextEdgeIndex);
            if (nextEdge.GetDirection().Dot(-currentEdge.GetDirection()) >= 0)
            {
                break;
            }

            SElement element;
            element.m_Vertices.push_back(nextEdge.m_v[0]);
            element.m_Vertices.push_back(nextEdge.m_v[1]);
            element.m_pObject = pObject;
            outPickInfo->push_back(element);
            nEdgeIndex = nNextEdgeIndex;
        }
    }
}

void ElementManager::FindElementsInRect(const QRect& rect, CViewport* pView, const Matrix34& worldTM, bool bOnlyUseSelectionCube, std::vector<SElement>& elements) const
{
    for (int i = 0, iCount(m_Elements.size()); i < iCount; ++i)
    {
        const SElement& element = m_Elements[i];

        if (element.IsVertex())
        {
            QPoint pt = pView->WorldToView(worldTM.TransformPoint(element.GetPos()));
            if (rect.contains(pt))
            {
                elements.push_back(element);
            }
        }

        if (element.IsEdge())
        {
            if (CD::DoesEdgeIntersectRect(rect, pView, worldTM, element.GetEdge()))
            {
                elements.push_back(element);
            }
        }

        if (element.IsFace() && element.m_pPolygon)
        {
            bool bAdded = false;
            CD::PolygonPtr pRectPolygon = CD::MakePolygonFromRectangle(rect);
            if (!bOnlyUseSelectionCube && element.m_pPolygon->InRectangle(pView, worldTM, pRectPolygon, false))
            {
                bAdded = true;
                elements.push_back(element);
            }
            if (!bAdded && gDesignerSettings.bHighlightElements)
            {
                if (CD::DoesSelectionBoxIntersectRect(pView, worldTM, rect, element.m_pPolygon))
                {
                    elements.push_back(element);
                }
            }
        }
    }
}

BrushVec3 ElementManager::GetNormal(CD::Model* pModel) const
{
    if (m_Elements.empty())
    {
        return BrushVec3(0, 0, 1);
    }

    BrushVec3 normal(0, 0, 0);
    for (int i = 0, iSize(m_Elements.size()); i < iSize; ++i)
    {
        if (m_Elements[i].IsFace())
        {
            normal += m_Elements[i].m_pPolygon->GetPlane().Normal();
        }
    }

    if (!normal.IsZero())
    {
        return normal.GetNormalized();
    }

    ModelDB::QueryResult queryResult = QueryFromElements(pModel);
    for (int i = 0, iQuerySize(queryResult.size()); i < iQuerySize; ++i)
    {
        for (int k = 0, iMarkListSize(queryResult[i].m_MarkList.size()); k < iMarkListSize; ++k)
        {
            CD::PolygonPtr pPolygon = queryResult[i].m_MarkList[k].m_pPolygon;
            if (pPolygon == NULL)
            {
                continue;
            }
            normal += pPolygon->GetPlane().Normal();
        }
    }

    return normal.GetNormalized();
}

void ElementManager::Erase(int nElementFlags)
{
    std::vector<SElement>::iterator ii = m_Elements.begin();
    for (; ii != m_Elements.end(); )
    {
        if ((nElementFlags& CD::ePF_Vertex) && (*ii).IsVertex() || (nElementFlags& CD::ePF_Edge) && (*ii).IsEdge() || (nElementFlags& CD::ePF_Face) && (*ii).IsFace())
        {
            ii = m_Elements.erase(ii);
        }
        else
        {
            ++ii;
        }
    }
}

bool ElementManager::Erase(ElementManager& elements)
{
    std::vector<SElement>::iterator ii = m_Elements.begin();
    bool bErasedAtLeastOne = false;
    for (; ii != m_Elements.end(); )
    {
        bool bErased = false;
        for (int i = 0, iElementCount(elements.GetCount()); i < iElementCount; ++i)
        {
            if ((*ii) == elements[i])
            {
                ii = m_Elements.erase(ii);
                bErasedAtLeastOne = bErased = true;
                break;
            }
        }
        if (!bErased)
        {
            ++ii;
        }
    }
    return bErasedAtLeastOne;
}

void ElementManager::Erase(const SElement& element)
{
    std::vector<SElement>::iterator ii = m_Elements.begin();
    for (; ii != m_Elements.end(); )
    {
        if ((*ii) == element)
        {
            ii = m_Elements.erase(ii);
        }
        else
        {
            ++ii;
        }
    }
}

bool ElementManager::Has(const SElement& elementInfo) const
{
    if (m_Elements.empty())
    {
        return false;
    }

    for (int i = 0, iElementSize(m_Elements.size()); i < iElementSize; ++i)
    {
        if (m_Elements[i].IsEquivalent(elementInfo))
        {
            return true;
        }
    }

    return false;
}

bool ElementManager::HasVertex(const BrushVec3& vertex) const
{
    for (int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i)
    {
        if (!m_Elements[i].IsVertex())
        {
            continue;
        }

        if (CD::IsEquivalent(m_Elements[i].GetPos(), vertex))
        {
            return true;
        }
    }

    return false;
}

QString ElementManager::GetElementsInfoText()
{
    int nVertexNum = 0;
    int nEdgeNum = 0;
    int nFaceNum = 0;
    for (int i = 0, iCount(m_Elements.size()); i < iCount; ++i)
    {
        if (m_Elements[i].IsVertex())
        {
            ++nVertexNum;
        }
        else if (m_Elements[i].IsEdge())
        {
            ++nEdgeNum;
        }
        else if (m_Elements[i].IsFace())
        {
            ++nFaceNum;
        }
    }

    QString str;
    if (nVertexNum > 0)
    {
        str += QObject::tr("%1 Vertex(s)").arg(nVertexNum);
    }
    if (nEdgeNum > 0)
    {
        if (!str.isEmpty())
        {
            str += QStringLiteral(",");
        }
        str += QObject::tr("%1 Edge(s)").arg(nEdgeNum);
    }
    if (nFaceNum > 0)
    {
        if (!str.isEmpty())
        {
            str += QStringLiteral(",");
        }
        str += QObject::tr("%1 Face(s)").arg(nFaceNum);
    }

    return str;
}

void ElementManager::RemoveInvalidElements()
{
    std::vector<SElement>::iterator ii = m_Elements.begin();
    for (; ii != m_Elements.end(); )
    {
        if ((*ii).IsFace() && ((*ii).m_pPolygon == NULL || !(*ii).m_pPolygon->IsValid()))
        {
            ii = m_Elements.erase(ii);
        }
        else
        {
            ++ii;
        }
    }
}

ModelDB::QueryResult ElementManager::QueryFromElements(CD::Model* pModel) const
{
    MODEL_SHELF_RECONSTRUCTOR(pModel);
    ModelDB::QueryResult queryResult;

    for (int i = 0, iSelectedSize(m_Elements.size()); i < iSelectedSize; ++i)
    {
        int iVertexSize(m_Elements[i].m_Vertices.size());
        for (int k = 0; k < iVertexSize; ++k)
        {
            pModel->GetDB()->QueryAsVertex(m_Elements[i].m_Vertices[k], queryResult);
        }
    }

    if (!pModel->CheckModeFlag(CD::eDesignerMode_Mirror))
    {
        return queryResult;
    }

    ModelDB::QueryResult::iterator iQuery = queryResult.begin();
    for (; iQuery != queryResult.end(); )
    {
        ModelDB::MarkList::iterator iMark = (*iQuery).m_MarkList.begin();
        for (; iMark != (*iQuery).m_MarkList.end(); )
        {
            CD::PolygonPtr pPolygon = iMark->m_pPolygon;
            if (pPolygon == NULL || pPolygon->CheckFlags(CD::ePolyFlag_Mirrored))
            {
                iMark = (*iQuery).m_MarkList.erase(iMark);
            }
            else
            {
                ++iMark;
            }
        }

        if ((*iQuery).m_MarkList.empty())
        {
            iQuery = queryResult.erase(iQuery);
        }
        else
        {
            ++iQuery;
        }
    }

    return queryResult;
}

bool ElementManager::QueryNearestVertex(CBaseObject* pObject, CD::Model* pModel, CViewport* pView, const QPoint& point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPos, BrushVec3* pOutNormal) const
{
    Vec3 raySrc, rayDir;
    pView->ViewToWorldRay(point, raySrc, rayDir);

    BrushFloat fLeastDist = (BrushFloat)3e10;
    bool bFound = false;

    for (int a = 0, iPolygonCount(pModel->GetPolygonCount()); a < iPolygonCount; ++a)
    {
        CD::PolygonPtr pPolygon = pModel->GetPolygon(a);
        if (pPolygon->CheckFlags(CD::ePolyFlag_Hidden))
        {
            continue;
        }
        for (int i = 0, iVertexCount(pPolygon->GetVertexCount()); i < iVertexCount; ++i)
        {
            const BrushVec3& v = pPolygon->GetPos(i);
            BrushFloat t = 0;
            BrushVec3 vWorldPos = pObject->GetWorldTM().TransformPoint(v);
            BrushVec3 vBoxSize = CD::GetElementBoxSize(pView, pView->GetType() != ET_ViewportCamera, vWorldPos);
            if (!CD::GetIntersectionOfRayAndAABB(CD::ToBrushVec3(raySrc), CD::ToBrushVec3(rayDir), AABB(CD::ToVec3(vWorldPos - vBoxSize), CD::ToVec3(vWorldPos + vBoxSize)), &t))
            {
                continue;
            }
            if (t > 0 && t < fLeastDist)
            {
                fLeastDist = t;
                outPos = v;
                if (pOutNormal)
                {
                    if (pPolygon->IsOpen())
                    {
                        int nPolygonIndex = -1;
                        if (pModel->QueryPolygon(rayLocalSrc, rayLocalDir, nPolygonIndex))
                        {
                            CD::PolygonPtr pClosedPolygon = pModel->GetPolygon(nPolygonIndex);
                            *pOutNormal = pClosedPolygon->GetPlane().Normal();
                        }
                        else
                        {
                            *pOutNormal = BrushVec3(0, 0, 1);
                        }
                    }
                    else
                    {
                        *pOutNormal = pPolygon->GetPlane().Normal();
                    }
                }
                bFound = true;
            }
        }
    }

    return bFound;
}

bool ElementManager::HasPolygonSelected(CD::PolygonPtr pPolygon) const
{
    for (int i = 0, iCount(m_Elements.size()); i < iCount; ++i)
    {
        if (m_Elements[i].IsFace() && m_Elements[i].m_pPolygon == pPolygon)
        {
            return true;
        }
    }
    return false;
}

CD::PolygonPtr ElementManager::PickPolygonFromRepresentativeBox(CBaseObject* pObject, CD::Model* pModel, CViewport* pView, const QPoint& point, const BrushVec3& rayLocalSrc, const BrushVec3& rayLocalDir, BrushVec3& outPickedPos) const
{
    if (!gDesignerSettings.bHighlightElements)
    {
        return NULL;
    }

    Vec3 raySrc, rayDir;
    pView->ViewToWorldRay(point, raySrc, rayDir);

    MODEL_SHELF_RECONSTRUCTOR(pModel);

    BrushFloat fLeastDist = (BrushFloat)3e10;
    CD::PolygonPtr pPickedPolygon = NULL;

    BrushMatrix34 matInvWorld = pObject->GetWorldTM().GetInverted();

    for (int i = 0; i < CD::kMaxShelfCount; ++i)
    {
        pModel->SetShelf(i);
        for (int k = 0, iPolygonCount(pModel->GetPolygonCount()); k < iPolygonCount; ++k)
        {
            CD::PolygonPtr pPolygon = pModel->GetPolygon(k);
            if (!pPolygon->IsValid() || pPolygon->CheckFlags(CD::ePolyFlag_Hidden))
            {
                continue;
            }
            BrushVec3 v = pPolygon->GetRepresentativePosition();
            BrushFloat t = 0;
            BrushVec3 vWorldPos = pObject->GetWorldTM().TransformPoint(v);
            BrushVec3 vBoxSize = CD::GetElementBoxSize(pView, pView->GetType() != ET_ViewportCamera, vWorldPos);
            if (CD::GetIntersectionOfRayAndAABB(CD::ToBrushVec3(raySrc), CD::ToBrushVec3(rayDir), AABB(CD::ToVec3(vWorldPos - vBoxSize), CD::ToVec3(vWorldPos + vBoxSize)), &t))
            {
                if (t > 0 && t < fLeastDist)
                {
                    fLeastDist = t;
                    outPickedPos = matInvWorld.TransformPoint(raySrc + rayDir * aznumeric_cast<float>(t));
                    pPickedPolygon = pPolygon;
                }
            }
        }
    }

    return pPickedPolygon;
}

int ElementManager::GetFaceCount() const
{
    int nFaceCount = 0;
    for (int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i)
    {
        if (m_Elements[i].IsFace())
        {
            ++nFaceCount;
        }
    }
    return nFaceCount;
}

int ElementManager::GetEdgeCount() const
{
    int nEdgeCount = 0;
    for (int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i)
    {
        if (m_Elements[i].IsEdge())
        {
            ++nEdgeCount;
        }
    }
    return nEdgeCount;
}

int ElementManager::GetVertexCount() const
{
    int nVertexCount = 0;
    for (int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i)
    {
        if (m_Elements[i].IsVertex())
        {
            ++nVertexCount;
        }
    }
    return nVertexCount;
}

void ElementManager::ApplyOffset(const BrushVec3& vOffset)
{
    for (int i = 0, iElementCount(m_Elements.size()); i < iElementCount; ++i)
    {
        for (int k = 0, iVertexCount(m_Elements[i].m_Vertices.size()); k < iVertexCount; ++k)
        {
            m_Elements[i].m_Vertices[k] += vOffset;
        }
    }
}

