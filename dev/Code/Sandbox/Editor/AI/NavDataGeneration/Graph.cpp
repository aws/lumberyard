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

// Description : implementation of the CGraph class.


#include "StdAfx.h"

#include <limits>
#include <algorithm>

#include <ISystem.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <ILog.h>
#include <I3DEngine.h>
#include <ITimer.h>
#include <IConsole.h>
#include <IPhysics.h>
#include <CryFile.h>

#include "Graph.h"
#include "AILog.h"
#include "Cry_Math.h"
#include "VertexList.h"
#include "TriangularNavRegion.h"
#include "WaypointHumanNavRegion.h"
#include "VolumeNavRegion.h"
#include "FlightNavRegion.h"
#include "RoadNavRegion.h"
#include "GraphLinkManager.h"
#include "GraphNodeManager.h"
#include "CommonDefs.h"
#include "Navigation.h"

#pragma warning(disable: 4244)

#define BAI_TRI_FILE_VERSION 54

// identifier so links can be marked as impassable, then restored
static const float RADIUS_FOR_BROKEN_LINKS = -121314.0f;

SetObstacleRefs CGraph::static_AllocatedObstacles;

const float GraphNode::fInvalidCost = 999999.0f;

StaticInstance<std::vector<const GraphNode*>> g_DebugGraphNodesToDraw;

//====================================================================
// ValidateNode
//====================================================================
inline bool CGraph::ValidateNode(unsigned nodeIndex, bool fullCheck) const
{
    const GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

    if (!nodeIndex)
    {
        return false;
    }
    if (!m_allNodes.DoesNodeExist(nodeIndex))
    {
        return false;
    }
    if (!fullCheck)
    {
        return true;
    }
    else
    {
        return ValidateNodeFullCheck(pNode);
    }
}

//====================================================================
// ValidateNodeFullCheck
//====================================================================
bool CGraph::ValidateNodeFullCheck(const GraphNode* pNode) const
{
    bool result = true;
    AIAssert(pNode);
    int nNonRoadLinks = 0;
    unsigned nTriLinks = 0;
    for (unsigned linkId = pNode->firstLinkIndex; linkId; linkId = GetLinkManager().GetNextLink(linkId))
    {
        unsigned nextNodeIndex = GetLinkManager().GetNextNode(linkId);
        const GraphNode* next = GetNodeManager().GetNode(nextNodeIndex);
        if (!CGraph::ValidateNode(nextNodeIndex, false))
        {
            result = false;
        }
        if (next->navType != IAISystem::NAV_ROAD)
        {
            ++nNonRoadLinks;
        }
        if (next->navType == IAISystem::NAV_TRIANGULAR)
        {
            ++nTriLinks;
        }
    }
    if (nNonRoadLinks > 50)
    {
        AIWarning("Too many non-road links (%d) from node %p type %d at position: (%5.2f, %5.2f, %5.2f)", nNonRoadLinks, pNode, pNode->navType,
            pNode->GetPos().x, pNode->GetPos().y, pNode->GetPos().z);
    }
    if (pNode->navType == IAISystem::NAV_TRIANGULAR && nTriLinks != 3)
    {
        unsigned numVertices = pNode->GetTriangularNavData()->vertices.size();
        if (numVertices != 3)
        {
            AIWarning("Triangular node at position: (%5.2f %5.2f %5.2f) does not have 3 vertices", pNode->GetPos().x, pNode->GetPos().y, pNode->GetPos().z);
        }
        else
        {
            const CVertexList& vertexList = m_pNavigation->GetVertexList();
            Vec3 v0Pos = vertexList.GetVertex(pNode->GetTriangularNavData()->vertices[0]).vPos;
            Vec3 v1Pos = vertexList.GetVertex(pNode->GetTriangularNavData()->vertices[1]).vPos;
            Vec3 v2Pos = vertexList.GetVertex(pNode->GetTriangularNavData()->vertices[2]).vPos;
            int numOut = 0;
            numOut += InsideOfBBox(v0Pos) ? 0 : 1;
            numOut += InsideOfBBox(v1Pos) ? 0 : 1;
            numOut += InsideOfBBox(v2Pos) ? 0 : 1;
            if (numOut < 2)
            {
                AIWarning("Triangular node at position: (%5.2f %5.2f %5.2f) does not have 3 triangular links (has %d). Triangulate level",
                    pNode->GetPos().x, pNode->GetPos().y, pNode->GetPos().z, numOut);
            }
        }
    }
    return result;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//====================================================================
// CGraph
//====================================================================
CGraph::CGraph(CNavigation* pNavigation)
    :   m_pGraphLinkManager(new CGraphLinkManager())
    , m_pGraphNodeManager(new CGraphNodeManager())
    , m_allNodes(*m_pGraphNodeManager)
    , m_pNavigation(pNavigation)
    , m_triangularBBox(AABB::RESET)
{
    m_safeFirstIndex = CreateNewNode(IAISystem::NAV_UNSET, Vec3(ZERO), 0);
    m_firstIndex = m_safeFirstIndex;
    m_pFirst = GetNodeManager().GetNode(m_safeFirstIndex);
    m_pSafeFirst = m_pFirst;
    m_pCurrent = m_pFirst;
    m_currentIndex = m_firstIndex;
    m_pCurrent->firstLinkIndex = 0;
}

//====================================================================
// ~CGraph
//====================================================================
CGraph::~CGraph()
{
    Clear(IAISystem::NAVMASK_ALL);

    delete m_pGraphNodeManager;
    delete m_pGraphLinkManager;
}


//====================================================================
// Clear
//====================================================================
void CGraph::Clear(unsigned navTypeMask)
{
    ClearMarks();
    ClearTags();
    DeleteGraph(navTypeMask);
    if (navTypeMask & (IAISystem::NAV_WAYPOINT_HUMAN | IAISystem::NAV_WAYPOINT_3DSURFACE))
    {
        EntranceMap::iterator next;
        for (EntranceMap::iterator it = m_mapEntrances.begin(); it != m_mapEntrances.end(); it = next)
        {
            next = it;
            ++next;
            GraphNode* pNode = GetNodeManager().GetNode(it->second);
            if (pNode->navType & navTypeMask)
            {
                m_mapEntrances.erase(it);
            }
        }
        for (EntranceMap::iterator it = m_mapExits.begin(); it != m_mapExits.end(); it = next)
        {
            next = it;
            ++next;
            GraphNode* pNode = GetNodeManager().GetNode(it->second);
            if (pNode->navType & navTypeMask)
            {
                m_mapExits.erase(it);
            }
        }
        for (EntranceMap::iterator it = m_mapRemovables.begin(); it != m_mapRemovables.end(); it = next)
        {
            next = it;
            ++next;
            GraphNode* pNode = GetNodeManager().GetNode(it->second);
            if (pNode->navType & navTypeMask)
            {
                m_mapRemovables.erase(it);
            }
        }
    }

    GetNodeManager().Clear(navTypeMask);

    if (m_pSafeFirst)
    {
        Disconnect(m_safeFirstIndex, false);
    }
    else
    {
        m_safeFirstIndex = CreateNewNode(IAISystem::NAV_UNSET, Vec3(ZERO), 0);
        m_pSafeFirst = GetNodeManager().GetNode(m_safeFirstIndex);
    }
    m_pFirst = m_pSafeFirst;
    m_firstIndex = m_safeFirstIndex;
    m_pCurrent = m_pFirst;
    m_currentIndex = m_firstIndex;
}

//====================================================================
// GetEnclosing
//====================================================================
unsigned CGraph::GetEnclosing(const Vec3& pos, IAISystem::tNavCapMask navCapMask, float passRadius, unsigned startIndex,
    float range, Vec3* closestValid, bool returnSuspect, const char* requesterName)
{
    IVisArea* pGoalArea;
    int nGoalBuilding;

    IAISystem::ENavigationType navType = m_pNavigation->CheckNavigationType(pos, nGoalBuilding, pGoalArea, navCapMask);

    if (!(navType & navCapMask))
    {
        return 0;
    }

    unsigned nodeIndex = 0;

    // in some cases prefer roads over everything. In other cases only check roads at the end
    bool checkRoadsFirst = !(navCapMask & IAISystem::NAV_VOLUME);
    if (checkRoadsFirst && navCapMask & IAISystem::NAV_ROAD)
    {
        nodeIndex = m_pNavigation->GetRoadNavRegion()->GetEnclosing(pos, passRadius, startIndex, range, closestValid, returnSuspect, requesterName);
        if (nodeIndex)
        {
            return nodeIndex;
        }
    }
    switch (navType)
    {
    case IAISystem::NAV_TRIANGULAR:
    case IAISystem::NAV_WAYPOINT_HUMAN:
    case IAISystem::NAV_WAYPOINT_3DSURFACE:
    case IAISystem::NAV_FLIGHT:
    case IAISystem::NAV_VOLUME:
    case IAISystem::NAV_ROAD:
    case IAISystem::NAV_FREE_2D:
        nodeIndex = m_pNavigation->GetNavRegion(navType, this)->GetEnclosing(pos, passRadius, startIndex, range, closestValid, returnSuspect, requesterName);
        break;
    default:
        AIError("CGraph::GetEnclosing Unhandled navigation type: %d %s [Code bug]", navType, requesterName);
        return 0;
    }
    if (!nodeIndex && !checkRoadsFirst && navCapMask & IAISystem::NAV_ROAD)
    {
        nodeIndex = m_pNavigation->GetRoadNavRegion()->GetEnclosing(pos, passRadius, startIndex, range, closestValid, returnSuspect, requesterName);
    }
    return nodeIndex;
}

//====================================================================
// Connect
//====================================================================
void CGraph::Connect(unsigned oneNodeIndex, unsigned twoNodeIndex,
    float radiusOneToTwo, float radiusTwoToOne,
    unsigned* pLinkOneTwo, unsigned* pLinkTwoOne)
{
    GraphNode* one = GetNodeManager().GetNode(oneNodeIndex);
    GraphNode* two = GetNodeManager().GetNode(twoNodeIndex);

    if (pLinkOneTwo)
    {
        *pLinkOneTwo = 0;
    }
    if (pLinkTwoOne)
    {
        *pLinkTwoOne = 0;
    }

    if (one == two)
    {
        return;
    }

    if (!one || !two)
    {
        return;
    }

    if (!CGraph::ValidateNode(oneNodeIndex, false) || !CGraph::ValidateNode(twoNodeIndex, false))
    {
        AIError("CGraph::Connect Attempt to connect nodes that aren't created [Code bug]");
        return;
    }

    if ((one == m_pSafeFirst || two == m_pSafeFirst) && m_pSafeFirst->firstLinkIndex)
    {
        AIWarning("Second link being made to safe/first node");
        return;
    }

    extern StaticInstance<std::vector<const GraphNode*>> g_DebugGraphNodesToDraw;
    g_DebugGraphNodesToDraw.clear();

    // handle case where they're already connected
    unsigned linkIndexOne = one->GetLinkTo(GetNodeManager(), GetLinkManager(), two);
    unsigned linkIndexTwo = two->GetLinkTo(GetNodeManager(), GetLinkManager(), one);

    if ((linkIndexOne == 0) != (linkIndexTwo == 0))
    {
        AIWarning("Trying to connect links but one is already connected, other isn't");
        return;
    }

    // Check that if both links have bidirectional data, then it is the same.
    assert(linkIndexOne == 0 || linkIndexTwo == 0 || (linkIndexOne & ~1) == (linkIndexTwo & ~1));

    // Create new bidirectional data if necessary.
    unsigned linkIndex = linkIndexOne;
    if (!linkIndex && linkIndexTwo)
    {
        linkIndex = linkIndexTwo ^ 1;
    }
    if (!linkIndex)
    {
        linkIndex = m_pGraphLinkManager->CreateLink();
    }

    Vec3 midPos = 0.5f * (one->GetPos() + two->GetPos());

    if (!linkIndexOne)
    {
        // [1/3/2007 MichaelS] Should be possible to push link on front, but I don't want to run the risk
        // of breaking code that relies on the link order being preserved, so we add it to the end.
        //one->links.push_back(linkIndex);
        if (!one->firstLinkIndex)
        {
            one->firstLinkIndex = linkIndex;
        }
        else
        {
            unsigned lastLink, nextLink;
            for (lastLink = one->firstLinkIndex; nextLink = m_pGraphLinkManager->GetNextLink(lastLink); lastLink = nextLink)
            {
                ;
            }
            m_pGraphLinkManager->SetNextLink(lastLink, linkIndex);
        }

        linkIndexOne = linkIndex;
        GetLinkManager().SetNextNode(linkIndex, twoNodeIndex);
        GetLinkManager().SetRadius(linkIndex, two == m_pSafeFirst ? -1.0f : radiusOneToTwo);
        GetLinkManager().GetEdgeCenter(linkIndex) = midPos;
        two->AddRef();
    }
    else
    {
        if (radiusOneToTwo != 0.0f)
        {
            GetLinkManager().ModifyRadius(linkIndexOne, radiusOneToTwo);
        }
    }
    if (!linkIndexTwo)
    {
        // [1/3/2007 MichaelS] Should be possible to push link on front, but I don't want to run the risk
        // of breaking code that relies on the link order being preserved, so we add it to the end.
        //one->links.push_back(linkIndex);
        if (!two->firstLinkIndex)
        {
            two->firstLinkIndex = linkIndex ^ 1;
        }
        else
        {
            unsigned lastLink, nextLink;
            for (lastLink = two->firstLinkIndex; nextLink = m_pGraphLinkManager->GetNextLink(lastLink); lastLink = nextLink)
            {
                ;
            }
            m_pGraphLinkManager->SetNextLink(lastLink, linkIndex ^ 1);
        }

        linkIndexTwo = linkIndex ^ 1;
        GetLinkManager().SetNextNode(linkIndex ^ 1, oneNodeIndex);
        GetLinkManager().SetRadius(linkIndex ^ 1, one == m_pSafeFirst ? -1.0f : radiusTwoToOne);
        GetLinkManager().GetEdgeCenter(linkIndex ^ 1) = midPos;
        one->AddRef();
    }
    else
    {
        if (radiusTwoToOne != 0.0f)
        {
            GetLinkManager().ModifyRadius(linkIndexTwo, radiusTwoToOne);
        }
    }

    if (pLinkOneTwo && linkIndexOne)
    {
        *pLinkOneTwo = linkIndexOne;
    }
    if (pLinkTwoOne && linkIndexTwo)
    {
        *pLinkTwoOne = linkIndexTwo;
    }

    if (m_pSafeFirst->firstLinkIndex)
    {
        return;
    }

    if (one->navType == IAISystem::NAV_TRIANGULAR)
    {
        Connect(m_safeFirstIndex, oneNodeIndex, 100.0f, -1.0f);
        m_pFirst = one;
        m_firstIndex = oneNodeIndex;
    }
    else if (two->navType == IAISystem::NAV_TRIANGULAR)
    {
        Connect(m_safeFirstIndex, twoNodeIndex, 100.0f, -1.0f);
        m_pFirst = two;
        m_firstIndex = twoNodeIndex;
    }
    // may have incurred a reallocation
    // [1/3/2007 MichaelS] Should no longer be necessary since we use indices instead of pointers, but leaving it here for now.
    if (pLinkOneTwo && linkIndexOne)
    {
        *pLinkOneTwo = linkIndexOne;
    }
    if (pLinkTwoOne && linkIndexTwo)
    {
        *pLinkTwoOne = linkIndexTwo;
    }
}

//====================================================================
// WriteToFile
//====================================================================
void CGraph::WriteToFile(const char* pname)
{
    m_vLinkDescs.clear();
    m_vNodeDescs.clear();

    m_vNodeDescs.reserve(100000);

    std::map<GraphNode*, unsigned> IDMap;
    unsigned lastAssignedID = 1;

    GraphNode::ResetIDs(GetNodeManager(),   m_allNodes, m_pSafeFirst);

    CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAV_TRIANGULAR | IAISystem::NAV_WAYPOINT_HUMAN |
        IAISystem::NAV_WAYPOINT_3DSURFACE | IAISystem::NAV_UNSET);

    while (unsigned currentNodeIndex = it.Increment())
    {
        WriteLine(currentNodeIndex);
    }

    CCryFile file;
    if (false != file.Open(pname, "wb"))
    {
        int iNumber = m_vNodeDescs.size();
        int nFileVersion = BAI_TRI_FILE_VERSION;
        int nRandomNr = 0;

        file.Write(&nFileVersion, sizeof(int));
        file.Write(&m_triangularBBox.min, sizeof(m_triangularBBox.min));
        file.Write(&m_triangularBBox.max, sizeof(m_triangularBBox.max));

        // write the triangle descriptors
        file.Write(&iNumber, sizeof(int));
        file.Write(&m_vNodeDescs[ 0 ], iNumber * sizeof(NodeDescriptor));

        iNumber = m_vLinkDescs.size();
        file.Write(&iNumber, sizeof(int));
        if (iNumber > 0)
        {
            file.Write(&m_vLinkDescs[ 0 ], iNumber * sizeof(LinkDescriptor));
        }

        file.Close();

        ClearVectorMemory(m_vLinkDescs);
        ClearVectorMemory(m_vNodeDescs);
        //m_mapEntrances.clear();
    }
}

