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
#include "Graph.h"
#include "AILog.h"

#include <ISystem.h>

#include <IRenderer.h>
#include <ILog.h>
#include <I3DEngine.h>
#include <algorithm>
#include <IConsole.h>
#include "IPhysics.h"
#include "Navigation.h"


//====================================================================
// Rearrange
//====================================================================
int CGraph::Rearrange(const string& areaName, ListNodeIds& nodesList, const Vec3r& cutStart, const Vec3r& cutEnd)
{
    int counter = 0;
    GraphNode* curNode;
    ListNodeIds::iterator it;

    for (it = nodesList.begin(); it != nodesList.end(); ++it)
    {
        curNode = GetNodeManager().GetNode(*it);
        //      VectorOfLinks::iterator vi;
        if (curNode->GetTriangularNavData()->vertices.size() != 3 || curNode == m_pSafeFirst)
        {
            nodesList.erase(it);
            it = nodesList.begin();
        }
    }

    counter = ProcessMegaMerge(areaName, nodesList, cutStart, cutEnd);
    return counter;
}

//====================================================================
// ProcessMegaMerge
//====================================================================
int CGraph::ProcessMegaMerge(const string& areaName, ListNodeIds& nodesList, const Vec3r& vCutStart, const Vec3r& vCutEnd)
{
    ListNodeIds   leftSideNodes;
    ListNodeIds   rightSideNodes;
    int               Counter = 0;
    Vec3r         cutDirection = vCutEnd - vCutStart;
    ObstacleIndexList leftOutline;
    ObstacleIndexList rightOutline;
    GraphNode* curNode;

    const CVertexList& vertexList = m_pNavigation->GetVertexList();
    ObstacleData  tmpObst;
    tmpObst.vPos = vCutEnd;
    int   cutEnd = vertexList.FindVertex(tmpObst);
    tmpObst.vPos = vCutStart;
    int   cutStart = vertexList.FindVertex(tmpObst);


    if (cutStart < 0 || cutEnd < 0)
    {
        AIWarning("CGraph::ProcessMegaMerge  cut is not in m_VertexList %d %d", cutStart, cutEnd);
        AIAssert(0);
        return 0;
    }

    if (nodesList.empty())
    {
        return 0;
    }

    ListNodeIds::iterator it;

    DbgCheckList(nodesList);

    Vec3r cutN = cutDirection.normalized();

    for (it = nodesList.begin(); it != nodesList.end(); it++)
    {
        unsigned curNodeIndex = *it;
        curNode = GetNodeManager().GetNode(curNodeIndex);

        if (std::find(rightSideNodes.begin(), rightSideNodes.end(), curNodeIndex) != rightSideNodes.end()) // already in list - skip
        {
            continue;
        }
        if (std::find(leftSideNodes.begin(), leftSideNodes.end(), curNodeIndex) != leftSideNodes.end()) // already in list - skip
        {
            continue;
        }

        unsigned newLink = curNode->FindNewLink(GetLinkManager());
        if (newLink)
        {
            unsigned nbrNodeIndex = GetLinkManager().GetNextNode(newLink);
            GraphNode* nbrNode = GetNodeManager().GetNode(nbrNodeIndex);
            unsigned nbrLink;
            real curCross = curNode->GetTriangularNavData()->GetCross(&vertexList, GetLinkManager(), vCutStart, cutN, newLink);

            for (nbrLink = nbrNode->firstLinkIndex; nbrLink; nbrLink = GetLinkManager().GetNextLink(nbrLink))
            {
                if (GetNodeManager().GetNode(GetLinkManager().GetNextNode(nbrLink)) == curNode)
                {
                    break;
                }
            }
            // mark neighbor - it's a new link in edge
            GetLinkManager().SetRadius(nbrLink, -1.f);
            real nbrCross = nbrNode->GetTriangularNavData()->GetCross(&vertexList, GetLinkManager(), vCutStart, cutN, (nbrLink));

            if (fabs(nbrCross) > fabs(curCross))
            {
                curCross = -nbrCross;
            }
            if (curCross < 0.000001f)
            {
                leftSideNodes.push_back(curNodeIndex);
                rightSideNodes.push_back(nbrNodeIndex);
            }
            else if (curCross > 0.000001f)
            {
                leftSideNodes.push_back(nbrNodeIndex);
                rightSideNodes.push_back(curNodeIndex);
            }
            else
            {
                AIAssert(!"dodgy cross product - ignore me?");
            }
        }
    }

    if (leftSideNodes.size() != rightSideNodes.size())
    {
        AIWarning("triangulation optimisation error -- newly-marked nodes are not symmetrical");
        AIAssert(0);
        return 0;
    }

    DbgCheckList(nodesList);
    for (it = nodesList.begin(); it != nodesList.end(); it++)
    {
        unsigned curNodeIndex = *it;
        curNode = GetNodeManager().GetNode(curNodeIndex);
        if (std::find(leftSideNodes.begin(), leftSideNodes.end(), curNodeIndex) != leftSideNodes.end()) // already in list - skip
        {
            continue;
        }
        if (std::find(rightSideNodes.begin(), rightSideNodes.end(), curNodeIndex) != rightSideNodes.end()) // already in list - skip
        {
            continue;
        }
        if ((curNode->GetTriangularNavData()->vertices[0] == cutStart) ||
            (curNode->GetTriangularNavData()->vertices[1] == cutStart) ||
            (curNode->GetTriangularNavData()->vertices[2] == cutStart) ||
            (curNode->GetTriangularNavData()->vertices[0] == cutEnd) ||
            (curNode->GetTriangularNavData()->vertices[1] == cutEnd) ||
            (curNode->GetTriangularNavData()->vertices[2] == cutEnd))
        {
            continue;       // don't touch begin/end of cut
        }
        bool bHasPointOnEdge = false;
        int vIdx(0);
        for (; vIdx < 3 && !bHasPointOnEdge; vIdx++)
        {
            for (ListNodeIds::iterator li = rightSideNodes.begin(); li != rightSideNodes.end() && !bHasPointOnEdge; li++)
            {
                GraphNode* refNode = GetNodeManager().GetNode(*li);
                unsigned newLink = refNode->FindNewLink(GetLinkManager());
                if (!newLink)
                {
                    continue;
                }
                if (refNode->GetTriangularNavData()->vertices[GetLinkManager().GetStartIndex(newLink)] == curNode->GetTriangularNavData()->vertices[vIdx] ||
                    refNode->GetTriangularNavData()->vertices[GetLinkManager().GetEndIndex(newLink)] == curNode->GetTriangularNavData()->vertices[vIdx])
                {
                    // found it - it's on cut - add the hode to outline
                    bHasPointOnEdge = true;
                }
            }
        }
        if (!bHasPointOnEdge)
        {
            continue;
        }
        vIdx--;
        // so just check now if it's left or right side and add to appropriate list
        int otherVertexIdx1 = 2 - vIdx;
        int otherVertexIdx2 = 3 - (otherVertexIdx1 + vIdx);

        const CVertexList& vertexList = m_pNavigation->GetVertexList();
        Vec3r theOtherVertex = vertexList.GetVertex(curNode->GetTriangularNavData()->vertices[otherVertexIdx1]).vPos
            - vCutStart;
        theOtherVertex.normalize();
        real curCross = cutN.x * theOtherVertex.y - cutN.y * theOtherVertex.x;
        theOtherVertex = vertexList.GetVertex(curNode->GetTriangularNavData()->vertices[otherVertexIdx2]).vPos
            - vCutStart;
        theOtherVertex.normalize();
        real cross2 = cutN.x * theOtherVertex.y - cutN.y * theOtherVertex.x;

        if (fabs(curCross) < fabs(cross2))
        {
            curCross = cross2;
        }
        if (curCross < 0)
        {
            leftSideNodes.push_back(curNodeIndex);
        }
        else
        {
            rightSideNodes.push_back(curNodeIndex);
        }
    }

    if (leftSideNodes.size() > 1)
    {
        // create left outline
        leftOutline.push_front(cutEnd);
        leftOutline.push_front(cutStart);
        if (!CreateOutline(areaName, leftSideNodes, nodesList, leftOutline))
        {
            return 0; // some strange problems (see CreateOutline)  - don't optimize this segment, return
        }
    }
    else
    {
        leftSideNodes.clear();
    }

    DbgCheckList(nodesList);
    if (rightSideNodes.size() > 1)
    {
        // create right outline
        rightOutline.push_front(cutEnd);
        rightOutline.push_front(cutStart);
        if (!CreateOutline(areaName, rightSideNodes, nodesList, rightOutline))
        {
            return 0; // some strange problems (see CreateOutline)  - don't optimize this segment, return
        }
    }
    else
    {
        rightSideNodes.clear();
    }

    DbgCheckList(nodesList);
    for (it = leftSideNodes.begin(); !leftSideNodes.empty() && it != leftSideNodes.end(); it++)
    {
        unsigned curNodeIndex = (*it);
        Disconnect(curNodeIndex);
        ListNodeIds::iterator itList = std::find(nodesList.begin(), nodesList.end(), curNodeIndex);
        nodesList.erase(itList);
    }

    DbgCheckList(nodesList);
    for (it = rightSideNodes.begin(); !rightSideNodes.empty() && it != rightSideNodes.end(); it++)
    {
        unsigned curNodeIndex = (*it);
        Disconnect(curNodeIndex);
        ListNodeIds::iterator itList = std::find(nodesList.begin(), nodesList.end(), curNodeIndex);
        nodesList.erase(itList);
    }

    DbgCheckList(nodesList);
    TriangulateOutline(areaName, nodesList, leftOutline, false);

    DbgCheckList(nodesList);
    TriangulateOutline(areaName, nodesList, rightOutline, true);
    DbgCheckList(nodesList);

    ConnectNodes(nodesList);
    DbgCheckList(nodesList);

    for (it = nodesList.begin(); it != nodesList.end(); it++)
    {
        curNode = GetNodeManager().GetNode(*it);
        for (unsigned link = curNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            if (GetLinkManager().GetRadius(link) < 0.0f ||
                (curNode->GetTriangularNavData()->vertices[GetLinkManager().GetStartIndex(link)] == cutStart &&
                 curNode->GetTriangularNavData()->vertices[GetLinkManager().GetEndIndex(link)] == cutEnd) ||
                (curNode->GetTriangularNavData()->vertices[GetLinkManager().GetEndIndex(link)] == cutStart &&
                 curNode->GetTriangularNavData()->vertices[GetLinkManager().GetStartIndex(link)] == cutEnd)
                )
            {
                GetLinkManager().SetRadius(link, -10.0f);
            }
        }
    }

    DbgCheckList(nodesList);
    return Counter;
}


