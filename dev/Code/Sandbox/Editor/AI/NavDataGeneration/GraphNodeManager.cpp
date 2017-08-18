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
#include "GraphStructures.h"
#include "GraphNodeManager.h"
#include "Graph.h"
#include "CommonDefs.h"

CGraphNodeManager::CGraphNodeManager()
{
    m_typeSizes[0] = sizeof(GraphNode_Unset);
    m_typeSizes[1] = sizeof(GraphNode_Triangular);
    m_typeSizes[2] = sizeof(GraphNode_WaypointHuman);
    m_typeSizes[3] = sizeof(GraphNode_Waypoint3DSurface);
    m_typeSizes[4] = sizeof(GraphNode_Flight);
    m_typeSizes[5] = sizeof(GraphNode_Volume);
    m_typeSizes[6] = sizeof(GraphNode_Road);
    m_typeSizes[7] = sizeof(GraphNode_SmartObject);
    m_typeSizes[8] = sizeof(GraphNode_Free2D);

    for (int i = 0; i < IAISystem::NAV_TYPE_COUNT; ++i)
    {
        m_lastBucket[i] = -1;
        m_nextNode[i] = BUCKET_SIZE;
    }
}

CGraphNodeManager::~CGraphNodeManager()
{
    Clear(-1);
}

void CGraphNodeManager::Clear(unsigned typeMask)
{
    for (int i = 0, count = int(m_buckets.size()); i < count; ++i)
    {
        if (m_buckets[i] && m_buckets[i]->type & typeMask)
        {
            delete [] (reinterpret_cast<char*>(m_buckets[i]));
            m_buckets[i] = 0;
        }
    }

    for (int i = 0; i < IAISystem::NAV_TYPE_COUNT; ++i)
    {
        if ((1 << i) & typeMask)
        {
            m_lastBucket[i] = -1;
            m_nextNode[i] = BUCKET_SIZE;
        }
    }
}

unsigned CGraphNodeManager::CreateNode(IAISystem::ENavigationType type, const Vec3& pos, unsigned ID)
{
    int typeIndex = TypeIndexFromType(type);
    if (typeIndex < 0 || typeIndex >= IAISystem::NAV_TYPE_COUNT)
    {
        return 0;
    }

    if (m_nextNode[typeIndex] >= BUCKET_SIZE)
    {
        int newBucketIndex = int(m_buckets.size());
        for (int i = 0; i < int(m_buckets.size()); ++i)
        {
            if (!m_buckets[i])
            {
                newBucketIndex = i;
            }
        }
        int bucketAllocationSize = sizeof(BucketHeader) + m_typeSizes[typeIndex] * BUCKET_SIZE;
        BucketHeader* pHeader = reinterpret_cast<BucketHeader*>(new char [bucketAllocationSize]);
        pHeader->type = type;
        pHeader->nodeSize = m_typeSizes[typeIndex];
        pHeader->nodes = pHeader + 1;
        if (newBucketIndex >= int(m_buckets.size()))
        {
            m_buckets.resize(newBucketIndex + 1);
        }
        m_buckets[newBucketIndex] = pHeader;
        m_lastBucket[typeIndex] = newBucketIndex;
        m_nextNode[typeIndex] = 0;
    }

    GraphNode* pNode = reinterpret_cast<GraphNode*>(
            static_cast<char*>(m_buckets[m_lastBucket[typeIndex]]->nodes) + m_nextNode[typeIndex] * m_typeSizes[typeIndex]);

    switch (type)
    {
    case IAISystem::NAV_UNSET:
        pNode = new(pNode) GraphNode_Unset(type, pos, ID);
        break;
    case IAISystem::NAV_TRIANGULAR:
        pNode = new(pNode) GraphNode_Triangular(type, pos, ID);
        break;
    case IAISystem::NAV_WAYPOINT_HUMAN:
        pNode = new(pNode) GraphNode_WaypointHuman(type, pos, ID);
        break;
    case IAISystem::NAV_WAYPOINT_3DSURFACE:
        pNode = new(pNode) GraphNode_Waypoint3DSurface(type, pos, ID);
        break;
    case IAISystem::NAV_FLIGHT:
        pNode = new(pNode) GraphNode_Flight(type, pos, ID);
        break;
    case IAISystem::NAV_VOLUME:
        pNode = new(pNode) GraphNode_Volume(type, pos, ID);
        break;
    case IAISystem::NAV_ROAD:
        pNode = new(pNode) GraphNode_Road(type, pos, ID);
        break;
    case IAISystem::NAV_SMARTOBJECT:
        pNode = new(pNode) GraphNode_SmartObject(type, pos, ID);
        break;
    case IAISystem::NAV_FREE_2D:
        pNode = new(pNode) GraphNode_Free2D(type, pos, ID);
        break;
    }

    return ((m_lastBucket[typeIndex] << BUCKET_SIZE_SHIFT) + m_nextNode[typeIndex]++) + 1;
}

void CGraphNodeManager::DestroyNode(unsigned index)
{
    // GraphNodes are optimized for size, and we avoid a virtual table by not having a virtual destructor.

    GraphNode* pNode = GetNode(index);
    switch (pNode->navType)
    {
    case IAISystem::NAV_UNSET:
        static_cast<GraphNode_Unset*>(pNode)->~GraphNode_Unset();
        break;
    case IAISystem::NAV_TRIANGULAR:
        static_cast<GraphNode_Triangular*>(pNode)->~GraphNode_Triangular();
        break;
    case IAISystem::NAV_WAYPOINT_HUMAN:
        static_cast<GraphNode_WaypointHuman*>(pNode)->~GraphNode_WaypointHuman();
        break;
    case IAISystem::NAV_WAYPOINT_3DSURFACE:
        static_cast<GraphNode_Waypoint3DSurface*>(pNode)->~GraphNode_Waypoint3DSurface();
        break;
    case IAISystem::NAV_FLIGHT:
        static_cast<GraphNode_Flight*>(pNode)->~GraphNode_Flight();
        break;
    case IAISystem::NAV_VOLUME:
        static_cast<GraphNode_Volume*>(pNode)->~GraphNode_Volume();
        break;
    case IAISystem::NAV_ROAD:
        static_cast<GraphNode_Road*>(pNode)->~GraphNode_Road();
        break;
    case IAISystem::NAV_SMARTOBJECT:
        static_cast<GraphNode_SmartObject*>(pNode)->~GraphNode_SmartObject();
        break;
    case IAISystem::NAV_FREE_2D:
        static_cast<GraphNode_Free2D*>(pNode)->~GraphNode_Free2D();
        break;
    default:
        break;
    }

    // TODO: Free memory (even though it's pooled)?
}

GraphNode* CGraphNodeManager::GetDummyNode() const
{
    static char buffer[sizeof(GraphNode) + 100] = {0};
    static GraphNode* pDummy = new (buffer) GraphNode_Triangular(IAISystem::NAV_TRIANGULAR, Vec3(0, 0, 0), 0xCECECECE);
    return pDummy;
}
