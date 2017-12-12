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
#include "Viewport.h"
#include "SpotManager.h"
#include "Tools/BaseTool.h"
#include "Core/Model.h"
#include "ViewManager.h"
#include "SurfaceInfoPicker.h"

void SpotManager::DrawCurrentSpot(DisplayContext& dc, const BrushMatrix34& worldTM) const
{
    static const ColorB edgeCenterColor(100, 255, 100, 255);
    static const ColorB polygonCenterColor(100, 255, 100, 255);
    static const ColorB eitherPointColor(255, 100, 255, 255);
    static const ColorB normalColor(100, 100, 100, 255);
    static const ColorB edgeColor(100, 100, 255, 255);
    static const ColorB startPointColor(255, 255, 100, 255);

    if (m_CurrentSpot.m_PosState == eSpotPosState_Edge || m_CurrentSpot.m_PosState == eSpotPosState_OnVirtualLine)
    {
        CD::DrawSpot(dc, worldTM, m_CurrentSpot.m_Pos, edgeColor);
    }
    else if (m_CurrentSpot.m_PosState == eSpotPosState_CenterOfEdge)
    {
        CD::DrawSpot(dc, worldTM, m_CurrentSpot.m_Pos, edgeCenterColor);
    }
    else if (m_CurrentSpot.m_PosState == eSpotPosState_CenterOfPolygon)
    {
        CD::DrawSpot(dc, worldTM, m_CurrentSpot.m_Pos, polygonCenterColor);
    }
    else if (m_CurrentSpot.m_PosState == eSpotPosState_EitherPointOfEdge || m_CurrentSpot.IsAtEndPoint())
    {
        CD::DrawSpot(dc, worldTM, m_CurrentSpot.m_Pos, eitherPointColor);
    }
    else if (m_CurrentSpot.m_PosState == eSpotPosState_AtFirstSpot)
    {
        CD::DrawSpot(dc, worldTM, m_CurrentSpot.m_Pos, startPointColor);
    }
    else
    {
        CD::DrawSpot(dc, worldTM, m_CurrentSpot.m_Pos, normalColor);
    }
}

void SpotManager::DrawPolyline(DisplayContext& dc) const
{
    if (m_SpotList.empty())
    {
        return;
    }

    for (int i = 0, iSpotSize(m_SpotList.size() - 1); i < iSpotSize; ++i)
    {
        if (m_SpotList[i].m_bProcessed)
        {
            continue;
        }
        const BrushVec3& currPos(m_SpotList[i].m_Pos);
        const BrushVec3& nextPos(m_SpotList[i + 1].m_Pos);
        dc.DrawLine(currPos, nextPos);
    }
}