//====================================================================
// CreateOutline
// creates outline of insideNodes, removes all triangles in outline from nodesList
//====================================================================
bool CGraph::CreateOutline(const string& areaName, ListNodeIds& insideNodes, ListNodeIds& nodesList, ObstacleIndexList&    outline)
{
    ListNodeIds::iterator it;
    GraphNode* curNode;
    bool  doneHere = false;
    ListNodeIds   insideNodesCurrent = insideNodes;

    for (it = insideNodesCurrent.begin(); !doneHere; it++)
    //for(it=insideNodesCurrent.begin(); it!=insideNodesCurrent.end(); it++)
    {
        if (it == insideNodesCurrent.end())
        {
            AIWarning("CGraph::CreateOutline can't find OutlineBegin  ");
            AIWarning("CGraph::CreateOutline can't find OutlineBegin  ");
            AIAssert(0);
            return false;
        }

        curNode = GetNodeManager().GetNode(*it);
        for (unsigned link = curNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            if (GetLinkManager().IsNewLink(link)) // it's on the edge - it's a new node
            {
                if ((curNode->GetTriangularNavData()->vertices[GetLinkManager().GetStartIndex(link)] == (*outline.begin())) ||
                    (curNode->GetTriangularNavData()->vertices[GetLinkManager().GetEndIndex(link)] == (*outline.begin())))
                {
                    int       theOtherVertexIdx = curNode->GetTriangularNavData()->vertices[3 - (GetLinkManager().GetStartIndex(link) + GetLinkManager().GetEndIndex(link))];

                    if (std::find(outline.begin(), outline.end(), theOtherVertexIdx) == outline.end())
                    {
                        outline.push_front(theOtherVertexIdx);
                    }
                    //                  else
                    //                      AIAssert(0);
                    insideNodesCurrent.erase(it++);
                    doneHere = true;
                    break;
                }
            }
        }
        if (doneHere)
        {
            break;
        }
    }

    if (!curNode)
    {
        AIWarning("CGraph::CreateOutline curNode is NULL  ");
        AIWarning("CGraph::CreateOutline curNode is NULL  ");
        AIAssert(0);
        return false;
    }



    while (!insideNodesCurrent.empty())
    {
        GraphNode* nextCandidate = NULL;
        int     theOtherVertexIdx = 0;
        int     nextOtherVertexIdx = 0;
        bool    bAddVertex = true;

        for (unsigned link = curNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            if (GetLinkManager().IsNewLink(link))
            {
                bAddVertex = false;
            }
        }

        //
        // if has links on base
        for (unsigned link = curNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            unsigned nextNodeIndex = GetLinkManager().GetNextNode(link);
            GraphNode* nextNode = GetNodeManager().GetNode(nextNodeIndex);
            if ((it = std::find(insideNodesCurrent.begin(), insideNodesCurrent.end(), nextNodeIndex)) != insideNodesCurrent.end())
            {
                for (unsigned nextLink = nextNode->firstLinkIndex; nextLink; nextLink = GetLinkManager().GetNextLink(nextLink))
                {
                    if (GetLinkManager().IsNewLink(nextLink))
                    {
                        nextCandidate = nextNode;
                        insideNodesCurrent.erase(it);
                        break;
                    }
                }
            }
        }
        //
        // if has noBase links
        for (unsigned link = curNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            unsigned nextNodeIndex = GetLinkManager().GetNextNode(link);
            GraphNode* nextNode = GetNodeManager().GetNode(nextNodeIndex);
            if ((it = std::find(insideNodesCurrent.begin(), insideNodesCurrent.end(), nextNodeIndex)) != insideNodesCurrent.end())
            {
                for (unsigned nextLink = nextNode->firstLinkIndex; nextLink; nextLink = GetLinkManager().GetNextLink(nextLink))
                {
                    if (GetNodeManager().GetNode(GetLinkManager().GetNextNode(nextLink)) == curNode)
                    {
                        if (!nextCandidate) // no base link - use this one
                        {
                            nextCandidate = nextNode;
                            insideNodesCurrent.erase(it);
                            nextOtherVertexIdx = nextNode->GetTriangularNavData()->vertices[3 - (GetLinkManager().GetStartIndex(nextLink) + GetLinkManager().GetEndIndex(nextLink))];
                        }
                        else
                        {
                            bAddVertex = false;
                        }

                        break;
                    }
                }
            }
        }
        if (bAddVertex)
        {
            if (std::find(outline.begin(), outline.end(), theOtherVertexIdx) == outline.end())
            {
                outline.push_front(theOtherVertexIdx);
            }
        }
        theOtherVertexIdx = nextOtherVertexIdx;

        //      AIAssert(nextCandidate);    //
        if (!nextCandidate)
        {
            AIWarning("CGraph::CreateOutline nextCandidate is NULL  ");
            AIAssert(0);
            return false;
        }
        curNode = nextCandidate;
    }
    return true;
}