//====================================================================
// WriteLine
//====================================================================
void CGraph::WriteLine(unsigned nodeIndex)
{
    GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);
    AIAssert(pNode);
    NodeDescriptor desc;

    AIAssert(pNode->navType >= 0 && pNode->navType <= (1 << sizeof(desc.navType) * 8) - 1);
    desc.navType = pNode->navType;
    desc.ID = pNode->ID;
    desc.pos = pNode->GetPos();

    desc.obstacle[0] = -1;
    desc.obstacle[1] = -1;
    desc.obstacle[2] = -1;

    switch (pNode->navType)
    {
    case IAISystem::NAV_UNSET:
        break;
    case IAISystem::NAV_TRIANGULAR:
    {
        int nObstacles = pNode->GetTriangularNavData()->vertices.size();
        if (nObstacles > 3)
        {
            AIWarning("Found node with more than 3 vertices linked to it - ignoring remaining vertices");
            nObstacles = 3;
        }
        for (int i = 0; i < nObstacles; ++i)
        {
            if (!m_pNavigation->GetVertexList().IsIndexValid(pNode->GetTriangularNavData()->vertices[i]))
            {
                AIError("Invalid obstacle index %d on saving", pNode->GetTriangularNavData()->vertices[i]);
            }
            desc.obstacle[i] = pNode->GetTriangularNavData()->vertices[i];
        }
        desc.bForbidden = pNode->GetTriangularNavData()->isForbidden;
        desc.bForbiddenDesigner = pNode->GetTriangularNavData()->isForbiddenDesigner;
    }
    break;
    case IAISystem::NAV_WAYPOINT_3DSURFACE:
    case IAISystem::NAV_WAYPOINT_HUMAN:
    {
        desc.index = pNode->GetWaypointNavData()->nBuildingID;
        desc.type = pNode->GetWaypointNavData()->type;
        EntranceMap::iterator ei;
        // see if this one is removable
        ei = m_mapRemovables.find(desc.index);
        if (ei != m_mapRemovables.end())
        {
            while ((ei != m_mapRemovables.end()) && (ei->first == desc.index))
            {
                if ((ei->second) == nodeIndex)
                {
                    desc.bRemovable = 1;
                    break;
                }
                ++ei;
            }
        }
        desc.dir = pNode->GetWaypointNavData()->dir;
        desc.up = pNode->GetWaypointNavData()->up;
    }
    break;
    case IAISystem::NAV_FLIGHT:
        desc.index = pNode->GetFlightNavData()->nSpanIdx;
        break;
    case IAISystem::NAV_VOLUME:
        desc.index = pNode->GetVolumeNavData()->nVolimeIdx;
        break;
    case IAISystem::NAV_ROAD:
        desc.dir.x = pNode->GetRoadNavData()->fRoadWidth;
        desc.dir.y = 0.0f /*pNode->GetRoadNavData()->fRoadOffset*/;
        break;
    case IAISystem::NAV_SMARTOBJECT:
        break;
    default:
        AIError("CGraph::WriteLine Unhandled nav type %d", pNode->navType);
    }

    m_vNodeDescs.push_back(desc);

    // now write the links
    for (unsigned link = pNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
    {
        unsigned int toPushIndex = GetLinkManager().GetNextNode(link);
        GraphNode* pToPush = GetNodeManager().GetNode(toPushIndex);

        // don't save non triangular/waypoint nodes - will be taken care of by the nav region Save
        // Danny todo do this properly
        if (pToPush->navType != IAISystem::NAV_TRIANGULAR &&
            pToPush->navType != IAISystem::NAV_WAYPOINT_HUMAN &&
            pToPush->navType != IAISystem::NAV_WAYPOINT_3DSURFACE)
        {
            continue;
        }

        LinkDescriptor ld;
        ld.nSourceNode = desc.ID;
        ld.nTargetNode = pToPush->ID;
        AIAssert(GetLinkManager().GetStartIndex(link) <= 2 && GetLinkManager().GetEndIndex(link) <= 2);
        ld.fMaxPassRadius = GetLinkManager().GetOrigRadius(link);
        ld.nStartIndex = GetLinkManager().GetStartIndex(link);
        ld.nEndIndex = GetLinkManager().GetEndIndex(link);
        ld.vEdgeCenter = GetLinkManager().GetEdgeCenter(link);
        ld.bIsPureTriangularLink = (pNode->navType == IAISystem::NAV_TRIANGULAR) && (pToPush->navType == IAISystem::NAV_TRIANGULAR);

        ld.fExposure = GetLinkManager().GetExposure(link); // TODO: Shouldn't write out shared value twice.
        ld.fMaxWaterDepth = GetLinkManager().GetMaxWaterDepth(link);
        ld.fMinWaterDepth = GetLinkManager().GetMinWaterDepth(link);

        ld.bSimplePassabilityCheck = GetLinkManager().IsSimple(link);

        m_vLinkDescs.push_back(ld);
    }
}

