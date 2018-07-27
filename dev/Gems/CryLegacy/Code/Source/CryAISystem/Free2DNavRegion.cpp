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

#include "CryLegacy_precompiled.h"
#include "Free2DNavRegion.h"
#include "CAISystem.h"
#include "AILog.h"

//===================================================================
// CFree2DNavRegion
//===================================================================
CFree2DNavRegion::CFree2DNavRegion(CGraph* pGraph)
{
    m_pDummyNode = 0;
    m_dummyNodeIndex = 0;
}
//===================================================================
// ~CFree2DNavRegion
//===================================================================
CFree2DNavRegion::~CFree2DNavRegion()
{
}

//===================================================================
// BeautifyPath
//===================================================================
void CFree2DNavRegion::BeautifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath,
    const Vec3& startPos, const Vec3& startDir,
    const Vec3& endPos, const Vec3& endDir,
    float radius,
    const AgentMovementAbility& movementAbility,
    const NavigationBlockers& navigationBlockers)
{
    AIWarning("CFree2DNavRegion::BeautifyPath should never be called");
}

//===================================================================
// UglifyPath
//===================================================================
void CFree2DNavRegion::UglifyPath(const VectorConstNodeIndices& inPath, TPathPoints& outPath,
    const Vec3& startPos, const Vec3& startDir,
    const Vec3& endPos, const Vec3& endDir)
{
    AIWarning("CFree2DNavRegion::UglifyPath should never be called");
}

//===================================================================
// GetEnclosing
//===================================================================
unsigned CFree2DNavRegion::GetEnclosing(const Vec3& pos, float passRadius, unsigned startIndex,
    float /*range*/, Vec3* closestValid, bool returnSuspect, const char* requesterName, bool omitWalkabilityTest)
{
    /*
      const SpecialArea *sa = GetAISystem()->GetSpecialArea(pos, SpecialArea::TYPE_FREE_2D);
      if (sa)
        return 0;
      int nBuildingID = sa->nBuildingID;
      if (nBuildingID < 0)
        return 0;
    */
    if (!m_pDummyNode)
    {
        m_dummyNodeIndex = gAIEnv.pGraph->CreateNewNode(IAISystem::NAV_FREE_2D, ZERO);
        m_pDummyNode = gAIEnv.pGraph->GetNode(m_dummyNodeIndex);
    }

    return m_dummyNodeIndex;
}

//===================================================================
// Clear
//===================================================================
void CFree2DNavRegion::Clear()
{
    if (m_pDummyNode)
    {
        gAIEnv.pGraph->Disconnect(m_dummyNodeIndex);
        m_pDummyNode = 0;
        m_dummyNodeIndex = 0;
    }
}

//===================================================================
// MemStats
//===================================================================
size_t CFree2DNavRegion::MemStats()
{
    size_t size = sizeof(*this);
    return size;
}

//===================================================================
// CheckPassability
//===================================================================
bool CFree2DNavRegion::CheckPassability(const Vec3& from, const Vec3& to,
    float radius, const NavigationBlockers& navigationBlockers, IAISystem::tNavCapMask) const
{
    const SpecialArea* sa = gAIEnv.pNavigation->GetSpecialArea(from, SpecialArea::TYPE_FREE_2D);
    if (!sa)
    {
        return false;
    }
    return !Overlap::Lineseg_Polygon2D(Lineseg(from, to), sa->GetPolygon(), &sa->GetAABB());
}