bool SpotManager::AddPolygonToDesignerFromSpotList(CD::Model* pModel, const SpotList& spotList)
{
    if (pModel == NULL || spotList.empty())
    {
        return false;
    }

    SpotList copiedSpotList(spotList);

    SSpot firstSpot = copiedSpotList[0];
    SSpot secondSpot = copiedSpotList.size() >= 2 ? copiedSpotList[1] : copiedSpotList[0];
    SSpot lastSpot = copiedSpotList[copiedSpotList.size() - 1];

    if (firstSpot.IsEquivalentPos(lastSpot))
    {
        SpotList spotListWithoutBeginning(copiedSpotList);
        spotListWithoutBeginning.erase(spotListWithoutBeginning.begin());
        CreatePolygonFromSpots(true, spotListWithoutBeginning);
    }
    else if (firstSpot.IsAtEndPoint() && lastSpot.IsAtEndPoint())
    {
        CD::PolygonPtr pPolygon0 = firstSpot.m_pPolygon;
        CD::PolygonPtr pPolygon1 = lastSpot.m_pPolygon;
        if (pPolygon0)
        {
            for (int i = 0, nSpotSize(copiedSpotList.size() - 1); i < nSpotSize; ++i)
            {
                if (firstSpot.m_PosState == eSpotPosState_FirstPointOfPolygon)
                {
                    pPolygon0->AddEdge(BrushEdge3D(copiedSpotList[i + 1].m_Pos, copiedSpotList[i].m_Pos));
                }
                else if (firstSpot.m_PosState == eSpotPosState_LastPointOfPolygon)
                {
                    pPolygon0->AddEdge(BrushEdge3D(copiedSpotList[i].m_Pos, copiedSpotList[i + 1].m_Pos));
                }
            }
            if (pPolygon1 && pPolygon0 != pPolygon1)
            {
                if (pPolygon0->Concatenate(pPolygon1))
                {
                    pModel->RemovePolygon(pPolygon1);
                }
            }

            BrushVec3 firstVertex;
            BrushVec3 lastVertex;
            pPolygon0->GetFirstVertex(firstVertex);
            pPolygon0->GetLastVertex(lastVertex);

            if (!pPolygon0->IsOpen())
            {
                pModel->RemovePolygon(pPolygon0);
                pPolygon0->ModifyOrientation();
                pModel->AddPolygon(pPolygon0, CD::eOpType_Split);
            }
            else if (pModel->IsVertexOnEdge(pPolygon0->GetPlane(), firstVertex, pPolygon0) && pModel->IsVertexOnEdge(pPolygon0->GetPlane(), lastVertex, pPolygon0))
            {
                pModel->RemovePolygon(pPolygon0);
                pModel->AddOpenPolygon(pPolygon0, false);
            }
        }
    }
    else if (firstSpot.IsAtEndPoint() || lastSpot.IsAtEndPoint())
    {
        if (!firstSpot.IsAtEndPoint())
        {
            std::swap(firstSpot, lastSpot);
            int iSize(copiedSpotList.size());
            for (int i = 0, iHalfSize(iSize / 2); i < iHalfSize; ++i)
            {
                std::swap(copiedSpotList[i], copiedSpotList[iSize - i - 1]);
            }
        }

        CD::PolygonPtr pPolygon0 = firstSpot.m_pPolygon;
        if (!pPolygon0)
        {
            return true;
        }

        pModel->RemovePolygon(pPolygon0);

        for (int i = 0, nSpotSize(copiedSpotList.size() - 1); i < nSpotSize; ++i)
        {
            if (firstSpot.m_PosState == eSpotPosState_FirstPointOfPolygon)
            {
                pPolygon0->AddEdge(BrushEdge3D(copiedSpotList[i + 1].m_Pos, copiedSpotList[i].m_Pos));
            }
            else if (firstSpot.m_PosState == eSpotPosState_LastPointOfPolygon)
            {
                pPolygon0->AddEdge(BrushEdge3D(copiedSpotList[i].m_Pos, copiedSpotList[i + 1].m_Pos));
            }
        }

        if (!pPolygon0->IsOpen())
        {
            pModel->AddPolygon(pPolygon0, CD::eOpType_Split);
        }
        else
        {
            BrushVec3 firstVertex;
            BrushVec3 lastVertex;
            bool bOnlyAdd = false;

            bool bFirstOnEdge = pPolygon0->GetFirstVertex(firstVertex) && pModel->IsVertexOnEdge(pPolygon0->GetPlane(), firstVertex, pPolygon0);
            bool bLastOnEdge = pPolygon0->GetLastVertex(lastVertex) && pModel->IsVertexOnEdge(pPolygon0->GetPlane(), lastVertex, pPolygon0);

            if (bFirstOnEdge && bLastOnEdge)
            {
                bOnlyAdd = false;
            }
            else
            {
                bOnlyAdd = true;
            }

            pModel->AddOpenPolygon(pPolygon0, bOnlyAdd);
        }
    }
    else if (firstSpot.IsOnEdge() && lastSpot.IsOnEdge())
    {
        BrushPlane plane;
        if (FindBestPlane(pModel, firstSpot, secondSpot, plane))
        {
            std::vector<BrushVec3> vList;
            GenerateVertexListFromSpotList(copiedSpotList, vList);
            CD::PolygonPtr pOpenPolygon = new CD::Polygon(vList, plane, 0, NULL, false);
            pModel->AddOpenPolygon(pOpenPolygon, false);
        }
    }
    else if (firstSpot.IsInPolygon() || lastSpot.IsInPolygon())
    {
        BrushPlane plane;
        if (FindBestPlane(pModel, firstSpot, secondSpot, plane))
        {
            std::vector<BrushVec3> vList;
            GenerateVertexListFromSpotList(copiedSpotList, vList);
            CD::PolygonPtr pPolygon = new CD::Polygon(vList, plane, 0, NULL, false);
            pModel->AddOpenPolygon(pPolygon, true);
        }
    }
    else if (firstSpot.m_PosState == eSpotPosState_OutsideDesigner && lastSpot.m_PosState == eSpotPosState_OutsideDesigner)
    {
        if (firstSpot.m_Plane.IsEquivalent(lastSpot.m_Plane))
        {
            std::vector<BrushVec3> vList;
            GenerateVertexListFromSpotList(copiedSpotList, vList);
            CD::PolygonPtr pPolygon = new CD::Polygon(vList, firstSpot.m_Plane, 0, NULL, false);
            pModel->AddOpenPolygon(pPolygon, true);
        }
    }
    else
    {
        ResetAllSpots();
        return false;
    }

    if (pModel)
    {
        pModel->ResetDB(CD::eDBRF_ALL);
    }

    return true;
}