//====================================================================
// DeleteGraph
//====================================================================
void CGraph::DeleteGraph(IAISystem::tNavCapMask navTypeMask)
{
    std::vector<unsigned> nodesToDelete;
    CAllNodesContainer::Iterator it(m_allNodes, navTypeMask);
    while (unsigned nodeIndex = it.Increment())
    {
        nodesToDelete.push_back(nodeIndex);
    }

    for (unsigned i = 0; i < nodesToDelete.size(); ++i)
    {
        GraphNode* pNode = GetNodeManager().GetNode(nodesToDelete[i]);
        AIAssert(pNode->navType & navTypeMask);
        Disconnect(nodesToDelete[i]);
        if (pNode == m_pSafeFirst)
        {
            m_pSafeFirst = 0;
            m_safeFirstIndex = 0;
        }
    }
}

//====================================================================
// Disconnect
//====================================================================
void CGraph::Disconnect(unsigned nodeIndex, unsigned linkId)
{
    GraphNode* pNode = m_pGraphNodeManager->GetNode(nodeIndex);

    if (!CGraph::ValidateNode(nodeIndex, false))
    {
        AIError("CGraph::Disconnect Attempt to disconnect link from node that isn't created [Code bug]");
        return;
    }
    AIAssert(linkId);
    unsigned otherNodeIndex = GetLinkManager().GetNextNode(linkId);
    GraphNode* pOtherNode = GetNodeManager().GetNode(otherNodeIndex);
    if (!CGraph::ValidateNode(otherNodeIndex, false))
    {
        AIError("CGraph::Disconnect Attempt to disconnect link from other node that isn't created [Code bug]");
        return;
    }

    pNode->RemoveLinkTo(GetLinkManager(), otherNodeIndex);
    pNode->Release();

    pOtherNode->RemoveLinkTo(GetLinkManager(), nodeIndex);
    pOtherNode->Release();

    GetLinkManager().DestroyLink(linkId);
}

