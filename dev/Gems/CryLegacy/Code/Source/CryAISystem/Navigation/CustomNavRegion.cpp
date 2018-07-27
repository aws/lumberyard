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

#include "Navigation/CustomNavRegion.h"
#include "Graph.h"



void CCustomNavRegion::Clear()
{
    // remove all smart object nodes
    m_pGraph->DeleteGraph(IAISystem::NAV_CUSTOM_NAVIGATION);
}

size_t CCustomNavRegion::MemStats()
{
    size_t size = sizeof(*this);
    if (m_pGraph)
    {
        size += m_pGraph->NodeMemStats(IAISystem::NAV_CUSTOM_NAVIGATION);
    }
    return size;
}

void CCustomNavRegion::UglifyPath(
    const VectorConstNodeIndices& inPath, TPathPoints& outPath,
    const Vec3& startPos, const Vec3& startDir,
    const Vec3& endPos, const Vec3& endDir)
{
    // markus: TODO: at least add all of the points. ideally use a callback for beautification
    if (inPath.size() == 1)
    {
        return;
    }


    //AIAssert( inPath.size() == 2 );
    const uint32 size = inPath.size();
    for (uint32 i = 0; i < size; ++i)
    {
        const GraphNode_CustomNav* node1 = static_cast<const GraphNode_CustomNav*>(m_pGraph->GetNodeManager().GetNode(inPath[i]));
        PathPointDescriptor pathPoint1(IAISystem::NAV_CUSTOM_NAVIGATION, node1->GetPos());
        outPath.push_back(pathPoint1);
        outPath.back().navTypeCustomId = node1->GetCustomId();
    }
    /*
        const GraphNode_CustomNav *node1 = static_cast<const GraphNode_CustomNav*>(m_pGraph->GetNodeManager().GetNode(inPath[0]));
        const GraphNode_CustomNav *node2 = static_cast<const GraphNode_CustomNav*>(m_pGraph->GetNodeManager().GetNode(inPath[1]));

        PathPointDescriptor pathPoint1( IAISystem::NAV_CUSTOM_NAVIGATION, node1->GetPos() );
        outPath.push_back( pathPoint1 );
        outPath.back().navTypeCustomId = node1->GetCustomId();
        PathPointDescriptor pathPoint2( IAISystem::NAV_CUSTOM_NAVIGATION, node2->GetPos() );
        outPath.push_back( pathPoint2 );
        outPath.back().navTypeCustomId = node2->GetCustomId();
    */
}


float CCustomNavRegion::GetCustomLinkCostFactor(const GraphNode* nodes[2], const PathfindingHeuristicProperties& pathFindProperties) const
{
    float ret = 0.0f;

    if (nodes[0] && nodes[1])
    {
        const GraphNode_CustomNav* node1 = static_cast<const GraphNode_CustomNav*>(nodes[0]);
        const GraphNode_CustomNav* node2 = static_cast<const GraphNode_CustomNav*>(nodes[1]);

        ret = node1->CustomLinkCostFactor(node1->GetCustomData(), node2->GetCustomData(), pathFindProperties);
    }

    return ret;
}

uint32 CCustomNavRegion::CreateCustomNode(const Vec3& pos, void* customData, uint16 customId, SCustomNavData::CostFunction pCostFunction, bool linkWithEnclosing)
{
    uint32 nodeIndex = m_pGraph->CreateNewNode(IAISystem::NAV_CUSTOM_NAVIGATION, pos);
    GraphNode* node = m_pGraph->GetNodeManager().GetNode(nodeIndex);

    GraphNode_CustomNav* pCustomNode = static_cast<GraphNode_CustomNav*>(node);

    pCustomNode->SetCustomId(customId);
    pCustomNode->SetCustomData(customData);
    pCustomNode->SetCostFunction(pCostFunction);

    return nodeIndex;
}

void CCustomNavRegion::RemoveNode(uint32 nodeIndex)
{
    assert(m_pGraph->GetNode(nodeIndex)->navType == IAISystem::NAV_CUSTOM_NAVIGATION);
    m_pGraph->Disconnect(nodeIndex, true);
};

void CCustomNavRegion::LinkCustomNodes(uint32 node1Index, uint32 node2Index, float radius1To2, float radius2To1)
{
    m_pGraph->Connect(node1Index, node2Index, radius1To2, radius2To1);
}