void SpotManager::GenerateVertexListFromSpotList(const SpotList& spotList, std::vector<BrushVec3>& outVList)
{
    int iSpotSize(spotList.size());
    outVList.reserve(iSpotSize);
    for (int i = 0; i < iSpotSize; ++i)
    {
        outVList.push_back(spotList[i].m_Pos);
    }
}

void SpotManager::RegisterSpotList(CD::Model* pModel, const SpotList& spotList)
{
    if (pModel == NULL)
    {
        return;
    }

    if (spotList.size() <= 1)
    {
        ResetAllSpots();
        return;
    }

    std::vector<SpotList> splittedSpotList;
    SplitSpotList(pModel, spotList, splittedSpotList);

    if (!splittedSpotList.empty())
    {
        for (int i = 0, spotListCount(splittedSpotList.size()); i < spotListCount; ++i)
        {
            AddPolygonToDesignerFromSpotList(pModel, splittedSpotList[i]);
        }
    }
}

void SpotManager::SplitSpotList(CD::Model* pModel, const SpotList& spotList, std::vector<SpotList>& outSpotLists)
{
    if (pModel == NULL)
    {
        return;
    }

    SpotList partSpotList;

    for (int k = 0, iSpotListCount(spotList.size()); k < iSpotListCount - 1; ++k)
    {
        const SSpot& currentSpot = spotList[k];
        const SSpot& nextSpot = spotList[k + 1];

        SpotPairList splittedSpotPairs;
        SplitSpot(pModel, SSpotPair(currentSpot, nextSpot), splittedSpotPairs);

        if (splittedSpotPairs.empty())
        {
            continue;
        }

        std::map<BrushFloat, SSpotPair> sortedSpotPairs;
        for (int i = 0, iSpotPairCount(splittedSpotPairs.size()); i < iSpotPairCount; ++i)
        {
            BrushFloat fDistance = currentSpot.m_Pos.GetDistance(splittedSpotPairs[i].m_Spot[0].m_Pos);
            sortedSpotPairs[fDistance] = splittedSpotPairs[i];
        }

        std::map<BrushFloat, SSpotPair>::iterator ii = sortedSpotPairs.begin();
        for (; ii != sortedSpotPairs.end(); ++ii)
        {
            const SSpotPair& spotPair = ii->second;

            if (partSpotList.empty())
            {
                partSpotList.push_back(spotPair.m_Spot[0]);
            }

            partSpotList.push_back(spotPair.m_Spot[1]);
            if (!spotPair.m_Spot[1].IsEquivalentPos(nextSpot))
            {
                outSpotLists.push_back(partSpotList);
                partSpotList.clear();
            }
        }
    }
    if (!partSpotList.empty())
    {
        outSpotLists.push_back(partSpotList);
    }
}

void SpotManager::SplitSpot(CD::Model* pModel, const SSpotPair& spotPair, SpotPairList& outSpotPairs)
{
    if (pModel == NULL)
    {
        return;
    }

    bool bSubtracted = false;
    BrushEdge3D inputEdge(spotPair.m_Spot[0].m_Pos, spotPair.m_Spot[1].m_Pos);
    BrushEdge3D invInputEdge(spotPair.m_Spot[1].m_Pos, spotPair.m_Spot[0].m_Pos);

    BrushPlane plane;
    if (!FindBestPlane(pModel, spotPair.m_Spot[0], spotPair.m_Spot[1], plane))
    {
        return;
    }

    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
        if (pPolygon == NULL)
        {
            continue;
        }

        if (!pPolygon->GetPlane().IsEquivalent(plane))
        {
            continue;
        }

        std::vector<BrushEdge3D> subtractedEdges;
        bSubtracted = pPolygon->SubtractEdge(inputEdge, subtractedEdges);

        if (!subtractedEdges.empty())
        {
            std::vector<BrushEdge3D>::iterator ii = subtractedEdges.begin();
            for (; ii != subtractedEdges.end(); )
            {
                if ((*ii).IsEquivalent(inputEdge) || (*ii).IsEquivalent(invInputEdge))
                {
                    ii = subtractedEdges.erase(ii);
                }
                else
                {
                    ++ii;
                }
            }
            if (subtractedEdges.empty())
            {
                bSubtracted = false;
            }
        }

        if (subtractedEdges.empty())
        {
            if (bSubtracted)
            {
                break;
            }
            continue;
        }

        if (subtractedEdges.size() == 1)
        {
            std::vector<BrushEdge3D>::iterator ii = subtractedEdges.begin();
            if (CD::IsEquivalent((*ii).m_v[0], (*ii).m_v[1]))
            {
                continue;
            }
        }

        int iSubtractedEdgeCount(subtractedEdges.size());

        for (int k = 0; k < iSubtractedEdgeCount; ++k)
        {
            BrushEdge3D subtractedEdge = subtractedEdges[k];
            SSpotPair subtractedSpotPair;

            for (int a = 0; a < 2; ++a)
            {
                subtractedSpotPair.m_Spot[a].m_Pos = subtractedEdge.m_v[a];

                if (pPolygon->HasVertex(subtractedEdge.m_v[a]))
                {
                    subtractedSpotPair.m_Spot[a].m_PosState = eSpotPosState_EitherPointOfEdge;
                    subtractedSpotPair.m_Spot[a].m_pPolygon = pPolygon;
                }
                else if (pPolygon->IsVertexOnCrust(subtractedEdge.m_v[a]))
                {
                    subtractedSpotPair.m_Spot[a].m_PosState = eSpotPosState_Edge;
                    subtractedSpotPair.m_Spot[a].m_pPolygon = pPolygon;
                }
            }

            SplitSpot(pModel, subtractedSpotPair, outSpotPairs);
        }
        break;
    }

    if (!bSubtracted)
    {
        outSpotPairs.push_back(spotPair);
    }
}

