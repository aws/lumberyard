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
#include <CrySizer.h>
#include "TriangularNavRegion.h"
#include "WaypointHumanNavRegion.h"
#include "FlightNavRegion.h"
#include "VolumeNavRegion.h"
#include "RoadNavRegion.h"
#include "SmartObjectNavRegion.h"
#include "Navigation.h"

size_t GraphNode::MemStats()
{
    size_t size = 0;

    switch (navType)
    {
    case IAISystem::NAV_UNSET:
        size += sizeof(GraphNode_Unset);
        break;
    case IAISystem::NAV_TRIANGULAR:
        size += sizeof(GraphNode_Triangular);
        size += (GetTriangularNavData()->vertices.capacity() ? GetTriangularNavData()->vertices.capacity() * sizeof(int) + 2 * sizeof(int) : 0);
        break;
    case IAISystem::NAV_WAYPOINT_HUMAN:
        size += sizeof(GraphNode_WaypointHuman);
        break;
    case IAISystem::NAV_WAYPOINT_3DSURFACE:
        size += sizeof(GraphNode_Waypoint3DSurface);
        break;
    case IAISystem::NAV_FLIGHT:
        size += sizeof(GraphNode_Flight);
        break;
    case IAISystem::NAV_VOLUME:
        size += sizeof(GraphNode_Volume);
        break;
    case IAISystem::NAV_ROAD:
        size += sizeof(GraphNode_Road);
        break;
    case IAISystem::NAV_SMARTOBJECT:
        size += sizeof(GraphNode_SmartObject);
        break;
    case IAISystem::NAV_FREE_2D:
        size += sizeof(GraphNode_Free2D);
        break;
    default:
        break;
    }

    //size += links.capacity()*sizeof(unsigned);

    return size;
}

//====================================================================
// MemStats
//====================================================================
size_t CGraph::MemStats()
{
    size_t size = sizeof *this;
    size += sizeof(GraphNode*) * m_taggedNodes.capacity();
    size += sizeof(GraphNode*) * m_markedNodes.capacity();

    size += m_vNodeDescs.capacity() * sizeof(NodeDescriptor);
    size += m_vLinkDescs.capacity() * sizeof(LinkDescriptor);

    size += m_allNodes.MemStats();
    //size += NodesPool.MemStats();
    size += mBadGraphData.capacity() * sizeof(SBadGraphData);
    size += m_lstNodesInsideSphere.size() * sizeof(GraphNode*);
    size += m_mapRemovables.size() * sizeof(bool) + sizeof(void*) * 3 + sizeof(EntranceMap::value_type);
    size += m_mapEntrances.size() * sizeof(bool) + sizeof(void*) * 3 + sizeof(EntranceMap::value_type);
    size += m_mapExits.size() * sizeof(bool) + sizeof(void*) * 3 + sizeof(EntranceMap::value_type);

    size += static_AllocatedObstacles.size() * sizeof(CObstacleRef);

    return size;
}

//====================================================================
// NodeMemStats
//====================================================================
size_t CGraph::NodeMemStats(unsigned navTypeMask)
{
    size_t nodesSize = 0;
    size_t linksSize = 0;

    CAllNodesContainer::Iterator it(m_allNodes, navTypeMask);
    while (unsigned nodeIndex = it.Increment())
    {
        GraphNode* pNode = GetNodeManager().GetNode(nodeIndex);

        if (navTypeMask & pNode->navType)
        {
            nodesSize += pNode->MemStats();
            /*
                  nodesSize += sizeof(GraphNode);
                  linksSize += pNode->links.capacity() * sizeof(GraphLink);
                  for (unsigned i = 0 ; i < pNode->links.size() ; ++i)
                  {
                    if (pNode->links[i].GetCachedPassabilityResult())
                      linksSize += sizeof(GraphLink::SCachedPassabilityResult);
                  }
                  switch (pNode->navType)
                  {
                    case IAISystem::NAV_TRIANGULAR:
                      nodesSize += sizeof(STriangularNavData);
                      nodesSize += pNode->GetTriangularNavData()->vertices.capacity() * sizeof(int);
                      break;
                    case IAISystem::NAV_WAYPOINT_3DSURFACE:
                    case IAISystem::NAV_WAYPOINT_HUMAN:
                      nodesSize += sizeof(SWaypointNavData);
                      break;
                    case IAISystem::NAV_SMARTOBJECT: nodesSize += sizeof(SSmartObjectNavData);
                      break;
                    default:
                      break;
                  }
            */
        }
    }
    return nodesSize + linksSize;
}

//----------------------------------------------------------------------------------

//====================================================================
// MemStats
//====================================================================
size_t CTriangularNavRegion::MemStats()
{
    size_t size = sizeof(*this);
    if (m_pGraph)
    {
        size += m_pGraph->NodeMemStats(IAISystem::NAV_TRIANGULAR);
    }
    return size;
}

//====================================================================
// MemStats
//====================================================================
size_t CWaypointHumanNavRegion::MemStats()
{
    size_t size = sizeof(*this);
    if (m_pNavigation)
    {
        size += m_pNavigation->GetGraph()->NodeMemStats(IAISystem::NAV_WAYPOINT_HUMAN);
    }
    return size;
}

//====================================================================
// MemStats
//====================================================================
size_t CRoadNavRegion::MemStats()
{
    size_t size = sizeof(*this);
    if (m_pGraph)
    {
        size += m_pGraph->NodeMemStats(IAISystem::NAV_ROAD);
    }
    return size;
}

//====================================================================
// MemStats
//====================================================================
size_t CFlightNavRegion::MemStats()
{
    size_t size = sizeof(*this);
    size += sizeof(SSpan) * m_spans.capacity();
    if (m_pGraph)
    {
        size += m_pGraph->NodeMemStats(IAISystem::NAV_FLIGHT);
    }
    return size;
}

//====================================================================
// MemStats
//====================================================================
size_t CVolumeNavRegion::MemStats()
{
    size_t size = sizeof(*this);
    if (m_pGraph)
    {
        size += m_pGraph->NodeMemStats(IAISystem::NAV_VOLUME);
    }

    size += sizeof(CVolumeNavRegion::CVolume) * m_volumes.capacity();
    size += sizeof(CVolumeNavRegion::CPortal) * m_portals.capacity();

    for (unsigned i = 0; i < m_volumes.size(); ++i)
    {
        const CVolume* vol = m_volumes[i];
        size += sizeof(vol);
        if (vol)
        {
            size += vol->m_portalIndices.capacity() * sizeof(int);
        }
    }

    return size;
}
