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
#include "TriangularNavRegion.h"
#include "AICollision.h"
#include "ISystem.h"
#include "I3DEngine.h"
#include "ISerialize.h"
#include "IConsole.h"
#include "IRenderer.h"
#include "Navigation.h"

#include <limits>

#define PI 3.141592653589793f

//====================================================================
// CTriangularNavRegion
//====================================================================
CTriangularNavRegion::CTriangularNavRegion(CGraph* pGraph, CVertexList* pVertexList)
    : m_pVertexList(pVertexList)
{
    AIAssert(pGraph);
    m_pGraph = pGraph;
}

//====================================================================
// SimplifiedLink
//====================================================================
class SimplifiedLink
{
public:
    Vec3  left;
    Vec3  right;
    SimplifiedLink(const Vec3& _left, const Vec3& _right)
        : left(_left)
        , right(_right) {}
    SimplifiedLink() {}
};

typedef std::vector< SimplifiedLink > tLinks;
static tLinks debugLinks;

//====================================================================
// GreedyStep1
// iterative function to quickly converge on the target position in the graph
//====================================================================
unsigned CTriangularNavRegion::GreedyStep1(unsigned beginIndex, const Vec3& pos)
{
    GraphNode* pBegin = m_pGraph->GetNodeManager().GetNode(beginIndex);
    AIAssert(pBegin && pBegin->navType == IAISystem::NAV_TRIANGULAR);

    if (m_pGraph->PointInTriangle(pos, pBegin))
    {
        return beginIndex;  // we have arrived
    }
    m_pGraph->MarkNode(beginIndex);

    Vec3r triPos = pBegin->GetPos();
    for (unsigned link = pBegin->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
    {
        unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
        GraphNode* pNextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);

        if (pNextNode->mark || pNextNode->navType != IAISystem::NAV_TRIANGULAR)
        {
            continue;
        }

        // cross to the next triangle if this edge splits space such that the current triangle and the
        // destination are on opposite sides.
        const ObstacleData& startObst = m_pVertexList->GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetStartIndex(link)]);
        const ObstacleData& endObst = m_pVertexList->GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetEndIndex(link)]);
        Vec3r edgeStart = startObst.vPos;
        Vec3r edgeEnd = endObst.vPos;
        Vec3r edgeDelta = edgeEnd - edgeStart;
        Vec3r edgeNormal(edgeDelta.y, -edgeDelta.x, 0.0f); // could point either way

        real dotDest = edgeNormal.Dot(pos - edgeStart);
        real dotTri = edgeNormal.Dot(triPos - edgeStart);
        if (sgn(dotDest) != sgn(dotTri))
        {
            return nextNodeIndex;
        }
    }
    return 0;
}

//===================================================================
// GreedyStep2
//===================================================================
unsigned CTriangularNavRegion::GreedyStep2(unsigned beginIndex, const Vec3& pos)
{
    GraphNode* pBegin = m_pGraph->GetNodeManager().GetNode(beginIndex);
    AIAssert(pBegin && pBegin->navType == IAISystem::NAV_TRIANGULAR);

    if (m_pGraph->PointInTriangle(pos, pBegin))
    {
        return beginIndex;  // we have arrived
    }
    m_pGraph->MarkNode(beginIndex);

    real bestDot = -1.0;
    unsigned bestLink = 0;

    Vec3r dirToPos = pos - pBegin->GetPos();
    dirToPos.z = 0.0f;
    dirToPos.NormalizeSafe();

    for (unsigned link = pBegin->firstLinkIndex; link; link = m_pGraph->GetLinkManager().GetNextLink(link))
    {
        unsigned nextNodeIndex = m_pGraph->GetLinkManager().GetNextNode(link);
        const GraphNode* pNextNode = m_pGraph->GetNodeManager().GetNode(nextNodeIndex);

        if (pNextNode->mark || pNextNode->navType != IAISystem::NAV_TRIANGULAR)
        {
            continue;
        }

        const ObstacleData& startObst = m_pVertexList->GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetStartIndex(link)]);
        const ObstacleData& endObst = m_pVertexList->GetVertex(pBegin->GetTriangularNavData()->vertices[m_pGraph->GetLinkManager().GetEndIndex(link)]);
        Vec3r edgeStart = startObst.vPos;
        Vec3r edgeEnd = endObst.vPos;
        Vec3r edgeDelta = (edgeEnd - edgeStart);
        Vec3r edgeNormal(edgeDelta.y, -edgeDelta.x, 0.0f); // points out - assumes correct winding
        edgeNormal.NormalizeSafe();
        if (edgeNormal.Dot(edgeStart - pBegin->GetPos()) < 0.0f)
        {
            edgeNormal *= -1.0f;
        }
        else
        {
            int junk = 0;
        }

        real thisDot = dirToPos.Dot(edgeNormal);
        if (thisDot > bestDot)
        {
            bestLink = link;
            bestDot = thisDot;
        }
    }

    if (bestLink)
    {
        return m_pGraph->GetLinkManager().GetNextNode(bestLink);
    }
    else
    {
        return 0;
    }
}