bool SpotManager::FindSpotNearAxisAlignedLine(IDisplayViewport* pViewport, CD::PolygonPtr pPolygon, const BrushMatrix34& worldTM, SSpot& outSpot)
{
    if (m_CurrentSpot.IsOnEdge() || !pPolygon->Include(m_CurrentSpot.m_Pos))
    {
        return false;
    }

    std::vector<BrushLine> axisAlignedLines;
    if (!pPolygon->QueryAxisAlignedLines(axisAlignedLines))
    {
        return false;
    }

    BrushVec2 vCurrentPos2D = pPolygon->GetPlane().W2P(m_CurrentSpot.m_Pos);
    for (int i = 0, iLineCount(axisAlignedLines.size()); i < iLineCount; ++i)
    {
        BrushVec2 vHitPos;
        if (!axisAlignedLines[i].HitTest(vCurrentPos2D, vCurrentPos2D + axisAlignedLines[i].m_Normal, 0, &vHitPos))
        {
            continue;
        }

        BrushFloat fLength(0);
        if (CD::AreTwoPositionsNear(pPolygon->GetPlane().P2W(vHitPos), m_CurrentSpot.m_Pos, worldTM, pViewport, kLimitForMagnetic, &fLength))
        {
            outSpot.Reset();
            outSpot.m_pPolygon = pPolygon;
            outSpot.m_PosState = eSpotPosState_OnVirtualLine;
            outSpot.m_Plane = pPolygon->GetPlane();
            outSpot.m_Pos = outSpot.m_Plane.P2W(vHitPos);
            return true;
        }
    }

    return false;
}

bool SpotManager::FindNicestSpot(IDisplayViewport* pViewport, const std::vector<SCandidateInfo>& candidates, const CD::Model* pModel, const BrushMatrix34& worldTM, const BrushVec3& pickedPos, CD::PolygonPtr pPickedPolygon, const BrushPlane& plane, SSpot& outSpot) const
{
    if (!pModel)
    {
        return false;
    }

    BrushFloat fMinimumLength(3e10f);

    for (int i = 0; i < candidates.size(); ++i)
    {
        BrushFloat fLength(0);
        if (!CD::AreTwoPositionsNear(pickedPos, candidates[i].m_Pos, worldTM, pViewport, kLimitForMagnetic, &fLength))
        {
            continue;
        }

        if (fLength >= fMinimumLength)
        {
            continue;
        }

        fMinimumLength = fLength;

        outSpot.m_Pos = candidates[i].m_Pos;
        outSpot.m_PosState = candidates[i].m_SpotPosState;
        if (pPickedPolygon)
        {
            outSpot.m_pPolygon = pPickedPolygon;
            outSpot.m_Plane = pPickedPolygon->GetPlane();
        }
        else
        {
            outSpot.m_pPolygon = NULL;
            outSpot.m_Plane = plane;
        }

        if (outSpot.m_PosState != eSpotPosState_EitherPointOfEdge)
        {
            continue;
        }

        bool bFirstPoint = false;
        if (!pPickedPolygon || !pPickedPolygon->IsEndPoint(outSpot.m_Pos, &bFirstPoint))
        {
            continue;
        }

        if (bFirstPoint)
        {
            outSpot.m_PosState = eSpotPosState_FirstPointOfPolygon;
        }
        else
        {
            outSpot.m_PosState = eSpotPosState_LastPointOfPolygon;
        }
    }

    return true;
}

