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
#include "ExtrusionSnappingHelper.h"
#include "ViewManager.h"

namespace CD
{
    ExtrusionSnappingHelper s_SnappingHelper;
}

void ExtrusionSnappingHelper::SearchForOppositePolygons(CD::PolygonPtr pCapPolygon)
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());
    GetModel()->SetShelf(0);

    for (int i = 0; i < 2; ++i)
    {
        // i == 0 --> Query push direction polygon
        // i == 1 --> Query pull direction polygon
        m_Opposites[i].Init();

        SOpposite opposite;
        SOpposite oppositeAdjancent;

        CD::EFindOppositeFlag nFlag = i == 0 ? CD::eFOF_PushDirection : CD::eFOF_PullDirection;
        BrushFloat fScale = kDesignerEpsilon * 10.0f;

        opposite.bTouchAdjacent = false;
        opposite.polygonRelation = GetModel()->QueryOppositePolygon(
                pCapPolygon,
                nFlag,
                0,
                opposite.polygon,
                opposite.nearestDistanceToPolygon);

        oppositeAdjancent.bTouchAdjacent = true;
        oppositeAdjancent.polygonRelation = GetModel()->QueryOppositePolygon(
                pCapPolygon,
                nFlag,
                fScale,
                oppositeAdjancent.polygon,
                oppositeAdjancent.nearestDistanceToPolygon);

        if (opposite.polygonRelation != CD::eER_None && oppositeAdjancent.polygonRelation != CD::eER_None)
        {
            if (opposite.nearestDistanceToPolygon - oppositeAdjancent.nearestDistanceToPolygon <= kDesignerEpsilon)
            {
                m_Opposites[i] = opposite;
            }
            else
            {
                m_Opposites[i] = oppositeAdjancent;
            }
        }
        else if (opposite.polygonRelation != CD::eER_None && oppositeAdjancent.polygonRelation == CD::eER_None)
        {
            m_Opposites[i] = opposite;
        }
        else if (opposite.polygonRelation == CD::eER_None && oppositeAdjancent.polygonRelation != CD::eER_None)
        {
            m_Opposites[i] = oppositeAdjancent;
        }
    }
}

bool ExtrusionSnappingHelper::IsOverOppositePolygon(const CD::PolygonPtr& pPolygon, CD::EPushPull pushpull, bool bIgnoreSimilarDirection)
{
    if (m_Opposites[pushpull].polygon)
    {
        if (bIgnoreSimilarDirection)
        {
            if (m_Opposites[pushpull].polygon->GetPlane().IsSameFacing(pPolygon->GetPlane()))
            {
                return false;
            }
        }

        BrushVec3 direction = -pPolygon->GetPlane().Normal();
        BrushFloat fNearestDistance = pPolygon->GetNearestDistance(m_Opposites[pushpull].polygon, direction);

        if (pushpull == CD::ePP_Push)
        {
            return fNearestDistance > -kDesignerEpsilon;
        }
        else
        {
            return fNearestDistance < kDesignerEpsilon;
        }
    }

    return false;
}

void ExtrusionSnappingHelper::ApplyOppositePolygons(CD::PolygonPtr pCapPolygon, CD::EPushPull pushpull, bool bHandleTouch)
{
    MODEL_SHELF_RECONSTRUCTOR(GetModel());

    if (m_Opposites[pushpull].polygon && m_Opposites[pushpull].polygonRelation == CD::eER_Intersection)
    {
        if (m_Opposites[pushpull].bTouchAdjacent && bHandleTouch)
        {
            GetModel()->SetShelf(0);
            GetModel()->AddPolygon(pCapPolygon, CD::eOpType_Union);
            GetModel()->SetShelf(1);
            GetModel()->RemovePolygon(pCapPolygon);
        }
        else if (!m_Opposites[pushpull].bTouchAdjacent)
        {
            pCapPolygon->Flip();
            GetModel()->SetShelf(0);

            std::vector<CD::PolygonPtr> intersetionPolygons;
            GetModel()->QueryIntersectionByPolygon(pCapPolygon, intersetionPolygons);

            CD::PolygonPtr ABInterectPolygon = pCapPolygon->Clone();
            CD::PolygonPtr ABSubtractPolygon = pCapPolygon->Clone();
            CD::PolygonPtr pUnionPolygon = new CD::Polygon;

            for (int i = 0, iIntersectionSize(intersetionPolygons.size()); i < iIntersectionSize; ++i)
            {
                pUnionPolygon->Union(intersetionPolygons[i]);
            }

            ABInterectPolygon->Intersect(pUnionPolygon, CD::eICEII_IncludeCoSame);
            ABSubtractPolygon->Subtract(pUnionPolygon);

            if (ABInterectPolygon->IsValid() && !ABInterectPolygon->IsOpen())
            {
                GetModel()->AddPolygon(ABInterectPolygon, CD::eOpType_SubtractAB);
            }

            if (ABSubtractPolygon->IsValid() && !ABSubtractPolygon->IsOpen())
            {
                GetModel()->AddPolygon(ABSubtractPolygon->Flip(), CD::eOpType_Add);
            }

            GetModel()->SetShelf(1);
            pCapPolygon->Flip();
            std::vector<CD::PolygonPtr> polygonList;
            GetModel()->QueryPolygons(pCapPolygon->GetPlane(), polygonList);
            for (int i = 0, iPolygonCount(polygonList.size()); i < iPolygonCount; ++i)
            {
                GetModel()->DrillPolygon(polygonList[i]);
            }
        }
    }

    m_Opposites[0].polygon = NULL;
    m_Opposites[1].polygon = NULL;
}

BrushFloat ExtrusionSnappingHelper::GetNearestDistanceToOpposite(CD::EPushPull pushpull) const
{
    BrushFloat nearestDistance = m_Opposites[pushpull].nearestDistanceToPolygon;
    if (pushpull == CD::ePP_Push)
    {
        nearestDistance = -nearestDistance;
    }
    return nearestDistance;
}

CD::PolygonPtr ExtrusionSnappingHelper::FindAlignedPolygon(CD::PolygonPtr pCapPolygon, const BrushMatrix34& worldTM, CViewport* view, const QPoint& point) const
{
    if (pCapPolygon == NULL)
    {
        return NULL;
    }
    BrushVec3 localRaySrc;
    BrushVec3 localRayDir;
    CD::GetLocalViewRay(worldTM, view, point, localRaySrc, localRayDir);
    int nIndex(-1);
    if (!GetModel()->QueryPolygon(localRaySrc, localRayDir, nIndex))
    {
        return NULL;
    }

    const CD::PolygonPtr pPolygon = GetModel()->GetPolygon(nIndex);
    if (pPolygon->GetPlane().Normal().IsEquivalent(pCapPolygon->GetPlane().Normal()))
    {
        return pPolygon;
    }

    return NULL;
}