//====================================================================
// TriangulateOutline
//====================================================================
void CGraph::TriangulateOutline(const string& areaName, ListNodeIds& nodesList, ObstacleIndexList& outline, bool orientation)
{
    if (outline.empty())
    {
        return;
    }

    //ListPositions::reverse_iterator crItr=outline.rbegin();
    //Vec3r vCut1 = *crItr++;
    //Vec3r vCut2 = *crItr;

    ObstacleData  obst;
    GraphNode_Triangular node(IAISystem::NAV_TRIANGULAR, Vec3r(ZERO), 0);
    ObstacleIndexList originalOutline;

    originalOutline.insert(originalOutline.begin(), outline.begin(), outline.end());

    CVertexList* pVertexList = &m_pNavigation->GetVertexList();

    while (outline.size() > 3)
    {
        //  GraphNode   tmpNode;
        //  ListPositions::iterator candidateRemove=outline.end();

        ObstacleIndexList::iterator candidateRemove = outline.end();
        bool notGood = false;
        real    candidateDegeneracy = -1.0f;

        unsigned candidateNodeIndex = CreateNewNode(IAISystem::NAV_TRIANGULAR, Vec3r(ZERO));
        GraphNode* candidateNode = GetNodeManager().GetNode(candidateNodeIndex);

        int cntr = 1;
        for (ObstacleIndexList::iterator curItr = outline.begin(); curItr != outline.end(); curItr++, cntr++)
        {
            ObstacleIndexList::iterator   vItr1 = curItr;
            ObstacleIndexList::iterator   vItr2 = curItr;
            if (cntr == outline.size())
            {
                vItr1 = outline.begin();
                vItr2 = vItr1;
                ++vItr2;
            }
            else if (cntr == outline.size() - 1)
            {
                vItr2 = outline.begin();
                ++vItr1;
            }
            else
            {
                ++vItr1;
                ++vItr2;
                ++vItr2;
            }

            node.GetTriangularNavData()->vertices.clear();
            //          obst.vPos = *curItr;
            node.GetTriangularNavData()->vertices.push_back(*curItr);
            //          obst.vPos = *vItr1;
            node.GetTriangularNavData()->vertices.push_back(*vItr1);
            //          obst.vPos = *vItr2;
            node.GetTriangularNavData()->vertices.push_back(*vItr2);

            notGood = false;
            if (node.GetTriangularNavData()->IsAntiClockwise(pVertexList) != orientation)
            {
                notGood = true;
            }
            node.GetTriangularNavData()->MakeAntiClockwise(pVertexList);

            // check if enithing inside the curremt triangle
            for (ObstacleIndexList::iterator   tmpItr = originalOutline.begin(); tmpItr != originalOutline.end() && !notGood; tmpItr++)
            {
                if (
                    //                  IsEquivalent()
                    *tmpItr == *curItr || *tmpItr == *vItr1 || *tmpItr == *vItr2)
                {
                    continue;
                }
                //              else if(PointInTriangle(*tmpItr, &tmpNode))
                else if (PointInTriangle(m_pNavigation->GetVertexList().GetVertex(*tmpItr).vPos, &node))
                {
                    notGood = true;
                }
            }

            if (notGood) // this triangle can't be used
            {
                continue;
            }

            real  dVal = node.GetTriangularNavData()->GetDegeneracyValue(pVertexList);

            if (dVal > candidateDegeneracy) // better candidate
            {
                candidateDegeneracy = dVal;
                candidateNode->GetTriangularNavData()->vertices = node.GetTriangularNavData()->vertices;
                candidateRemove = vItr1;
            }
        }

        //
        //add the best candidate - remove vertex from outline
        candidateNode->GetTriangularNavData()->MakeAntiClockwise(pVertexList);
        FillGraphNodeData(candidateNodeIndex);
        nodesList.push_back(candidateNodeIndex);
        AIAssert(candidateRemove != outline.end());
        outline.erase(candidateRemove);
    }


    //
    // add the last triangle
    unsigned candidateNodeIndex = CreateNewNode(IAISystem::NAV_TRIANGULAR, Vec3r(ZERO));
    GraphNode* candidateNode = GetNodeManager().GetNode(candidateNodeIndex);
    candidateNode->GetTriangularNavData()->vertices.push_back(outline.front());
    outline.pop_front();
    candidateNode->GetTriangularNavData()->vertices.push_back(outline.back());
    candidateNode->GetTriangularNavData()->vertices.push_back(outline.front());
    candidateNode->GetTriangularNavData()->MakeAntiClockwise(pVertexList);
    FillGraphNodeData(candidateNodeIndex);
    nodesList.push_back(candidateNodeIndex);
}