bool SpotManager::FindSnappedSpot(const BrushMatrix34& worldTM, CD::PolygonPtr pPickedPolygon, const BrushVec3& pickedPos, SSpot& outSpot) const
{
    bool bEnableSnap = IsSnapEnabled();
    if (!bEnableSnap)
    {
        return false;
    }

    if (!pPickedPolygon || pPickedPolygon->IsOpen())
    {
        return false;
    }

    BrushPlane pickedPlane(pPickedPolygon->GetPlane());
    BrushVec3 snappedPlanePos = Snap(pickedPos);
    outSpot.m_Pos = pickedPlane.P2W(pickedPlane.W2P(snappedPlanePos));
    outSpot.m_pPolygon = pPickedPolygon;
    outSpot.m_Plane = pPickedPolygon->GetPlane();
    return true;
}

bool SpotManager::UpdateCurrentSpotPosition(
    CD::Model* pModel,
    const BrushMatrix34& worldTM,
    const BrushPlane& plane,
    IDisplayViewport* view,
    const QPoint& point,
    bool bKeepInitialPlane,
    bool bSearchAllShelves)
{
    if (pModel == NULL)
    {
        return false;
    }

    BrushVec3 localRaySrc, localRayDir;
    BrushPlane pickedPlane(plane);
    BrushVec3 pickedPos(m_CurrentSpot.m_Pos);
    bool bSuccessQuery(false);
    CD::PolygonPtr pPickedPolygon;
    int nShelf = -1;

    ResetCurrentSpotWeakly();

    CD::GetLocalViewRay(worldTM, view, point, localRaySrc, localRayDir);

    if (!bSearchAllShelves)
    {
        if (bKeepInitialPlane)
        {
            bSuccessQuery = pModel->QueryPosition(pickedPlane, localRaySrc, localRayDir, pickedPos, NULL, &pPickedPolygon);
        }
        else
        {
            bSuccessQuery = pModel->QueryPosition(localRaySrc, localRayDir, pickedPos, &pickedPlane, NULL, &pPickedPolygon);
        }
    }
    else
    {
        CD::PolygonPtr pPolygons[2] = { NULL, NULL };
        bool bSuccessShelfQuery[2] = { false, false };
        MODEL_SHELF_RECONSTRUCTOR(pModel);
        for (int i = 0; i < 2; ++i)
        {
            pModel->SetShelf(i);
            if (bKeepInitialPlane)
            {
                bSuccessShelfQuery[i] = pModel->QueryPosition(pickedPlane, localRaySrc, localRayDir, pickedPos, NULL, &(pPolygons[i]));
            }
            else
            {
                bSuccessShelfQuery[i] = pModel->QueryPosition(localRaySrc, localRayDir, pickedPos, &pickedPlane, NULL, &(pPolygons[i]));
            }
        }

        BrushFloat ts[2] = { 3e10, 3e10 };
        for (int i = 0; i < 2; ++i)
        {
            if (pPolygons[i])
            {
                pPolygons[i]->IsPassed(localRaySrc, localRayDir, ts[i]);
            }
        }

        if (ts[0] < ts[1])
        {
            pPickedPolygon = pPolygons[0];
            bSuccessQuery = bSuccessShelfQuery[0];
            nShelf = 0;
        }
        else
        {
            pPickedPolygon = pPolygons[1];
            bSuccessQuery = bSuccessShelfQuery[1];
            nShelf = 1;
        }
    }

    if (pPickedPolygon && pPickedPolygon->CheckFlags(CD::ePolyFlag_Mirrored | CD::ePolyFlag_Hidden))
    {
        m_CurrentSpot.m_pPolygon = NULL;
        return false;
    }

    if (!bSuccessQuery || bKeepInitialPlane && !pPickedPolygon)
    {
        if (!bSuccessQuery)
        {
            if (!GetPosAndPlaneBasedOnWorld(view, point, worldTM, pickedPos, pickedPlane))
            {
                return false;
            }
        }
        m_CurrentSpot.m_pPolygon = NULL;
        m_CurrentSpot.m_Pos = pickedPos;
        m_CurrentSpot.m_PosState = eSpotPosState_OutsideDesigner;
        m_CurrentSpot.m_Plane = pickedPlane;

        SSpot niceSpot(m_CurrentSpot);
        std::vector<SCandidateInfo> candidates;
        if (GetSpotListCount() > 0)
        {
            candidates.push_back(SCandidateInfo(GetSpot(0).m_Pos, eSpotPosState_AtFirstSpot));
        }

        std::vector<CD::PolygonPtr> penetratedOpenPolygons;
        pModel->QueryOpenPolygons(localRaySrc, localRayDir, penetratedOpenPolygons);
        CD::PolygonPtr pConnectedPolygon = NULL;
        for (int i = 0, iCount(penetratedOpenPolygons.size()); i < iCount; ++i)
        {
            if (GetSpotListCount() > 0 && !GetSpot(0).m_Plane.IsEquivalent(penetratedOpenPolygons[i]->GetPlane()))
            {
                continue;
            }
            std::vector<CD::SVertex> vList;
            penetratedOpenPolygons[i]->GetLinkedVertices(vList);
            pConnectedPolygon = penetratedOpenPolygons[i];
            for (int k = 0, iVCount(vList.size()); k < iVCount; ++k)
            {
                if (k == 0)
                {
                    candidates.push_back(SCandidateInfo(vList[k].pos, eSpotPosState_FirstPointOfPolygon));
                }
                else if (k == iVCount - 1)
                {
                    candidates.push_back(SCandidateInfo(vList[k].pos, eSpotPosState_LastPointOfPolygon));
                }
                else
                {
                    candidates.push_back(SCandidateInfo(vList[k].pos, eSpotPosState_EitherPointOfEdge));
                }
                if (k < iVCount - 1)
                {
                    candidates.push_back(SCandidateInfo((vList[k].pos + vList[k + 1].pos) * 0.5f, eSpotPosState_CenterOfEdge));
                    BrushEdge3D edge(vList[k].pos, vList[k + 1].pos);
                    BrushVec3 posOnEdge;
                    bool bInEdge = false;
                    if (edge.GetNearestVertex(pickedPos, posOnEdge, bInEdge) && bInEdge && CD::AreTwoPositionsNear(pickedPos, posOnEdge, worldTM, view, kLimitForMagnetic))
                    {
                        if (!CD::IsEquivalent(posOnEdge, vList[k].pos) && !CD::IsEquivalent(posOnEdge, vList[k + 1].pos))
                        {
                            candidates.push_back(SCandidateInfo(posOnEdge, eSpotPosState_Edge));
                        }
                    }
                }
            }
        }
        if (!candidates.empty() && FindNicestSpot(view, candidates, pModel, worldTM, pickedPos, pConnectedPolygon, plane, niceSpot))
        {
            m_CurrentSpot = niceSpot;
        }

        if (!m_CurrentSpot.m_pPolygon)
        {
            m_CurrentSpot.m_Pos = Snap(m_CurrentSpot.m_Pos);
        }
        return true;
    }

    m_CurrentSpot.m_pPolygon = pPickedPolygon;

    bool bEnableSnap = IsSnapEnabled();
    if (!m_bEnableMagnetic && !bEnableSnap)
    {
        if (bSuccessQuery)
        {
            m_CurrentSpot.m_Pos = pickedPos;
            m_CurrentSpot.m_Plane = pickedPlane;
        }
        return true;
    }

    if (bEnableSnap && pPickedPolygon == NULL)
    {
        m_CurrentSpot.m_Plane = pickedPlane;
        m_CurrentSpot.m_Pos = Snap(pickedPos);
        m_CurrentSpot.m_pPolygon = NULL;
        return true;
    }

    BrushVec3 nearestEdge[2];
    BrushVec3 posOnEdge;
    std::vector<CD::SQueryEdgeResult> queryResults;
    std::vector<BrushEdge3D> queryEdges;

    {
        MODEL_SHELF_RECONSTRUCTOR(pModel);
        if (nShelf != -1)
        {
            pModel->SetShelf(nShelf);
        }
        bSuccessQuery = pModel->QueryNearestEdges(pickedPlane, localRaySrc, localRayDir, pickedPos, posOnEdge, queryResults);
    }

    if (bKeepInitialPlane && !bSuccessQuery)
    {
        if (!bEnableSnap)
        {
            m_CurrentSpot.m_Pos = pickedPos;
        }
        m_CurrentSpot.m_Plane = pickedPlane;
        return true;
    }

    if (!bSuccessQuery)
    {
        return false;
    }

    for (int i = 0, iQueryResultsCount(queryResults.size()); i < iQueryResultsCount; ++i)
    {
        queryEdges.push_back(queryResults[i].m_Edge);
    }

    if (m_bEnableMagnetic && CD::AreTwoPositionsNear(pickedPos, posOnEdge, worldTM, view, kLimitForMagnetic))
    {
        m_CurrentSpot.m_Pos = posOnEdge;
        m_CurrentSpot.m_PosState = eSpotPosState_Edge;
    }
    else if (!bEnableSnap)
    {
        m_CurrentSpot.m_Pos = pickedPos;
    }
    m_CurrentSpot.m_Plane = pickedPlane;

    int nShortestIndex = CD::FindShortestEdge(queryEdges);
    pPickedPolygon = queryResults[nShortestIndex].m_pPolygon;
    const BrushEdge3D& edge = queryEdges[nShortestIndex];

    SSpot niceSpot(m_CurrentSpot);
    std::vector<SCandidateInfo> candidates;
    candidates.push_back(SCandidateInfo(edge.m_v[0], eSpotPosState_EitherPointOfEdge));
    candidates.push_back(SCandidateInfo(edge.m_v[1], eSpotPosState_EitherPointOfEdge));
    candidates.push_back(SCandidateInfo((edge.m_v[0] + edge.m_v[1]) * 0.5f, eSpotPosState_CenterOfEdge));
    if (GetSpotListCount() > 0)
    {
        candidates.push_back(SCandidateInfo(GetSpot(0).m_Pos, eSpotPosState_AtFirstSpot));
    }
    BrushVec3 centerPolygonPos(m_CurrentSpot.m_Pos);
    {
        MODEL_SHELF_RECONSTRUCTOR(pModel);
        if (nShelf != -1)
        {
            pModel->SetShelf(nShelf);
        }
        if (pModel->QueryCenterOfPolygon(localRaySrc, localRayDir, centerPolygonPos))
        {
            candidates.push_back(SCandidateInfo(centerPolygonPos, eSpotPosState_CenterOfPolygon));
        }
    }
    if (FindNicestSpot(view, candidates, pModel, worldTM, pickedPos, pPickedPolygon, plane, niceSpot))
    {
        m_CurrentSpot = niceSpot;
    }

    SSpot snappedSpot;
    if (FindSnappedSpot(worldTM, m_CurrentSpot.m_pPolygon, pickedPos, snappedSpot))
    {
        m_CurrentSpot = snappedSpot;
        queryResults.clear();
        MODEL_SHELF_RECONSTRUCTOR(pModel);
        pModel->SetShelf(0);
        pModel->QueryNearestEdges(pickedPlane, m_CurrentSpot.m_Pos, posOnEdge, queryResults);
        if (CD::IsEquivalent(posOnEdge, m_CurrentSpot.m_Pos))
        {
            m_CurrentSpot.m_PosState = eSpotPosState_Edge;
        }
        else if (bSearchAllShelves)
        {
            pModel->SetShelf(1);
            pModel->QueryNearestEdges(pickedPlane, m_CurrentSpot.m_Pos, posOnEdge, queryResults);
            if (CD::IsEquivalent(posOnEdge, m_CurrentSpot.m_Pos))
            {
                m_CurrentSpot.m_PosState = eSpotPosState_Edge;
            }
        }
        return true;
    }

    SSpot nearSpot(m_CurrentSpot);
    if (FindSpotNearAxisAlignedLine(view, pPickedPolygon, worldTM, nearSpot))
    {
        m_CurrentSpot = nearSpot;
    }

    return bSuccessQuery;
}