//====================================================================
// GetGoodPositionInTriangle
//====================================================================
Vec3 CTriangularNavRegion::GetGoodPositionInTriangle(const GraphNode* pNode, const Vec3& pos) const
{
    Vec3 newPos(pos);
    for (unsigned iVertex = 0; iVertex < pNode->GetTriangularNavData()->vertices.size(); ++iVertex)
    {
        int obIndex = pNode->GetTriangularNavData()->vertices[iVertex];
        const ObstacleData& od = m_pVertexList->GetVertex(obIndex);
        float distToObSq = Distance::Point_Point2DSq(newPos, od.vPos);
        if (distToObSq > 0.0001f && distToObSq < square(max(od.fApproxRadius, 0.0f)))
        {
            Vec3 dirToMove = newPos - od.vPos;
            dirToMove.z = 0.0f;
            dirToMove.Normalize();
            newPos = od.vPos + max(od.fApproxRadius, 0.0f) * dirToMove;
        }
    }
    return newPos;
}


//====================================================================
// GetEnclosing
//====================================================================
unsigned CTriangularNavRegion::GetEnclosing(const Vec3& pos, float passRadius, unsigned startIndex,
    float /*range*/, Vec3* closestValid, bool returnSuspect, const char* requesterName)
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_AI);

    if (!m_pGraph->InsideOfBBox(pos))
    {
        AIWarning("CTriangularNavRegion::GetEnclosing at position: (%5.2f %5.2f %5.2f) %s is outside of navigation bounding box",
            pos.x, pos.y, pos.z, requesterName);
        return 0;
    }

    m_pGraph->ClearMarks();

    if (!startIndex || (m_pGraph->GetNodeManager().GetNode(startIndex)->navType != IAISystem::NAV_TRIANGULAR))
    {
        typedef std::vector< std::pair<float, unsigned> > TNodes;
        static TNodes nodes;
        nodes.resize(0);
        // Most times we'll find the desired node close by
        CAllNodesContainer& allNodes = m_pGraph->GetAllNodes();
        allNodes.GetAllNodesWithinRange(nodes, pos, 15.0f, IAISystem::NAV_TRIANGULAR);
        if (!nodes.empty())
        {
            startIndex = nodes[0].second;
        }
        else
        {
            CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR);
            startIndex = it.Increment();
            if (!startIndex)
            {
                AIWarning("CTriangularNavRegion::GetEnclosing No triangular nodes in navigation");
                return 0;
            }
        }
    }

    AIAssert(startIndex && m_pGraph->GetNodeManager().GetNode(startIndex)->navType == IAISystem::NAV_TRIANGULAR);

    unsigned nextNodeIndex = startIndex;
    unsigned prevNodeIndex = 0;
    int iterations = 0;
    while (nextNodeIndex && prevNodeIndex != nextNodeIndex)
    {
        ++iterations;
        prevNodeIndex = nextNodeIndex;
        nextNodeIndex = GreedyStep1(prevNodeIndex, pos);
    }
    m_pGraph->ClearMarks();
    // plan B - should not get called very much
    if (!nextNodeIndex)
    {
        AILogProgress("CTriangularNavRegion::GetEnclosing (%5.2f %5.2f %5.2f %s radius = %5.2f) Resorting to slower algorithm",
            pos.x, pos.y, pos.z, requesterName, passRadius);
        static int planBTimes = 0;
        ++planBTimes;
        nextNodeIndex = startIndex;
        prevNodeIndex = 0;
        iterations = 0;
        while (nextNodeIndex && prevNodeIndex != nextNodeIndex)
        {
            ++iterations;
            prevNodeIndex = nextNodeIndex;
            nextNodeIndex = GreedyStep2(prevNodeIndex, pos);
        }
        m_pGraph->ClearMarks();
    }
    // plan C - bad news if this gets called
    if (!nextNodeIndex)
    {
        AILogProgress("CTriangularNavRegion::GetEnclosing (%5.2f %5.2f %5.2f %s radius = %5.2f) Resorting to brute-force algorithm",
            pos.x, pos.y, pos.z, requesterName, passRadius);
        CAllNodesContainer& allNodes = m_pGraph->GetAllNodes();
        CAllNodesContainer::Iterator it(allNodes, IAISystem::NAV_TRIANGULAR);
        while (nextNodeIndex = it.Increment())
        {
            if (m_pGraph->PointInTriangle(pos, m_pGraph->GetNodeManager().GetNode(nextNodeIndex)))
            {
                break;
            }
            else
            {
                nextNodeIndex = 0;
            }
        }
    }

    if (nextNodeIndex && m_pGraph->PointInTriangle(pos, m_pGraph->GetNodeManager().GetNode(nextNodeIndex)))
    {
        if (closestValid)
        {
            *closestValid = GetGoodPositionInTriangle(m_pGraph->GetNodeManager().GetNode(nextNodeIndex), pos);
        }
        return nextNodeIndex; // or pPrevNode, they are the same
    }
    else
    {
        // silently fail if there's no navigation at all
        if (nextNodeIndex == m_pGraph->m_safeFirstIndex)
        {
            return 0;
        }
        AIWarning("CTriangularNavRegion::GetEnclosing DebugWalk bad result from looking for node containing %s position: (%5.2f, %5.2f, %5.2f)",
            requesterName, pos.x, pos.y, pos.z);
        return 0;
    }
}

//====================================================================
// Serialize
//====================================================================
void CTriangularNavRegion::Serialize(TSerialize ser)
{
    ser.BeginGroup("TriangularNavRegion");

    ser.EndGroup();
}

//====================================================================
// Reset
//====================================================================
void CTriangularNavRegion::Clear()
{
}

//====================================================================
// Reset
//====================================================================
void CTriangularNavRegion::Reset(IAISystem::EResetReason reason)
{
}