//===================================================================
// ExpandShape
//===================================================================
static void ShrinkShape(const ListPositions& shapeIn, ListPositions& shapeOut, float radius)
{
    // push the points in. The shape is guaranteed to be wound anti-clockwise
    shapeOut.clear();
    for (ListPositions::const_iterator it = shapeIn.begin(); it != shapeIn.end(); ++it)
    {
        ListPositions::const_iterator itNext = it;
        if (++itNext == shapeIn.end())
        {
            itNext = shapeIn.begin();
        }
        ListPositions::const_iterator itNextNext = itNext;
        if (++itNextNext == shapeIn.end())
        {
            itNextNext = shapeIn.begin();
        }

        Vec3 pos = *it;
        Vec3 posNext = *itNext;
        Vec3 posNextNext = *itNextNext;
        pos.z = posNext.z = posNextNext.z = 0.0f;

        Vec3 segDirPrev = (posNext - pos).GetNormalizedSafe();
        Vec3 segDirNext = (posNextNext - posNext).GetNormalizedSafe();

        Vec3 normalInPrev(-segDirPrev.y, segDirPrev.x, 0.0f);
        Vec3 normalInNext(-segDirNext.y, segDirNext.x, 0.0f);
        Vec3 normalAv = (normalInPrev + normalInNext).GetNormalizedSafe();

        Vec3 cross = segDirPrev.Cross(segDirNext);
        // convex means the point is convex from the point of view of inside the shape
        bool convex = cross.z < 0.0f;

        static float radiusScale = 0.5f;
        if (convex)
        {
            Vec3 newPtPrev = posNext + normalInPrev * (radius * radiusScale);
            newPtPrev.z = it->z;
            Vec3 newPtMid = posNext + normalAv * (radius * radiusScale);
            newPtMid.z = itNext->z;
            Vec3 newPtNext = posNext + normalInNext * (radius * radiusScale);
            newPtNext.z = itNextNext->z;
            shapeOut.push_back(newPtPrev);
            shapeOut.push_back(newPtMid);
            shapeOut.push_back(newPtNext);
        }
        else
        {
            float dot = segDirPrev.Dot(segDirNext);
            float extraRadiusScale = sqrtf(2.0f / (1.0f + dot));
            Vec3 newPtMid = posNext + normalAv * (radius * radiusScale * extraRadiusScale);
            newPtMid.z = itNext->z;
            shapeOut.push_back(newPtMid);
        }
    }
}