void SpotManager::AddSpotToSpotList(const SSpot& spot)
{
    bool bExistSameSpot(false);
    for (int i = 0, iSpotSize(GetSpotListCount()); i < iSpotSize; ++i)
    {
        if (GetSpot(i).IsEquivalentPos(spot) && !GetSpot(i).m_bProcessed)
        {
            bExistSameSpot = true;
            break;
        }
    }

    if (!bExistSameSpot)
    {
        m_SpotList.push_back(spot);
    }
}

void SpotManager::ReplaceSpotList(const SpotList& spotList)
{
    m_SpotList = spotList;
}

SpotManager::SSpot SpotManager::Convert2Spot(CD::Model* pModel, const BrushVec3& pos) const
{
    SSpot spot(pos);

    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        CD::PolygonPtr pPolygon = pModel->GetPolygon(i);
        if (!pPolygon)
        {
            continue;
        }

        if (pPolygon->HasVertex(pos))
        {
            spot.m_PosState = eSpotPosState_EitherPointOfEdge;
            spot.m_pPolygon = pPolygon;
        }
        else if (pPolygon->IsVertexOnCrust(pos))
        {
            spot.m_PosState = eSpotPosState_Edge;
            spot.m_pPolygon = pPolygon;
        }
    }

    return spot;
}