//====================================================================
// Disconnect
//====================================================================
void CGraph::Disconnect(unsigned nodeIndex, bool bDelete)
{
    GraphNode* pDisconnected = m_pGraphNodeManager->GetNode(nodeIndex);

    if (!CGraph::ValidateNode(nodeIndex, false))
    {
        AIError("CGraph::Disconnect Attempt to disconnect node that isn't created [Code bug]");
        return;
    }

    extern StaticInstance<std::vector<const GraphNode*>> g_DebugGraphNodesToDraw;
    g_DebugGraphNodesToDraw.clear();

    // if the node we are disconnecting is the current node, move the current
    // to one of his links, or the root if it has no links
    if (pDisconnected == m_pCurrent)
    {
        if (pDisconnected->firstLinkIndex)
        {
            m_currentIndex = GetLinkManager().GetNextNode(pDisconnected->firstLinkIndex);
            m_pCurrent = GetNodeManager().GetNode(m_currentIndex);
        }
        else
        {
            m_currentIndex = m_safeFirstIndex;
            m_pCurrent = m_pSafeFirst;
        }
    }

    // if its the root that is being disconnected, move it
    if (m_pFirst == pDisconnected)
    {
        if (pDisconnected->firstLinkIndex)
        {
            m_firstIndex = GetLinkManager().GetNextNode(pDisconnected->firstLinkIndex);
            m_pFirst = GetNodeManager().GetNode(m_firstIndex);
        }
        else
        {
            if (m_pFirst != m_pSafeFirst)
            {
                m_pFirst = m_pSafeFirst;
                m_firstIndex = m_safeFirstIndex;
            }
            else
            {
                m_pFirst = 0;
                m_firstIndex = 0;
            }
        }
    }

    // now disconnect this node from its links
    for (unsigned link = pDisconnected->firstLinkIndex, nextLink; link; link = nextLink)
    {
        nextLink = GetLinkManager().GetNextLink(link);

        unsigned nextNodeIndex = GetLinkManager().GetNextNode(link);
        GraphNode* pNextNode = GetNodeManager().GetNode(nextNodeIndex);
        pNextNode->RemoveLinkTo(GetLinkManager(), nodeIndex);
        pNextNode->Release();
        pDisconnected->Release();
        GetLinkManager().DestroyLink(link);
    }

    pDisconnected->firstLinkIndex = 0;

    if (pDisconnected->nRefCount != 1)
    {
        AIWarning("Node reference count is not 1 after disconnecting");
    }

    if (bDelete)
    {
        DeleteNode(nodeIndex);
    }

    if (!m_pSafeFirst)
    {
        return;
    }

    if (pDisconnected != m_pSafeFirst && !m_pSafeFirst->firstLinkIndex)
    {
        unsigned firstIndex = m_firstIndex;
        // we have disconnected the link to the dummy safe node - relink it to any outdoor node of the graph
        if (firstIndex == m_safeFirstIndex)
        {
            if (m_currentIndex == m_safeFirstIndex)
            {
                // try any entrance
                if (!m_mapEntrances.empty())
                {
                    firstIndex = (m_mapEntrances.begin()->second);
                }
                else
                {
                    return; // m_pSafeFirst links will stay empty
                }
            }
            else
            {
                firstIndex = m_currentIndex;
            }
        }

        if (firstIndex)
        {
            GraphNode* pFirst = GetNodeManager().GetNode(firstIndex);
            if (pFirst->navType == IAISystem::NAV_TRIANGULAR)
            {
                Connect(m_safeFirstIndex, firstIndex, 100.0f, -1.0f);
            }
            else if (pFirst->navType == IAISystem::NAV_WAYPOINT_HUMAN)
            {
                GraphNode* pEntrance = GetEntrance(pFirst->GetWaypointNavData()->nBuildingID, Vec3(0, 0, 0));
                if (pEntrance)
                {
                    for (unsigned link = pEntrance->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
                    {
                        unsigned nextNodeIndex = GetLinkManager().GetNextNode(link);
                        GraphNode* pNextNode = GetNodeManager().GetNode(nextNodeIndex);
                        if (pNextNode->navType == IAISystem::NAV_WAYPOINT_HUMAN)
                        {
                            Connect(m_safeFirstIndex, nextNodeIndex, 100.0f, -1.0f);
                            break;
                        }
                    }
                }
            }
        }
    }
}

bool operator<(const Vec3r& v1, const Vec3r& v2)
{
    if (v1.x < v2.x)
    {
        return true;
    }
    else if (v1.x > v2.y)
    {
        return false;
    }
    if (v1.y < v2.y)
    {
        return true;
    }
    else if (v1.y > v2.y)
    {
        return false;
    }
    if (v1.z < v2.z)
    {
        return true;
    }
    else if (v1.z > v2.z)
    {
        return false;
    }
    return false;
}

//====================================================================
// PointInTriangle
// Check whether a position is within a node's triangle
//====================================================================
bool CGraph::PointInTriangle(const Vec3& pos, GraphNode* pNode)
{
    AIAssert(pNode);
    if (!pNode || pNode->navType != IAISystem::NAV_TRIANGULAR || pNode->GetTriangularNavData()->vertices.empty())
    {
        return false;
    }
    unsigned numV = pNode->GetTriangularNavData()->vertices.size();

#define CROSS2D(vec1, vec2) (vec1.x * vec2.y - vec1.y * vec2.x)

    // don't know the winding of the triangle, so just require all the results to be
    // the same. 0 Means there wasn't a prev result, or it was on the edge
    int prevResult = 0;

    // count points close to the boundary as inside the triangle
    const real tol = 0.00000000001;

    for (unsigned i = 0; i < numV; ++i)
    {
        unsigned iNext = (i + 1) % numV;
        int index1 = pNode->GetTriangularNavData()->vertices[i];
        int index2 = pNode->GetTriangularNavData()->vertices[iNext];

        const CVertexList& vertexList = m_pNavigation->GetVertexList();
        Vec3r v1 = vertexList.GetVertex(index1).vPos;
        Vec3r v2 = vertexList.GetVertex(index2).vPos;

        // ensure that the test is consistent for adjacent triangles
        bool swapped = false;
        if (v1 < v2)
        {
            std::swap(v1, v2);
            swapped = true;
        }

        Vec3r edge = v2 - v1;
        Vec3r dir = pos - v1;
        // inside if edge x dir is up... but only in twoD
        real cross = CROSS2D(edge, dir);

        if (swapped)
        {
            cross = -cross;
        }

        int result;
        if (cross > tol)
        {
            result = 1;
        }
        else if (cross < -tol)
        {
            result = 2;
        }
        else
        {
            result = 0;
        }

        if (prevResult == 1 && result == 2)
        {
            return false;
        }
        else if (prevResult == 2 && result == 1)
        {
            return false;
        }

        if (result != 0)
        {
            prevResult = result;
        }
    }
    // if prevResult == 0 then it means all our results were 0 - bloomin wierd,
    // but we would still want to return true
    // AIAssert(prevResult);
    return true;
#undef CROSS2D
}

//#define VALIDATEINMARK

//====================================================================
// MarkNode
// uses mark for internal graph operation without disturbing the pathfinder
//====================================================================
void CGraph::MarkNode(unsigned nodeIndex) const
{
#ifdef VALIDATEINMARK
    if (!CGraph::ValidateNode(nodeIndex))
    {
        AIError("CGraph::MarkNode Unable to validate/mark node %p [Code bug]", pNode);
        return;
    }
#endif
    GetNodeManager().GetNode(nodeIndex)->mark = 1;
    m_markedNodes.push_back(nodeIndex);
}

//====================================================================
// ClearMarks
// clears the marked nodes
//====================================================================
void CGraph::ClearMarks() const
{
    while (!m_markedNodes.empty())
    {
#ifdef VALIDATEINMARK
        if (!CGraph::ValidateNode(m_markedNodes.back()))
        {
            AIError("CGraph::ClearMarks Unable to validate/clear mark node %p [Code bug]", m_markedNodes.back());
        }
        else
#endif
        GetNodeManager().GetNode(m_markedNodes.back())->mark = 0;
        m_markedNodes.pop_back();
    }
}

//====================================================================
// TagNode
//====================================================================
void CGraph::TagNode(unsigned nodeIndex) const
{
    GetNodeManager().GetNode(nodeIndex)->tag = 1;
    m_taggedNodes.push_back(nodeIndex);
}

//====================================================================
// ClearTags
//====================================================================
void CGraph::ClearTags() const
{
    while (!m_taggedNodes.empty())
    {
        const GraphNode* pLastNode = GetNodeManager().GetNode(m_taggedNodes.back());
        pLastNode->tag = 0;
        pLastNode->fCostFromStart = GraphNode::fInvalidCost;
        pLastNode->fEstimatedCostToGoal = GraphNode::fInvalidCost;
        pLastNode->prevPathNodeIndex = 0;
        m_taggedNodes.pop_back();
    }
}

//====================================================================
// MakeNodeRemovable
// adds an entrance for easy traversing later
//====================================================================
void CGraph::MakeNodeRemovable(int nBuildingID, unsigned nodeIndex, bool bRemovable)
{
    GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

    if (!CGraph::ValidateNode(nodeIndex, false))
    {
        AIError("CGraph::MakeNodeRemovable Attempt to make removeable node that isn't created [Code bug]");
        return;
    }
    EntranceMap::iterator ei = m_mapRemovables.find(nBuildingID);
    if (ei != m_mapRemovables.end())
    {
        while ((ei->first == nBuildingID) && ei != m_mapRemovables.end())
        {
            if (ei->second == nodeIndex)
            {
                if (!bRemovable)
                {
                    m_mapRemovables.erase(ei);
                }
                return;
            }
            ei++;
        }
    }
    //not found
    if (bRemovable)
    {
        m_mapRemovables.insert(EntranceMap::iterator::value_type(nBuildingID, nodeIndex));
    }
}

//====================================================================
// AddIndoorEntrance
// adds an entrance for easy traversing later
//====================================================================
void CGraph::AddIndoorEntrance(int nBuildingID, unsigned nodeIndex, bool bExitOnly)
{
    GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

    if (!CGraph::ValidateNode(nodeIndex, false))
    {
        AIError("CGraph::AddIndoorEntrance Attempt to add indoor link to node that isn't created [Code bug]");
        return;
    }

    if (m_pSafeFirst)
    {
        if (!m_pSafeFirst->firstLinkIndex)
        {
            return;
        }
    }

    if (bExitOnly)
    {
        m_mapExits.insert(EntranceMap::iterator::value_type(nBuildingID, nodeIndex));
    }
    else
    {
        m_mapEntrances.insert(EntranceMap::iterator::value_type(nBuildingID, nodeIndex));
    }

    bool bAlreadyOutsideLink = false;
    for (unsigned link = pNode->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
    {
        unsigned int nextNodeIndex = GetLinkManager().GetNextNode(link);
        GraphNode* pNextNode = GetNodeManager().GetNode(nextNodeIndex);
        if (pNextNode->navType == IAISystem::NAV_TRIANGULAR)
        {
            bAlreadyOutsideLink = true;
        }
    }

    float radius = 100.0f;
    const SpecialArea* pSA = m_pNavigation->GetSpecialArea(nBuildingID);
    if (!pSA)
    {
        AIWarning("CGraph::AddIndoorEntrance cannot find area from building ID %d", nBuildingID);
    }
    else if (!pSA->bVehiclesInHumanNav)
    {
        radius = 2.0f;
    }

    if (!bAlreadyOutsideLink)
    {
        // it has to be connected to the outside
        unsigned outsideNodeIndex = m_pNavigation->GetTriangularNavRegion()->GetEnclosing(pNode->GetPos());
        GraphNode* pOutsideNode = GetNode(outsideNodeIndex);
        if (pOutsideNode)
        {
            if (!m_pNavigation->IsPointInForbiddenRegion(pOutsideNode->GetPos()))
            {
                Connect(nodeIndex, outsideNodeIndex, radius, bExitOnly ? 0.0f : radius);
            }
        }
        else
        {
            AIWarning("Unable to connect indoor entrance to outside");
        }
    }
}

//====================================================================
// ReadFromFile
// Reads the AI graph from a specified file
//====================================================================
bool CGraph::ReadFromFile(const char* szName)
{
    CCryFile file;
    ;
    if (file.Open(szName, "rb"))
    {
        return ReadNodes(file);
    }
    else
    {
        return false;
    }
}

//====================================================================
// ReadNodes
// reads all the nodes in a map
//====================================================================
bool CGraph::ReadNodes(CCryFile& file)
{
    //AIAssert(_heapchk()==_HEAPOK);
    int iNumber;
    Vec3 mins, maxs;

    AILogLoading("Verifying BAI file version");
    file.ReadType(&iNumber);
    if (iNumber != BAI_TRI_FILE_VERSION)
    {
        AIError("CGraph::ReadNodes Wrong triangulation BAI file version - found %d expected %d: Regenerate triangulation in the editor [Design bug]", iNumber, BAI_TRI_FILE_VERSION);
        return false;
    }

    AILogLoading("Reading BBOX");
    file.ReadType(&mins);
    file.ReadType(&maxs);

    SetBBox(mins, maxs);

    AILogLoading("Reading node descriptors");

    file.ReadType(&iNumber);

    if (iNumber > 0)
    {
        m_vNodeDescs.resize(iNumber);
        file.ReadType(&m_vNodeDescs[0], iNumber);
    }

    AILogLoading("Creating graph nodes");

    typedef std::map< unsigned, unsigned > ReadNodesMap;
    ReadNodesMap mapReadNodes;

    I3DEngine* pEngine = gEnv->pSystem->GetI3DEngine();

    NodeDescBuffer::iterator ni;
    int index = 0;

    if (/*!gEnv->IsEditor() && */ iNumber)
    {
        //int nodeCounts[IAISystem::NAV_TYPE_COUNT] = {0};

        NodeDescBuffer::iterator end = m_vNodeDescs.end();
        //for (ni=m_vNodeDescs.begin();ni != end;++ni,++index)
        //{
        //  NodeDescriptor desc = (*ni);
        //  IAISystem::ENavigationType type = (IAISystem::ENavigationType) desc.navType;

        //  int typeIndex = TypeIndexFromType(type);

        //  // in editor waypoint nodes get added by the editor
        //  // MichaelS - removed check to improve stats consistency.
        //  if (typeIndex < 0 ||
        //      (/*type != IAISystem::NAV_TRIANGULAR || */type != IAISystem::NAV_UNSET && gEnv->IsEditor()))
        //      continue;

        //  ++nodeCounts[typeIndex];
        //}

        for (ni = m_vNodeDescs.begin(); ni != end; ++ni, ++index)
        {
            NodeDescriptor desc = (*ni);
            IAISystem::ENavigationType type = (IAISystem::ENavigationType) desc.navType;

            // in editor waypoint nodes get added by the editor
            if (type != IAISystem::NAV_TRIANGULAR && type != IAISystem::NAV_UNSET && gEnv->IsEditor())
            {
                continue;
            }
            unsigned nodeIndex = CreateNewNode(type, desc.pos, desc.ID);
            GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

            switch (pNode->navType)
            {
            case IAISystem::NAV_UNSET:
                break;
            case IAISystem::NAV_TRIANGULAR:
            {
                ClearVectorMemory(pNode->GetTriangularNavData()->vertices);
                int nObstacles = 0;
                if (desc.obstacle[2] >= 0)
                {
                    nObstacles = 3;
                }
                else if (desc.obstacle[1] >= 0)
                {
                    nObstacles = 2;
                }
                else if (desc.obstacle[0] >= 0)
                {
                    nObstacles = 1;
                }
                if (nObstacles)
                {
                    pNode->GetTriangularNavData()->vertices.reserve(nObstacles);
                    for (int i = 0; i < nObstacles; i++)
                    {
                        int nVertexIndex = desc.obstacle[i];
                        pNode->GetTriangularNavData()->vertices.push_back(nVertexIndex);
                        if (!m_pNavigation->GetVertexList().IsIndexValid(nVertexIndex))
                        {
                            AIError("Invalid obstacle index %d on loading", nVertexIndex);
                        }
                    }
                }
                pNode->GetTriangularNavData()->isForbidden = desc.bForbidden;
                pNode->GetTriangularNavData()->isForbiddenDesigner = desc.bForbiddenDesigner;
            }
            break;
            case IAISystem::NAV_WAYPOINT_3DSURFACE:
            case IAISystem::NAV_WAYPOINT_HUMAN:
            {
                pNode->GetWaypointNavData()->type = (EWaypointNodeType) desc.type;

                // building id isn't preserved
                pNode->GetWaypointNavData()->nBuildingID = -1;
                pNode->GetWaypointNavData()->pArea = 0;
                SpecialArea::EType areaType = pNode->navType == IAISystem::NAV_WAYPOINT_HUMAN ? SpecialArea::TYPE_WAYPOINT_HUMAN : SpecialArea::TYPE_WAYPOINT_3DSURFACE;
                const SpecialArea* sa = m_pNavigation->GetSpecialArea(pNode->GetPos(), areaType);
                if (sa)
                {
                    pNode->GetWaypointNavData()->nBuildingID = sa->nBuildingID;
                    I3DEngine* p3dEngine = gEnv->pSystem->GetI3DEngine();
                    pNode->GetWaypointNavData()->pArea = p3dEngine->GetVisAreaFromPos(pNode->GetPos());
                }

                if (desc.type == WNT_ENTRYEXIT)
                {
                    m_mapEntrances.insert(EntranceMap::iterator::value_type(pNode->GetWaypointNavData()->nBuildingID, nodeIndex));
                }

                if (desc.type == WNT_ENTRYEXIT || desc.type == WNT_EXITONLY)
                {
                    m_mapExits.insert(EntranceMap::iterator::value_type(pNode->GetWaypointNavData()->nBuildingID, nodeIndex));
                }

                if (desc.bRemovable)
                {
                    m_mapRemovables.insert(EntranceMap::iterator::value_type(pNode->GetWaypointNavData()->nBuildingID, nodeIndex));
                }

                pNode->GetWaypointNavData()->dir = desc.dir;
                pNode->GetWaypointNavData()->up = desc.up;
            }
            break;
            case IAISystem::NAV_FLIGHT:
                AIAssert(!"flight nav should be loaded separately");
                pNode->GetFlightNavData()->nSpanIdx = desc.index;
                break;
            case IAISystem::NAV_VOLUME:
                AIAssert(!"volume nav should be loaded separately");
                pNode->GetVolumeNavData()->nVolimeIdx = desc.index;
                break;
            case IAISystem::NAV_ROAD:
                AIAssert(!"road nav should be loaded separately");
                pNode->GetRoadNavData()->fRoadWidth = desc.dir.x;
                /*pNode->GetRoadNavData()->fRoadOffset = desc.dir.y;*/
                break;
            case IAISystem::NAV_SMARTOBJECT:
                AIAssert(!"smart object nav should be loaded separately");
                break;
            default:
                AIError("CGraph::ReadNodes Unhandled nav type %d", pNode->navType);
            }
            mapReadNodes.insert(ReadNodesMap::value_type((ReadNodesMap::key_type)desc.ID, nodeIndex));
        }
    }

    AILogLoading("Reading links");

    file.ReadType(&iNumber);
    if (iNumber > 0)
    {
        m_vLinkDescs.resize(iNumber);
        file.ReadType(&m_vLinkDescs[0], iNumber);
    }

    ReadNodesMap::iterator ei, link;

    ei = mapReadNodes.find(1);
    if (ei == mapReadNodes.end())
    {
        AIError("CGraph::ReadNodes First node not found - navigation loading failed [code bug]");
        gEnv->pSystem->GetILog()->UpdateLoadingScreen(" ");
        return false;
    }
    else
    {
        if (m_pSafeFirst)
        {
            Disconnect(m_safeFirstIndex);
        }
        m_safeFirstIndex = (ei->second);
        m_pSafeFirst = GetNodeManager().GetNode(m_safeFirstIndex);
        m_pFirst = m_pSafeFirst;
        m_firstIndex = m_safeFirstIndex;
        m_pCurrent = m_pSafeFirst;
        m_currentIndex = m_safeFirstIndex;
    }


    float fStartTime = gEnv->pSystem->GetITimer()->GetAsyncCurTime();
    AILogLoading("Reconnecting links");

    if (!m_vLinkDescs.empty())
    {
        LinkDescBuffer::iterator iend = m_vLinkDescs.end();
        for (LinkDescBuffer::iterator ldbi = m_vLinkDescs.begin(); ldbi != iend; ++ldbi)
        {
            LinkDescriptor& ldb = (*ldbi);
            if (!ldb.bIsPureTriangularLink && gEnv->IsEditor())
            {
                continue;
            }

            ReadNodesMap::key_type srcKey = static_cast<ReadNodesMap::key_type>(ldb.nSourceNode);
            ei = mapReadNodes.find(srcKey);
            if (ei == mapReadNodes.end())
            {
#if defined __GNUC__
                AIWarning("NodeId %lld not found in node map", (long long)ldb.nSourceNode);
#else
                AIWarning("NodeId %I64d not found in node map", (int64)ldb.nSourceNode);
#endif
            }
            else
            {
                unsigned nodeIndex = ei->second;
                GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);
                ReadNodesMap::key_type targetKey = static_cast<ReadNodesMap::key_type>(ldb.nSourceNode);
                link = mapReadNodes.find((ReadNodesMap::key_type)ldb.nTargetNode);
                if (link == mapReadNodes.end() || ei == mapReadNodes.end())
                {
                    AIError("CGraph::ReadNodes Read a link to a node which could not be found [Code bug]");
                }
                else
                {
                    if (pNode != m_pSafeFirst && link->second != m_safeFirstIndex)
                    {
                        unsigned linkOneTwo = 0;
                        unsigned linkTwoOne = 0;
                        // incoming link gets set when the other way is read
                        Connect(nodeIndex, link->second, ldb.fMaxPassRadius, 0.0f, &linkOneTwo, &linkTwoOne);
                        if (linkOneTwo)
                        {
                            unsigned nextNodeIndex = link->second;
                            GraphNode* pNextNode = GetNodeManager().GetNode(nextNodeIndex);
                            AIAssert(GetLinkManager().GetNextNode(linkOneTwo) == nextNodeIndex);

                            float radius = ldb.fMaxPassRadius;

                            // Marcio: Hax for Crysis 2 Critters
                            // ... but only if the link is passable - hence condition (radius > 0.f) [2/1/2011 evgeny]
                            if ((radius > 0.f) && (pNode->navType == IAISystem::NAV_WAYPOINT_HUMAN) && (pNextNode->navType == IAISystem::NAV_WAYPOINT_HUMAN))
                            {
                                const SpecialArea* sa1 = m_pNavigation->GetSpecialArea(pNode->GetWaypointNavData()->nBuildingID);
                                if (sa1->bCritterOnly)
                                {
                                    radius = 0.150001f;
                                }
                                else
                                {
                                    const SpecialArea* sa2 = m_pNavigation->GetSpecialArea(pNextNode->GetWaypointNavData()->nBuildingID);
                                    if (sa2->bCritterOnly)
                                    {
                                        radius = 0.150001f;
                                    }
                                }
                            }

                            if (GetLinkManager().GetNextNode(linkOneTwo) == link->second)
                            {
                                GetLinkManager().SetRadius(linkOneTwo, radius);
                                GetLinkManager().SetStartIndex(linkOneTwo, ldb.nStartIndex);
                                GetLinkManager().SetEndIndex(linkOneTwo, ldb.nEndIndex);
                                GetLinkManager().GetEdgeCenter(linkOneTwo) = ldb.vEdgeCenter; // TODO: Don't read shared value twice
                                GetLinkManager().SetExposure(linkOneTwo, ldb.fExposure); // TODO: Don't read shared value twice
                                GetLinkManager().SetMaxWaterDepth(linkOneTwo, ldb.fMaxWaterDepth);
                                GetLinkManager().SetMinWaterDepth(linkOneTwo, ldb.fMinWaterDepth);
                                GetLinkManager().SetSimple(linkOneTwo, ldb.bSimplePassabilityCheck);
                            }
                            if (linkTwoOne)
                            {
                                GetLinkManager().RestoreLink(linkTwoOne);
                            }
                        }
                    }
                }
            }
        }
    }

    AILogLoading("Finished Reconnecting links in %6.3f sec",
        gEnv->pSystem->GetITimer()->GetAsyncCurTime() - fStartTime);

    ClearVectorMemory(m_vNodeDescs);
    ClearVectorMemory(m_vLinkDescs);
    mapReadNodes.clear();
    //AIAssert(_heapchk()==_HEAPOK);

    return true;
}

//====================================================================
// SetBBox
// defines bounding rectangle of this graph
//====================================================================
void CGraph::SetBBox(const Vec3& min, const Vec3& max)
{
    m_triangularBBox.min = min;
    m_triangularBBox.max = max;
}

//====================================================================
// InsideOfBBox
//====================================================================
bool CGraph::InsideOfBBox(const Vec3& pos) const
{
    return pos.x > m_triangularBBox.min.x && pos.x < m_triangularBBox.max.x && pos.y > m_triangularBBox.min.y && pos.y < m_triangularBBox.max.y;
}

//====================================================================
// RemoveEntrance
//====================================================================
bool CGraph::RemoveEntrance(int nBuildingID, unsigned nodeIndex)
{
    GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

    if (!CGraph::ValidateNode(nodeIndex, false))
    {
        AIError("CGraph::RemoveEntrance Attempt to remove entrance from node that isn't created [Code bug]");
        return false;
    }

    EntranceMap::iterator ei = m_mapEntrances.find(nBuildingID);
    if (ei != m_mapEntrances.end())
    {
        while ((ei->first == nBuildingID) && ei != m_mapEntrances.end())
        {
            if (ei->second == nodeIndex)
            {
                m_mapEntrances.erase(ei);
                return true;
            }
            ei++;
        }
    }

    ei = m_mapExits.find(nBuildingID);
    if (ei != m_mapExits.end())
    {
        while ((ei->first == nBuildingID) && ei != m_mapExits.end())
        {
            if (ei->second == nodeIndex)
            {
                m_mapExits.erase(ei);
                return true;
            }
            ei++;
        }
    }
    return false;
}



unsigned CGraph::CreateNewNode(IAISystem::ENavigationType type, const Vec3& pos, unsigned ID)
{
    //GraphNode *pNode = new GraphNode(type, pos, ID);
    //GraphNode *pNode = NodesPool.AddGraphNode(type, pos, ID);
    unsigned nodeIndex = m_pGraphNodeManager->CreateNode(type, pos, ID);
    GraphNode* pNode = m_pGraphNodeManager->GetNode(nodeIndex);
    pNode->AddRef();
    m_allNodes.AddNode(nodeIndex);

    // TODO Mrz 5, 2008: <pvl> this function is called from Graph constructor
    // (with NAV_UNSET as type) while creating the "safe first node".  Graph ctor
    // is run from AISystem ctor before Navigation ctor is called so m_pNavigation
    // is still 0 at that point.  That makes it hard to call
    // CNavigation::GetNavRegion() from here.
    // Check to see if there's a better solution.
    if (type != IAISystem::NAV_UNSET)
    {
        CNavRegion* pNavRegion = m_pNavigation->GetNavRegion(pNode->navType, this);
        if (pNavRegion)
        {
            pNavRegion->NodeCreated(nodeIndex);
        }
    }

    return nodeIndex;
}

//====================================================================
// GetNode
//====================================================================
GraphNode* CGraph::GetNode(unsigned index)
{
    return GetNodeManager().GetNode(index);
}

//====================================================================
// MoveNode
//====================================================================
void CGraph::MoveNode(unsigned nodeIndex, const Vec3& newPos)
{
    GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);
    if (pNode->GetPos().IsEquivalent(newPos))
    {
        return;
    }
    m_allNodes.RemoveNode(nodeIndex);
    pNode->SetPos(newPos);
    m_allNodes.AddNode(nodeIndex);

    CNavRegion* pNavRegion = m_pNavigation->GetNavRegion(pNode->navType, this);
    if (pNavRegion)
    {
        pNavRegion->NodeMoved(nodeIndex);
    }
}