//====================================================================
// GetDegeneracyValue
//====================================================================
real    STriangularNavData::GetDegeneracyValue(const CVertexList* pVList)
{
    Vec3r firstE = pVList->GetVertex(vertices[1]).vPos - pVList->GetVertex(vertices[0]).vPos;
    Vec3r secondE = pVList->GetVertex(vertices[2]).vPos - pVList->GetVertex(vertices[0]).vPos;

    firstE.Normalize();
    secondE.Normalize();
    real  cosAngle0 = 1.0f - firstE.Dot(secondE);

    firstE = pVList->GetVertex(vertices[0]).vPos - pVList->GetVertex(vertices[1]).vPos;
    secondE = pVList->GetVertex(vertices[2]).vPos - pVList->GetVertex(vertices[1]).vPos;
    firstE.Normalize();
    secondE.Normalize();
    real  cosAngle1 = 1.0f - firstE.Dot(secondE);

    firstE = pVList->GetVertex(vertices[0]).vPos - pVList->GetVertex(vertices[2]).vPos;
    secondE = pVList->GetVertex(vertices[1]).vPos - pVList->GetVertex(vertices[2]).vPos;
    firstE.Normalize();
    secondE.Normalize();
    real  cosAngle2 = 1.0f - firstE.Dot(secondE);

    if (cosAngle0 <= cosAngle1 && cosAngle0 <= cosAngle2)
    {
        return cosAngle0;
    }
    if (cosAngle1 <= cosAngle2 && cosAngle1 <= cosAngle0)
    {
        return cosAngle1;
    }
    return cosAngle2;
}