bool SpotManager::GetPlaneBeginEndPoints(const BrushPlane& plane, BrushVec2& outProjectedStartPt, BrushVec2& outProjectedEndPt) const
{
    outProjectedStartPt = plane.W2P(GetStartSpotPos());
    outProjectedEndPt = plane.W2P(GetCurrentSpotPos());

    const BrushFloat SmallestPolygonSize(0.01f);
    if (std::abs(outProjectedEndPt.x - outProjectedStartPt.x) < SmallestPolygonSize || std::abs(outProjectedEndPt.y - outProjectedStartPt.y) < SmallestPolygonSize)
    {
        return false;
    }

    const BrushVec2 vecMinimum(-1000.0f, -1000.0f);
    const BrushVec2 vecMaximum(1000.0f, 1000.0f);

    outProjectedStartPt.x = std::max(outProjectedStartPt.x, vecMinimum.x);
    outProjectedStartPt.y = std::max(outProjectedStartPt.y, vecMinimum.y);
    outProjectedEndPt.x = std::min(outProjectedEndPt.x, vecMaximum.x);
    outProjectedEndPt.y = std::min(outProjectedEndPt.y, vecMaximum.y);

    return true;
}

bool SpotManager::GetPosAndPlaneBasedOnWorld(IDisplayViewport* view, const QPoint& point, const BrushMatrix34& worldTM, BrushVec3& outPos, BrushPlane& outPlane)
{
    Vec3 vPickedPos;
    if (!CD::PickPosFromWorld(view, point, vPickedPos))
    {
        return false;
    }
    Matrix34 invTM = worldTM.GetInverted();
    Vec3 p0 = invTM.TransformPoint(vPickedPos);
    outPlane = BrushPlane(BrushVec3(0, 0, 1), -p0.Dot(BrushVec3(0, 0, 1)));
    outPos = CD::ToBrushVec3(p0);
    return true;
}