struct SNodeFinder
{
    SNodeFinder(unsigned nodeIndex)
        : nodeIndex(nodeIndex) {}
    bool operator()(const EntranceMap::value_type& val) const {return val.second == nodeIndex; }
    unsigned nodeIndex;
};

//====================================================================
// DeleteNode
//====================================================================
void CGraph::DeleteNode(unsigned nodeIndex)
{
    GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

    if (!CGraph::ValidateNode(nodeIndex, false))
    {
        AIError("CGraph::DeleteNode Attempting to delete node that doesn't exist %p [Code bug]", pNode);
        return;
    }

    if (pNode->firstLinkIndex)
    {
        AIWarning("Deleting node %p but it is still connected - disconnecting", pNode);
        Disconnect(nodeIndex, false);
    }

    if (pNode->Release())
    {
        CNavRegion* pNavRegion = m_pNavigation->GetNavRegion(pNode->navType, this);
        if (pNavRegion)
        {
            pNavRegion->NodeAboutToBeDeleted(pNode);
        }

        m_allNodes.RemoveNode(nodeIndex);

        VectorConstNodeIndices::iterator it;
        it = std::remove(m_taggedNodes.begin(), m_taggedNodes.end(), nodeIndex);
        m_taggedNodes.erase(it, m_taggedNodes.end());
        it = std::remove(m_markedNodes.begin(), m_markedNodes.end(), nodeIndex);
        m_markedNodes.erase(it, m_markedNodes.end());

        EntranceMap::iterator entranceIt;
        entranceIt = std::find_if(m_mapEntrances.begin(), m_mapEntrances.end(), SNodeFinder(nodeIndex));
        if (entranceIt != m_mapEntrances.end())
        {
            m_mapEntrances.erase(entranceIt);
        }
        entranceIt = std::find_if(m_mapExits.begin(), m_mapExits.end(), SNodeFinder(nodeIndex));
        if (entranceIt != m_mapExits.end())
        {
            m_mapExits.erase(entranceIt);
        }
        entranceIt = std::find_if(m_mapRemovables.begin(), m_mapRemovables.end(), SNodeFinder(nodeIndex));
        if (entranceIt != m_mapRemovables.end())
        {
            m_mapRemovables.erase(entranceIt);
        }

        // no waypoint nav region when AI system being deleted
        if (m_pNavigation->GetWaypointHumanNavRegion())
        {
            m_pNavigation->GetWaypointHumanNavRegion()->ResetUpdateStatus();
        }

        m_pGraphNodeManager->DestroyNode(nodeIndex);
    }
}