//====================================================================
// MakeAntiClockwise
//====================================================================
void STriangularNavData::MakeAntiClockwise(const CVertexList* pVList)
{
    if (vertices.size() < 3)
    {
        return;
    }

    int od1 = vertices[0];
    int od2 = vertices[1];
    int od3 = vertices[2];

    Vec3r one = pVList->GetVertex(od2).vPos - pVList->GetVertex(od1).vPos;
    Vec3r two = pVList->GetVertex(od3).vPos - pVList->GetVertex(od2).vPos;

    real fcrossz = one.x * two.y - two.x * one.y;

    if (fcrossz < 0)
    {
        // rearrange the first and second
        vertices.clear();

        vertices.push_back(od2);
        vertices.push_back(od1);
        vertices.push_back(od3);

        //      MakeAntiClockwise();
    }
}


//====================================================================
// IsAntiClockwise
//====================================================================
bool STriangularNavData::IsAntiClockwise(const CVertexList* pVList)
{
    if (vertices.size() < 3)
    {
        return false;
    }

    int od1 = vertices[0];
    int od2 = vertices[1];
    int od3 = vertices[2];

    Vec3r one = pVList->GetVertex(od2).vPos - pVList->GetVertex(od1).vPos;
    Vec3r two = pVList->GetVertex(od3).vPos - pVList->GetVertex(od2).vPos;

    real fcrossz = one.x * two.y - two.x * one.y;

    if (fcrossz < 0)
    {
        return false;
    }
    return true;
}