bool SpotManager::FindBestPlane(CD::Model* pModel, const SSpot& s0, const SSpot& s1, BrushPlane& outPlane)
{
    if (s0.m_pPolygon && s0.m_pPolygon == s1.m_pPolygon)
    {
        outPlane = s0.m_pPolygon->GetPlane();
        return true;
    }

    BrushPlane candidatePlanes[] = { s0.m_Plane, s1.m_Plane };
    for (int i = 0, iCandidatePlaneCount(sizeof(candidatePlanes) / sizeof(*candidatePlanes)); i < iCandidatePlaneCount; ++i)
    {
        if (candidatePlanes[i].Normal().IsZero(kDesignerEpsilon))
        {
            continue;
        }
        BrushFloat d0 = candidatePlanes[i].Distance(s0.m_Pos);
        BrushFloat d1 = candidatePlanes[i].Distance(s1.m_Pos);
        if (std::abs(d0) < kDesignerEpsilon && std::abs(d1) < kDesignerEpsilon)
        {
            outPlane = candidatePlanes[i];
            return true;
        }
    }

    for (int i = 0, iPolygonCount(pModel->GetPolygonCount()); i < iPolygonCount; ++i)
    {
        const BrushPlane& plane = pModel->GetPolygon(i)->GetPlane();
        BrushFloat d0 = plane.Distance(s0.m_Pos);
        BrushFloat d1 = plane.Distance(s1.m_Pos);
        if (std::abs(d0) < kDesignerEpsilon && std::abs(d1) < kDesignerEpsilon)
        {
            outPlane = plane;
            return true;
        }
    }

    DESIGNER_ASSERT(0);
    return false;
}

bool SpotManager::IsSnapEnabled() const
{
    if (!m_bBuiltInSnap)
    {
        return GetIEditor()->GetViewManager()->GetGrid()->IsEnabled() | m_bBuiltInSnap;
    }
    return true;
}

BrushVec3 SpotManager::Snap(const BrushVec3& vPos) const
{
    if (!m_bBuiltInSnap)
    {
        return GetIEditor()->GetViewManager()->GetGrid()->Snap(vPos);
    }

    BrushVec3 snapped;
    snapped.x = std::floor((vPos.x / m_BuiltInSnapSize) + (BrushFloat)0.5) * m_BuiltInSnapSize;
    snapped.y = std::floor((vPos.y / m_BuiltInSnapSize) + (BrushFloat)0.5) * m_BuiltInSnapSize;
    snapped.z = std::floor((vPos.z / m_BuiltInSnapSize) + (BrushFloat)0.5) * m_BuiltInSnapSize;

    return snapped;
}