struct SVolumeHideSpotFinder
{
    SVolumeHideSpotFinder(ListObstacles& obs)
        : obs(obs) {}

    void operator()(const SVolumeHideSpot& hs, float)
    {
        obs.push_back(ObstacleData(hs.pos, hs.dir));
    }

private:
    ListObstacles& obs;
};

//====================================================================
// ResolveLinkData
//====================================================================
void CGraph::ResolveLinkData(GraphNode* pOne, GraphNode* pTwo)
{
    if (pOne->navType != IAISystem::NAV_TRIANGULAR || pTwo->navType != IAISystem::NAV_TRIANGULAR)
    {
        return;
    }

    if (pOne->GetTriangularNavData()->vertices.empty() || pTwo->GetTriangularNavData()->vertices.empty())
    {
        return;
    }

    int iOneEdge1 = -1, iOneEdge2 = -1;
    int iTwoEdge1 = -1, iTwoEdge2 = -1;

    // todo Danny this seems like a really obscure way of getting the vertex indices - and at the end we
    // still don't necessarily know the winding! The original triangle verts should be wound anti-clockwise
    // so we should just find a matching index pair (between nodes) - then can get the other pair by looking +/- 1
    // that way we can also make sure that the link stores the vertex indices in an anti-clockwise sense.
    for (unsigned int i = 0; i < pOne->GetTriangularNavData()->vertices.size(); i++)
    {
        for (unsigned int j = 0; j < pTwo->GetTriangularNavData()->vertices.size(); j++)
        {
            if (i != iOneEdge1 && j != iTwoEdge1 && pOne->GetTriangularNavData()->vertices[i] == pTwo->GetTriangularNavData()->vertices[j])
            {
                if (iOneEdge1 < 0)
                {
                    iOneEdge1 = i;
                    iTwoEdge1 = j;
                }
                else
                {
                    iOneEdge2 = i;
                    iTwoEdge2 = j;

                    // exit both loops
                    i = pOne->GetTriangularNavData()->vertices.size();
                }

                // process next vertex of pOne
                break;
            }
        }
    }

    // Are you trying to ResolveLinkData(...) between non-linked nodes?
    AIAssert(iOneEdge1 >= 0 && iOneEdge1 <= 2);
    AIAssert(iOneEdge2 >= 0 && iOneEdge2 <= 2);
    AIAssert(iTwoEdge1 >= 0 && iTwoEdge1 <= 2);
    AIAssert(iTwoEdge2 >= 0 && iTwoEdge2 <= 2);

    // find which link from one goes to two
    const CVertexList& vertexList  = m_pNavigation->GetVertexList();
    for (unsigned link = pOne->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
    {
        if (GetNodeManager().GetNode(GetLinkManager().GetNextNode(link)) == pTwo)
        {
            GetLinkManager().SetStartIndex(link, iOneEdge1);
            GetLinkManager().SetEndIndex(link, iOneEdge2);

            Vec3 edge = vertexList.GetVertex(pOne->GetTriangularNavData()->vertices[iOneEdge1]).vPos
                - vertexList.GetVertex(pOne->GetTriangularNavData()->vertices[iOneEdge2]).vPos;
            GetLinkManager().GetEdgeCenter(link) = vertexList.GetVertex(pOne->GetTriangularNavData()->vertices[iOneEdge1]).vPos - 0.5f * edge;
            AIAssert(GetLinkManager().GetEdgeCenter(link).GetLength() > 0.0f);
            break;
        }
    }

    for (unsigned link = pTwo->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
    {
        if (GetNodeManager().GetNode(GetLinkManager().GetNextNode(link)) == pOne)
        {
            GetLinkManager().SetStartIndex(link, iTwoEdge1);
            GetLinkManager().SetEndIndex(link, iTwoEdge2);

            Vec3 edge = vertexList.GetVertex(pTwo->GetTriangularNavData()->vertices[iTwoEdge1]).vPos
                - vertexList.GetVertex(pTwo->GetTriangularNavData()->vertices[iTwoEdge2]).vPos;

            GetLinkManager().GetEdgeCenter(link) = vertexList.GetVertex(pTwo->GetTriangularNavData()->vertices[iTwoEdge1]).vPos - 0.5f * edge;
            AIAssert(GetLinkManager().GetEdgeCenter(link).GetLength() > 0.0f);
            break;
        }
    }
}

//====================================================================
// ConnectNodes
//====================================================================
void CGraph::ConnectNodes(ListNodeIds& lstNodes)
{
    // clear degenerate triangles
    ListNodeIds::iterator it;
    for (it = lstNodes.begin(); it != lstNodes.end(); )
    {
        GraphNode* pCurrent = GetNodeManager().GetNode(*it);
        if (!pCurrent || pCurrent->navType != IAISystem::NAV_TRIANGULAR)
        {
            ++it;
            continue;
        }
        ObstacleIndexVector::iterator obi, obi2;
        bool bRemoved = false;

        for (obi = pCurrent->GetTriangularNavData()->vertices.begin(); obi != pCurrent->GetTriangularNavData()->vertices.end(); ++obi)
        {
            for (obi2 = pCurrent->GetTriangularNavData()->vertices.begin(); obi2 != pCurrent->GetTriangularNavData()->vertices.end(); ++obi2)
            {
                if (obi == obi2)
                {
                    continue;
                }
                if ((*obi) == (*obi2))
                {
                    it = lstNodes.erase(it);
                    bRemoved = true;
                    break;
                }
            }
            if (bRemoved)
            {
                break;
            }
        }
        if (!bRemoved)
        {
            it++;
        }
    }


    // reconnect triangles in nodes list
    ListNodeIds::iterator it1, it2;
    m_pCurrent = 0;
    m_currentIndex = 0;
    for (it1 = lstNodes.begin(); it1 != lstNodes.end(); it1++)
    {
        unsigned currentIndex = *it1;
        GraphNode* pCurrent = GetNodeManager().GetNode(*it1);
        for (it2 = lstNodes.begin(); it2 != lstNodes.end(); it2++)
        {
            unsigned candidateIndex = *it2;
            GraphNode* pCandidate = GetNodeManager().GetNode(*it2);
            ObstacleIndexVector::iterator obCur, obNext;
            for (obCur = pCurrent->GetTriangularNavData()->vertices.begin(); obCur != pCurrent->GetTriangularNavData()->vertices.end(); obCur++)
            {
                obNext = obCur;
                obNext++;
                if (obNext == pCurrent->GetTriangularNavData()->vertices.end())
                {
                    obNext = pCurrent->GetTriangularNavData()->vertices.begin();
                }

                ObstacleIndexVector::iterator obCan, obNextCan;
                for (obCan = pCandidate->GetTriangularNavData()->vertices.begin(); obCan != pCandidate->GetTriangularNavData()->vertices.end(); obCan++)
                {
                    obNextCan = obCan;
                    obNextCan++;
                    if (obNextCan == pCandidate->GetTriangularNavData()->vertices.end())
                    {
                        obNextCan = pCandidate->GetTriangularNavData()->vertices.begin();
                    }

                    if (((*obCur) == (*obCan)) && ((*obNext) == (*obNextCan)))
                    {
                        if (pCandidate != pCurrent)
                        {
                            if (-1 == pCurrent->GetLinkIndex(GetNodeManager(), GetLinkManager(), pCandidate))
                            {
                                Connect(currentIndex, candidateIndex);
                            }
                            ResolveLinkData(pCurrent, pCandidate);
                        }
                    }

                    if (((*obCur) == (*obNextCan)) && ((*obNext) == (*obCan)))
                    {
                        if (pCandidate != pCurrent)
                        {
                            if (-1 == pCurrent->GetLinkIndex(GetNodeManager(), GetLinkManager(), pCandidate))
                            {
                                Connect(currentIndex, candidateIndex);
                            }
                            ResolveLinkData(pCurrent, pCandidate);
                        }
                    }
                }
            }
        }
    }

    for (it1 = lstNodes.begin(); it1 != lstNodes.end(); it1++)
    {
        GraphNode* pCurrent = GetNodeManager().GetNode(*it1);
        for (unsigned link = pCurrent->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            ResolveLinkData(pCurrent, GetNodeManager().GetNode(GetLinkManager().GetNextNode(link)));
        }
    }
}

//====================================================================
// FillGraphNodeData
//====================================================================
void CGraph::FillGraphNodeData(unsigned nodeIndex)
{
    GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

    I3DEngine* pEngine = gEnv->pSystem->GetI3DEngine();

    const STriangularNavData* pTriNavData = pNode->GetTriangularNavData();

    // get the triangle centre from the mean of the vertex points, weighting by the edge lengths.
    // this results in straighter unbeautified paths when traversing long/thin triangles
    Vec3 pos(ZERO);
    float totalLen = 0.0f;
    const ObstacleIndexVector::const_iterator oiEnd = pTriNavData->vertices.end();
    for (ObstacleIndexVector::const_iterator oi = pTriNavData->vertices.begin(); oi != oiEnd; ++oi)
    {
        ObstacleIndexVector::const_iterator oiNext = oi;
        if (++oiNext == oiEnd)
        {
            oiNext = pTriNavData->vertices.begin();
        }
        ObstacleIndexVector::const_iterator oiNextNext = oiNext;
        if (++oiNextNext == oiEnd)
        {
            oiNextNext = pTriNavData->vertices.begin();
        }

        const CVertexList& vertexList = m_pNavigation->GetVertexList();
        Vec3 pt = vertexList.GetVertex(*oi).vPos;
        Vec3 ptNext = vertexList.GetVertex(*oiNext).vPos;
        Vec3 ptNextNext = vertexList.GetVertex(*oiNextNext).vPos;

        float lenBefore = Distance::Point_Point2D(pt, ptNext);
        float lenAfter = Distance::Point_Point2D(ptNext, ptNextNext);
        totalLen += lenBefore;

        pos += ptNext * (lenBefore + lenAfter);
    }
    if (totalLen > 0.0f)
    {
        pos /= (2.0f * totalLen);
    }
    else if (!pTriNavData->vertices.empty())
    {
        pos = m_pNavigation->GetVertexList().GetVertex(pTriNavData->vertices.front()).vPos;
    }
    else
    {
        pos.zero();
    }

    // put it on the terrain (up a little bit)
    pos.z = 0.2f + pEngine->GetTerrainElevation(pos.x, pos.y);
    MoveNode(nodeIndex, pos);
}

//====================================================================
// GetCachedPassabilityResult
//====================================================================
SCachedPassabilityResult* CGraph::GetCachedPassabilityResult(unsigned graphLink, bool bCreate)
{
    PassabilityCache::iterator it = m_passabilityCache.find(graphLink);
    if (it == m_passabilityCache.end() && bCreate)
    {
        it = m_passabilityCache.insert(std::make_pair(graphLink, SCachedPassabilityResult())).first;
    }

    return (it != m_passabilityCache.end() ? &(*it).second : 0);
}

//====================================================================
// Reset
//====================================================================
void CGraph::Reset()
{
    ClearMarks();
    ClearTags();
    RestoreAllNavigation();
}

//====================================================================
// GetEntrance
//====================================================================
GraphNode* CGraph::GetEntrance(int nBuildingID, const Vec3& pos)
{
    GraphNode* pEntrance = 0;
    float mindist = 1000000;
    EntranceMap::iterator ei = m_mapEntrances.find(nBuildingID);
    if (ei != m_mapEntrances.end())
    {
        pEntrance = GetNodeManager().GetNode(ei->second);
        if (m_mapEntrances.count(nBuildingID) > 1)
        {
            mindist = (pEntrance->GetPos() - pos).GetLengthSquared();
            for (; ei != m_mapEntrances.end(); ei++)
            {
                if (ei->first != nBuildingID)
                {
                    break;
                }
                float curr_dist = (GetNodeManager().GetNode(ei->second)->GetPos() - pos).GetLengthSquared();
                if (curr_dist <= mindist)
                {
                    pEntrance = GetNodeManager().GetNode(ei->second);
                    mindist = curr_dist;
                }
            }
        }
    }

    ei = m_mapExits.find(nBuildingID);
    if (ei != m_mapExits.end())
    {
        pEntrance = GetNodeManager().GetNode(ei->second);
        if (m_mapExits.count(nBuildingID) > 1)
        {
            mindist = (pEntrance->GetPos() - pos).GetLengthSquared();
            for (; ei != m_mapExits.end(); ei++)
            {
                if (ei->first != nBuildingID)
                {
                    break;
                }
                float curr_dist = (GetNodeManager().GetNode(ei->second)->GetPos() - pos).GetLengthSquared();
                if (curr_dist <= mindist)
                {
                    pEntrance = GetNodeManager().GetNode(ei->second);
                    mindist = curr_dist;
                }
            }
        }
    }


    return pEntrance;
}

//====================================================================
// GetEntrances
//====================================================================
bool CGraph::GetEntrances(int nBuildingID, const Vec3& pos, std::vector<unsigned>& nodes)
{
    EntranceMap::iterator it1 = m_mapEntrances.lower_bound(nBuildingID);
    EntranceMap::iterator it2 = m_mapEntrances.upper_bound(nBuildingID);
    for (EntranceMap::iterator it = it1; it != it2; ++it)
    {
        nodes.push_back(it->second);
    }
    it1 = m_mapExits.lower_bound(nBuildingID);
    it2 = m_mapExits.upper_bound(nBuildingID);
    for (EntranceMap::iterator it = it1; it != it2; ++it)
    {
        nodes.push_back(it->second);
    }
    return !nodes.empty();
}

//====================================================================
// RestoreAllNavigation
// for all removable nodes restore links passibility etc
//====================================================================
void CGraph::RestoreAllNavigation()
{
    CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAVMASK_ALL);
    while (GraphNode* node = GetNodeManager().GetNode(it.Increment()))
    {
        for (unsigned link = node->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
        {
            GetLinkManager().RestoreLink(link);
            PassabilityCache cache;
            m_passabilityCache.swap(cache);
        }
    }
}

//====================================================================
// IsPointInRegions
//====================================================================
static bool IsPointInRegions(const Vec3& p, const CGraph::CRegions& regions)
{
    if (!regions.m_AABB.IsContainSphere(p, 0.0f))
    {
        return false;
    }
    for (std::list<CGraph::CRegion>::const_iterator regionIt = regions.m_regions.begin();
         regionIt != regions.m_regions.end(); ++regionIt)
    {
        const CGraph::CRegion& region = *regionIt;
        if (region.m_AABB.IsContainSphere(p, 0.0f))
        {
            if (Overlap::Point_Polygon2D(p, region.m_outline, &region.m_AABB))
            {
                return true;
            }
        }
    }
    return false;
}

//====================================================================
// DoesTriangleOverlapRegions
//====================================================================
static bool DoesTriangleOverlapRegions(const Vec3& v0, const Vec3& v1, const Vec3& v2,
    const CGraph::CRegions& regions)
{
    AABB triAABB(AABB::RESET);
    triAABB.Add(v0);
    triAABB.Add(v1);
    triAABB.Add(v2);
    triAABB.min.z = -std::numeric_limits<float>::max();
    triAABB.max.z = std::numeric_limits<float>::max();

    if (!regions.m_AABB.IsIntersectBox(triAABB))
    {
        return false;
    }

    for (std::list<CGraph::CRegion>::const_iterator regionIt = regions.m_regions.begin();
         regionIt != regions.m_regions.end();
         ++regionIt)
    {
        const CGraph::CRegion& region = *regionIt;
        if (region.m_AABB.IsIntersectBox(triAABB))
        {
            if (Overlap::Triangle_Polygon2D(v0, v1, v2, region.m_outline))
            {
                return true;
            }
        }
    }
    return false;
}

//====================================================================
// CheckForEmpty
//====================================================================
bool CGraph::CheckForEmpty(IAISystem::tNavCapMask navTypeMask) const
{
    unsigned count = 0;
    CAllNodesContainer::Iterator it(m_allNodes, navTypeMask);
    while (const GraphNode* node = GetNodeManager().GetNode(it.Increment()))
    {
        ++count;
        AILogEvent("Unexpected Node %p, type = %d, pos = (%5.2f %5.2f %5.2f)", node, node->navType,
            node->GetPos().x, node->GetPos().y, node->GetPos().z);
    }
    if (count)
    {
        AIWarning("Detected %d unexpected nodes whilst checking types %u", count, static_cast<unsigned int>(navTypeMask));
    }
    return (count == 0);
}

//====================================================================
// Validate
//====================================================================
bool CGraph::Validate(const char* msg, bool checkPassable) const
{
#ifdef _DEBUG
    return true;
#endif

    if (!AIGetWarningErrorsEnabled())
    {
        AILogEvent("CGraph::Validate Skipping: %s", msg);
        return true;
    }

    AILogProgress("CGraph::Validate Starting: %s", msg);

    // Danny todo tweak this so that if !checkPassable then we only clear
    // errors that are not to do with the passable checks...
    if (checkPassable)
    {
        mBadGraphData.clear();
    }

    if (!m_pFirst)
    {
        return false;
    }

    if (!m_pSafeFirst->firstLinkIndex)
    {
        return true;
    }

    // the first safe node is different - expect only one link
    if (GetLinkManager().GetNextLink(m_pSafeFirst->firstLinkIndex) != 0)
    {
        AIWarning("CGraph::Validate Expect only 1 link to the first safe node");
        return false;
    }

    int badOutdoorNodes = 0;
    int badIndoorNodes = 0;
    int badLinks = 0;
    int badNumOutsideLinks = 0;
    int badLinkPassability = 0;

    static int maxBadImassabilityWarnings = 10;

    // nodes containing vertices with identical indices
    int indexDegenerates = 0;
    // nodes containing vertices that have different indices, but essentially
    // the same position.
    int posDegenerates = 0;

    CAllNodesContainer::Iterator it(m_allNodes, IAISystem::NAVMASK_ALL);
    while (unsigned nodeIndex = it.Increment())
    {
        const GraphNode* node = GetNodeManager().GetNode(nodeIndex);
        if (!CGraph::ValidateNode(nodeIndex, true))
        {
            AIWarning("CGraph::Validate Node validation failed: %p", node);
        }

        if (node->navType == IAISystem::NAV_WAYPOINT_HUMAN)
        {
            if (node->GetWaypointNavData()->nBuildingID == -1)
            {
                AIWarning("Node at position: (%5.2f, %5.2f, %5.2f) is not in a human waypoint nav modifier [design bug]",
                    node->GetPos().x, node->GetPos().y, node->GetPos().z);
                ++badIndoorNodes;
            }
        }
        else if (node->navType == IAISystem::NAV_WAYPOINT_3DSURFACE)
        {
            if (node->GetWaypointNavData()->nBuildingID == -1)
            {
                AIWarning("Node at position: (%5.2f, %5.2f, %5.2f) is not in a 3d surface nav modifier [design bug]",
                    node->GetPos().x, node->GetPos().y, node->GetPos().z);
                ++badIndoorNodes;
            }
        }
        else if (node->navType == IAISystem::NAV_TRIANGULAR)
        {
            unsigned numVertices = node->GetTriangularNavData()->vertices.size();
            if (numVertices != 3)
            {
                ++badOutdoorNodes;
            }

            // check for degenerates
            for (unsigned iV = 0; iV < numVertices; ++iV)
            {
                unsigned iNext = iV + 1;
                if (iNext == numVertices)
                {
                    iNext = 0;
                }

                CVertexList& vertexList = m_pNavigation->GetVertexList();
                if (node->GetTriangularNavData()->vertices[iV] == node->GetTriangularNavData()->vertices[iNext])
                {
                    const Vec3& v = vertexList.GetVertex(node->GetTriangularNavData()->vertices[iV]).vPos;
                    AIWarning("Degenerate vertex at position: (%5.2f, %5.2f, %5.2f)", v.x, v.y, v.z);
                    ++indexDegenerates;
                }
                else
                {
                    const Vec3& v0 = vertexList.GetVertex(node->GetTriangularNavData()->vertices[iV]).vPos;
                    const Vec3& v1 = vertexList.GetVertex(node->GetTriangularNavData()->vertices[iNext]).vPos;
                    float distTol = 0.001f;
                    if ((v0 - v1).GetLengthSquared() < distTol * distTol)
                    {
                        ++posDegenerates;
                    }
                }
            }
            // store if we're in forbidden so we can check boundaries
            bool nodeInForbidden = node->GetTriangularNavData()->isForbidden;

            for (unsigned link = node->firstLinkIndex; link; link = GetLinkManager().GetNextLink(link))
            {
                if (GetLinkManager().GetStartIndex(link) != GetLinkManager().GetEndIndex(link) && GetLinkManager().GetEdgeCenter(link).GetLength() == 0.0f)
                {
                    ++badLinks;
                }

                const GraphNode* next = GetNodeManager().GetNode(GetLinkManager().GetNextNode(link));
                AIAssert(next);

                // check that there's a link back...
                unsigned nextLink;
                for (nextLink = next->firstLinkIndex; nextLink; nextLink = GetLinkManager().GetNextLink(nextLink))
                {
                    if (GetLinkManager().GetNextNode(nextLink) == nodeIndex)
                    {
                        break;
                    }
                }
                if (!nextLink)
                {
                    ++badNumOutsideLinks;
                }

                // check if it's impassable:
                if (checkPassable &&
                    next != m_pSafeFirst &&
                    (GetLinkManager().GetStartIndex(link) != GetLinkManager().GetEndIndex(link) && !GetLinkManager().GetEdgeCenter(link).IsZero()))
                {
                    const CVertexList& vertexList = m_pNavigation->GetVertexList();
                    Vec3 vStart = vertexList.GetVertex(node->GetTriangularNavData()->vertices[GetLinkManager().GetStartIndex(link)]).vPos;
                    Vec3 vEnd = vertexList.GetVertex(node->GetTriangularNavData()->vertices[GetLinkManager().GetEndIndex(link)]).vPos;

                    bool nextInForbidden = next->GetTriangularNavData()->isForbidden;
                    if (GetLinkManager().GetRadius(link) > 0.0f && nextInForbidden && !nodeInForbidden)
                    {
                        mBadGraphData.push_back(SBadGraphData());
                        mBadGraphData.back().mType = SBadGraphData::BAD_IMPASSABLE;
                        mBadGraphData.back().mPos1 = vStart;
                        mBadGraphData.back().mPos2 = vEnd;
                        ++badLinkPassability;
                        if (badLinkPassability <= maxBadImassabilityWarnings)
                        {
                            AIWarning("Bad link passability at position: (%5.2f %5.2f %5.2f), linkID %#x - see if tweaking the object positions there helps",
                                0.5f * (vStart.x + vEnd.x), 0.5f * (vStart.y + vEnd.y), 0.5f * (vStart.z + vEnd.z), LinkId (link));
                        }
                    }
                    else if (GetLinkManager().GetRadius(link) == -1.0f && !nextInForbidden && nodeInForbidden)
                    {
                        mBadGraphData.push_back(SBadGraphData());
                        mBadGraphData.back().mType = SBadGraphData::BAD_PASSABLE;
                        mBadGraphData.back().mPos1 = vStart;
                        mBadGraphData.back().mPos2 = vEnd;
                        ++badLinkPassability;
                        if (badLinkPassability <= maxBadImassabilityWarnings)
                        {
                            AIWarning("Bad link passability at position: (%5.2f %5.2f %5.2f), linkID %#x - see if tweaking the object positions there helps",
                                0.5f * (vStart.x + vEnd.x), 0.5f * (vStart.y + vEnd.y), 0.5f * (vStart.z + vEnd.z), LinkId (link));
                        }
                    }
                }
            }
        }
    }

    if (badOutdoorNodes + badIndoorNodes + badNumOutsideLinks + indexDegenerates + posDegenerates + badLinkPassability > 0)
    {
        AIError("CGraph::Validate AI graph contains invalid: %d outdoor, %d indoor, %d outdoor links, %d degenerate index, %d degenerate pos, %d passable: %s: Regenerate triangulation in editor [Design bug]",
            badOutdoorNodes, badIndoorNodes, badNumOutsideLinks, indexDegenerates, posDegenerates, badLinkPassability, msg);
    }

    AILogProgress("CGraph::Validate Finished graph validation: %s", msg);
    return (0 == badOutdoorNodes + badIndoorNodes);
}

//===================================================================
// ResetIDs
//===================================================================
void GraphNode::ResetIDs(CGraphNodeManager& nodeManager, class CAllNodesContainer& allNodes, GraphNode* pNodeForID1)
{
    CAllNodesContainer::Iterator itAll(allNodes, 0xffffffff);

    maxID = 1;
    while (GraphNode* pCurrent = nodeManager.GetNode(itAll.Increment()))
    {
        if (pCurrent == pNodeForID1)
        {
            pCurrent->ID = 1;
        }
        else
        {
            pCurrent->ID = ++maxID;
        }
    }

    freeIDs.clear();
}


// interface fns concerning the node/links
int CGraph::GetBuildingIDFromWaypointNode(const GraphNode* pNode) {return pNode->GetWaypointNavData()->nBuildingID; }
void CGraph::SetBuildingIDInWaypointNode(GraphNode* pNode, unsigned nBuildingID) {pNode->GetWaypointNavData()->nBuildingID = nBuildingID; }
void CGraph::SetVisAreaInWaypointNode(GraphNode* pNode, IVisArea* pArea) {pNode->GetWaypointNavData()->pArea = pArea; }
EWaypointNodeType CGraph::GetWaypointNodeTypeFromWaypointNode(const GraphNode* pNode) {return pNode->GetWaypointNavData()->type; }
void CGraph::SetWaypointNodeTypeInWaypointNode(GraphNode* pNode, EWaypointNodeType type) {pNode->GetWaypointNavData()->type = type; }
EWaypointLinkType CGraph::GetOrigWaypointLinkTypeFromLink(unsigned link) {return SWaypointNavData::GetLinkTypeFromRadius(GetLinkManager().GetOrigRadius(link)); }
float CGraph::GetRadiusFromLink(unsigned link) {return GetLinkManager().GetRadius(link); }
float CGraph::GetOrigRadiusFromLink(unsigned link) {return GetLinkManager().GetOrigRadius(link); }
IAISystem::ENavigationType CGraph::GetNavType(const GraphNode* pNode) {return pNode->navType; }
unsigned CGraph::GetNumNodeLinks(const GraphNode* pNode) {return pNode->GetLinkCount(GetLinkManager()); }
unsigned CGraph::GetGraphLink(const GraphNode* pNode, unsigned iLink)
{
    unsigned link;
    for (link = pNode->firstLinkIndex; link && iLink; link = GetLinkManager().GetNextLink(link), --iLink)
    {
        ;
    }
    return link;
}
Vec3 CGraph::GetWaypointNodeUpDir(const GraphNode* pNode) {return pNode->GetWaypointNavData()->up; }
void CGraph::SetWaypointNodeUpDir(GraphNode* pNode, const Vec3& up)  {pNode->GetWaypointNavData()->up = up; }
Vec3 CGraph::GetWaypointNodeDir(const GraphNode* pNode)  {return pNode->GetWaypointNavData()->dir; }
void CGraph::SetWaypointNodeDir(GraphNode* pNode, const Vec3& dir)  {pNode->GetWaypointNavData()->dir = dir; }
Vec3 CGraph::GetNodePos(const GraphNode* pNode) {return pNode->GetPos(); }
const GraphNode* CGraph::GetNextNode(unsigned link) const {return GetNodeManager().GetNode(GetLinkManager().GetNextNode(link)); }
GraphNode* CGraph::GetNextNode(unsigned link) {return GetNodeManager().GetNode(GetLinkManager().GetNextNode(link)); }