//====================================================================
// DbgCheckList
//====================================================================
bool CGraph::DbgCheckList(ListNodeIds& nodesList) const
{
    bool isGood = true;
    int ndNumber = 0;

    for (ListNodeIds::iterator it = nodesList.begin(); it != nodesList.end(); it++, ndNumber++)
    {
        int counter = 0;
        unsigned curNodeIndex = *it;
        const GraphNode* curNode = GetNodeManager().GetNode(curNodeIndex);
        for (unsigned link = curNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            if (GetLinkManager().IsNewLink(link)) // it's on the edge - it's a new node
            {
                counter++;
            }
        }
        if (counter > 1)
        {
            isGood = false;
        }
        if (curNode->GetTriangularNavData()->vertices.size() < 3)
        {
            isGood = false;
        }

        if (curNode->nRefCount > 1 && curNode->nRefCount < 4)
        {
            isGood = false;
        }
        ListNodeIds::iterator li;
        ListNodeIds::iterator itnext = it;
        if (++itnext != nodesList.end())
        {
            li = std::find(itnext, nodesList.end(), (*it));
            if (li != nodesList.end())
            {
                isGood = false;
            }
        }
    }

    return isGood;
}

//====================================================================
// GetCross
//====================================================================
real STriangularNavData::GetCross(const CVertexList* pVList, CGraphLinkManager& linkManager, const Vec3r& vCutStart, const Vec3r& vDir, unsigned theLink)
{
    Vec3r theOtherVertex = pVList->GetVertex(vertices[3 - (linkManager.GetStartIndex(theLink) + linkManager.GetEndIndex(theLink))]).vPos
        - vCutStart;
    theOtherVertex.normalize();
    return vDir.x * theOtherVertex.y - vDir.y * theOtherVertex.x;
}

//====================================================================
// FindNewLink
//====================================================================
unsigned GraphNode::FindNewLink(CGraphLinkManager& linkManager)
{
    for (unsigned link = firstLinkIndex; link; link = linkManager.GetNextLink(link))
    {
        if (linkManager.IsNewLink(link)) // it's on the edge - it's a new node
        {
            return link;
        }
    }
    return 0;
}