//===================================================================
// GetSingleNodePath
// Danny todo This "works" but doesn't generate ideal paths because it hugs
// a single wall - it can't cross over to hug the other wall. Should
// be OK for most sensible areas though. Also there is a danger that
// the path can get close to the area edge, but I don't know what can
// be done to stop that. Also, the shape shrinking could/should
// be precalculated.
//===================================================================
bool CFree2DNavRegion::GetSingleNodePath(const GraphNode* pNode,
    const Vec3& startPos, const Vec3& endPos, float radius,
    const NavigationBlockers& navigationBlockers,
    std::vector<PathPointDescriptor>& points,
    IAISystem::tNavCapMask) const
{
    const SpecialArea* sa = gAIEnv.pNavigation->GetSpecialAreaNearestPos(startPos, SpecialArea::TYPE_FREE_2D);
    if (!sa)
    {
        return false;
    }

    const ListPositions& origShape = sa->GetPolygon();
    ListPositions shape;
    ShrinkShape(origShape, shape, radius);

    // simplest straight-line case
    if (!Overlap::Lineseg_Polygon2D(Lineseg(startPos, endPos), shape))
    {
        points.resize(0);
        points.push_back(PathPointDescriptor(IAISystem::NAV_FREE_2D, startPos));
        points.push_back(PathPointDescriptor(IAISystem::NAV_FREE_2D, endPos));
        return true;
    }

    if (!Overlap::Point_Polygon2D(endPos, origShape, &sa->GetAABB()))
    {
        return false;
    }

    // So, now start and end are in the same area. make a "safe" path
    // by going from startPos to the nearest vertex, then through all
    // vertices to the vertex nearest to endPos, then to endPos. Subsequently
    // we'll tidy this up by cutting vertices.

    ListPositions::const_iterator itStart, itEnd;
    float bestDistStartSq = std::numeric_limits<float>::max();
    float bestDistEndSq = std::numeric_limits<float>::max();
    int countToStart = -1;
    int countToEnd = -1;
    int count = 0;
    for (ListPositions::const_iterator it = shape.begin(); it != shape.end(); ++it, ++count)
    {
        Vec3 itPos = *it;
        static float frac = 0.99f;
        bool reachableStart = !Overlap::Lineseg_Polygon2D(Lineseg(startPos, startPos + frac * (itPos - startPos)), origShape, &sa->GetAABB());
        bool reachableEnd   = !Overlap::Lineseg_Polygon2D(Lineseg(endPos, endPos + frac * (itPos - endPos)), origShape, &sa->GetAABB());
        if (reachableStart)
        {
            float distToStartSq = Distance::Point_Point2DSq(startPos, itPos);
            if (distToStartSq < bestDistStartSq)
            {
                bestDistStartSq = distToStartSq;
                itStart = it;
                countToStart = count;
            }
        }
        if (reachableEnd)
        {
            float distToEndSq = Distance::Point_Point2DSq(endPos, itPos);
            if (distToEndSq < bestDistEndSq)
            {
                bestDistEndSq = distToEndSq;
                itEnd = it;
                countToEnd = count;
            }
        }
    }

    if (countToStart < 0 || countToEnd < 0)
    {
        AIWarning("CFree2DNavRegion::GetSingleNodePath failed from (%5.2f, %5.2f, %5.2f) to (%5.2f, %5.2f, %5.2f)",
            startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);
        return false;
    }

    // add the points
    points.resize(0);
    points.push_back(PathPointDescriptor(IAISystem::NAV_FREE_2D, startPos));
    bool walkingFwd = true;
    if (itStart == itEnd)
    {
        points.push_back(PathPointDescriptor(IAISystem::NAV_FREE_2D, *itStart));
    }
    else
    {
        // decide on the direction using the counts...
        int shapeCount = shape.size();
        int countFwd = (shapeCount + countToEnd - countToStart) % shapeCount;
        int countBwd = shapeCount - countFwd;
        walkingFwd = countFwd < countBwd;

        ListPositions::const_iterator it = itStart;
        do
        {
            points.push_back(PathPointDescriptor(IAISystem::NAV_FREE_2D, *it));
            if (walkingFwd)
            {
                ++it;
                if (it == shape.end())
                {
                    it = shape.begin();
                }
            }
            else
            {
                if (it == shape.begin())
                {
                    it = shape.end();
                }
                --it;
            }
        } while (it != itEnd);
        points.push_back(PathPointDescriptor(IAISystem::NAV_FREE_2D, *itEnd));
    }
    points.push_back(PathPointDescriptor(IAISystem::NAV_FREE_2D, endPos));

    // now walk through iteratively cutting points
    static bool doCutting = true;
    bool cutOne = doCutting;
    while (cutOne == true && points.size() > 2)
    {
        cutOne = false;
        for (std::vector<PathPointDescriptor>::iterator it = points.begin(); it != points.end(); ++it)
        {
            std::vector<PathPointDescriptor>::iterator itNext = it;
            if (++itNext == points.end())
            {
                break;
            }
            std::vector<PathPointDescriptor>::iterator itNextNext = itNext;
            if (++itNextNext == points.end())
            {
                break;
            }

            Vec3 pos = it->vPos;
            //      Vec3 posNext = itNext->vPos;
            Vec3 posNextNext = itNextNext->vPos;

            static float frac = 0.001f;
            pos += frac * (posNextNext - pos);
            posNextNext += frac * (pos - posNextNext);

            Lineseg seg(pos, posNextNext);
            Vec3 posMid = 0.5f * (pos + posNextNext);
            std::vector<PathPointDescriptor>::iterator itNextNextNext = itNextNext;
            ++itNextNextNext;
            bool doingEnd = it == points.begin() || itNextNextNext == points.end();
            const ListPositions& thisShape = doingEnd ? origShape : shape;
            if (Overlap::Point_Polygon2D(pos, thisShape) && !Overlap::Lineseg_Polygon2D(seg, thisShape))
            {
                points.erase(itNext);
                cutOne = true;
                // iterator it is safe because itNext comes after it
                it = points.begin();
            }
        }
    }

    return true;
}
