// Modifications copyright Amazon.com, Inc. or its affiliates.

////////////////////////////////////////////////////////////////////
// Original file: Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#ifdef LY_TERRAIN_RUNTIME

#include "CDLODQuadTree.h"
#include <AzCore/Math/IntersectSegment.h>
#include "Cry_Math.h"

#include <Terrain/Bus/TerrainProviderBus.h>

#include <AzCore/Jobs/JobManagerBus.h>

CDLODQuadTree::CDLODQuadTree()
{
}

CDLODQuadTree::~CDLODQuadTree()
{
    Clean();

    Terrain::HeightmapDataNotificationBus::Handler::BusDisconnect();
}

bool CDLODQuadTree::Create(const CreateDesc& desc)
{
    Clean();

    Terrain::HeightmapDataNotificationBus::Handler::BusConnect();

    m_desc = desc;

    AZ_Assert(m_desc.m_lodLevelCount <= Terrain::TerrainRenderingParameters::c_TerrainMaxLODLevels,
        "CDLODQuadTree::Create() | Invalid LODLevelCount!");

    //////////////////////////////////////////////////////////////////////////
    // Determine how many nodes will we use, and the size of the top (root) tree
    // node.
    int totalNodeCount = 0;

    AZ::u32 nodeSize = static_cast<AZ::u32>(desc.m_leafRenderNodeSize);
    m_nodeSizes[0] = desc.m_leafRenderNodeSize;

    // determine top node size using the LODCount and leaf node size
    for (int i = 0; i < m_desc.m_lodLevelCount; i++)
    {
        if (i != 0)
        {
            nodeSize *= 2;
            m_nodeSizes[i] = nodeSize;
        }

        int nodeCountX = static_cast<int>(desc.m_mapDims.SizeX() / GetTopNodeSize() + 1);
        int nodeCountY = static_cast<int>(desc.m_mapDims.SizeY() / GetTopNodeSize() + 1);

        totalNodeCount += (nodeCountX) * (nodeCountY);
    }

    // Lowest LOD nodes
    m_topNodeCountX = static_cast<AZ::u32>((m_desc.m_mapDims.SizeX() + GetTopNodeSize() - 1) / GetTopNodeSize());
    m_topNodeCountY = static_cast<AZ::u32>((m_desc.m_mapDims.SizeY() + GetTopNodeSize() - 1) / GetTopNodeSize());

    return true;
}

CDLODQuadTree::Node::LODSelectResult CDLODQuadTree::Node::LODSelect(LODSelectInfo& lodSelectInfo, bool parentCompletelyInFrustum, unsigned int x, unsigned int y, unsigned short size, int LODLevel)
{
    AZ::Aabb boundingBox;

    float minZ = lodSelectInfo.MapDims.MinZ();
    float maxZ = lodSelectInfo.MapDims.MaxZ();

    lodSelectInfo.SelectionObj->m_quadTree->GetMinMaxHeight(x, y, LODLevel, minZ, maxZ);
    GetAABB(boundingBox, lodSelectInfo.MapDims, x, y, size, minZ, maxZ);

    const AZ::Vector3& observerPos = lodSelectInfo.SelectionObj->m_observerPos;
    const int maxSelectionCount = lodSelectInfo.SelectionObj->m_maxSelectionCount;

    LODSelection::FrustumIntersectResult frustumIt = (parentCompletelyInFrustum) ? (LODSelection::FrustumIntersectResult::Inside) : lodSelectInfo.SelectionObj->CalculateFrustumIntersection(boundingBox);
    if (frustumIt == LODSelection::FrustumIntersectResult::Outside)
    {
        return IT_OutOfFrustum;
    }

    const float distanceLimitSqr = lodSelectInfo.SelectionObj->m_visibilityRangesSqr[LODLevel];

    // check if within the respective LOD range
    float dist2 = boundingBox.GetDistanceSq(observerPos);
    if (dist2 > distanceLimitSqr)
    {
        return IT_OutOfRange;
    }

    LODSelectResult subTLSelRes = IT_Undefined;
    LODSelectResult subTRSelRes = IT_Undefined;
    LODSelectResult subBLSelRes = IT_Undefined;
    LODSelectResult subBRSelRes = IT_Undefined;

    if (LODLevel != lodSelectInfo.SelectionObj->m_stopAtLODLevel)
    {
        const float nextDistanceLimitSqr = lodSelectInfo.SelectionObj->m_visibilityRangesSqr[LODLevel - 1];

        // subdivide if this node is within the next LOD
        if (dist2 <= nextDistanceLimitSqr)
        {
            bool weAreCompletelyInFrustum = (frustumIt == LODSelection::FrustumIntersectResult::Inside);

            unsigned short halfSize = size / 2;
            subTLSelRes = Node::LODSelect(lodSelectInfo, weAreCompletelyInFrustum, x, y, halfSize, LODLevel - 1);
            subTRSelRes = Node::LODSelect(lodSelectInfo, weAreCompletelyInFrustum, x + halfSize, y, halfSize, LODLevel - 1);
            subBLSelRes = Node::LODSelect(lodSelectInfo, weAreCompletelyInFrustum, x, y + halfSize, halfSize, LODLevel - 1);
            subBRSelRes = Node::LODSelect(lodSelectInfo, weAreCompletelyInFrustum, x + halfSize, y + halfSize, halfSize, LODLevel - 1);
        }
    }

    // We don't want to select sub nodes that are invisible (out of frustum, so not visible) or are selected (subdivision was selected)
    // (we DO want to select if they are out of range, because range calculations are based on each LODs' respective distance range)
    bool bRemoveSubTL = (subTLSelRes == IT_OutOfFrustum) || (subTLSelRes == IT_Selected);
    bool bRemoveSubTR = (subTRSelRes == IT_OutOfFrustum) || (subTRSelRes == IT_Selected);
    bool bRemoveSubBL = (subBLSelRes == IT_OutOfFrustum) || (subBLSelRes == IT_Selected);
    bool bRemoveSubBR = (subBRSelRes == IT_OutOfFrustum) || (subBRSelRes == IT_Selected);

    // select (whole or in part) unless all sub nodes are selected by child nodes, either as parts of this or lower LOD levels
    bool bIncludeThisNode = !(bRemoveSubTL && bRemoveSubTR && bRemoveSubBL && bRemoveSubBR);

    // or select anyway if IncludeAllNodesInRange flag is on (used for streaming)
    bIncludeThisNode |= ((lodSelectInfo.SelectionObj->m_flags & LODSelection::Flags::IncludeAllNodesInRange) != 0);

    if (bIncludeThisNode)
    {
        bool selectionBufferHasMoreSpace = lodSelectInfo.SelectionCount < maxSelectionCount;

        AZ_Error("CDLODQuadTree", selectionBufferHasMoreSpace, "CDLODQuadTree::Node::LODSelect | Selection buffer not large enough!");

        if (selectionBufferHasMoreSpace)
        {
            lodSelectInfo.SelectionObj->m_selectionBuffer[lodSelectInfo.SelectionCount++] = SelectedNode(boundingBox, LODLevel, !bRemoveSubTL, !bRemoveSubTR, !bRemoveSubBL, !bRemoveSubBR, dist2);

            if (!lodSelectInfo.SelectionObj->m_visDistTooSmall
                && (LODLevel != lodSelectInfo.LODLevelCount - 1))
            {
                float maxDistFromCam = sqrtf(dist2);

                float morphStartRange = lodSelectInfo.SelectionObj->m_morphStart[LODLevel + 1];

                // check if distance to node is further than morph start range
                // these cases are possible with extremely steep terrain, or if the node size is too small relative to the morph range
                if (maxDistFromCam > morphStartRange)
                {
                    // TODO add debug cvar for rendering wireframe boxes when this issue occurs

                    lodSelectInfo.SelectionObj->m_visDistTooSmall = true;

                    AZ_Error("CDLODQuadTree", false, "Node size too large for morph range!");
                }
            }

            return IT_Selected;
        }
    }

    // if any of child nodes are selected, then return selected - otherwise all of them are out of frustum, so we're out of frustum too
    if ((subTLSelRes == IT_Selected) || (subTRSelRes == IT_Selected) || (subBLSelRes == IT_Selected) || (subBRSelRes == IT_Selected))
    {
        return IT_Selected;
    }
    else
    {
        return IT_OutOfFrustum;
    }
}

CDLODQuadTree::Node::LODSelectResult CDLODQuadTree::Node::LODSelect_TextureStreaming(LODSelectInfo& lodSelectInfo, bool parentCompletelyInFrustum, unsigned int x, unsigned int y, unsigned short size, int LODLevel, int& outMinMipReq, int& outMaxMipReq)
{
    // initialize mip output data
    outMinMipReq = lodSelectInfo.SelectionObj->m_quadTree->GetLODLevelCount() - 1;
    outMaxMipReq = 0;

    AZ::Aabb boundingBox;

    float minZ = lodSelectInfo.MapDims.MinZ();
    float maxZ = lodSelectInfo.MapDims.MaxZ();

    lodSelectInfo.SelectionObj->m_quadTree->GetMinMaxHeight(x, y, LODLevel, minZ, maxZ);
    GetAABB(boundingBox, lodSelectInfo.MapDims, x, y, size, minZ, maxZ);

    const AZ::Vector3& observerPos = lodSelectInfo.SelectionObj->m_observerPos;
    const int maxSelectionCount = lodSelectInfo.SelectionObj->m_maxSelectionCount;
    const float maxViewDistance = lodSelectInfo.SelectionObj->m_visibilityDistance;

    LODSelection::FrustumIntersectResult frustumIt = (parentCompletelyInFrustum) ? (LODSelection::FrustumIntersectResult::Inside) : lodSelectInfo.SelectionObj->CalculateFrustumIntersection(boundingBox);
    if (frustumIt == LODSelection::FrustumIntersectResult::Outside)
    {
        return IT_OutOfFrustum;
    }

    // NEW-TERRAIN LY-103230: convert to squared distances
    float minWorldDistanceToNode = boundingBox.GetDistance(observerPos);
    float maxWorldDistanceToNode = boundingBox.GetMaxDistanceSq(observerPos).GetSqrt();

    if (minWorldDistanceToNode > maxViewDistance)
    {
        return IT_OutOfRange;
    }

    int minMipReq, maxMipReq;
    lodSelectInfo.SelectionObj->CalculateMipRequirements(minWorldDistanceToNode, maxWorldDistanceToNode, minMipReq, maxMipReq);

    LODSelectResult subTLSelRes = IT_Undefined;
    LODSelectResult subTRSelRes = IT_Undefined;
    LODSelectResult subBLSelRes = IT_Undefined;
    LODSelectResult subBRSelRes = IT_Undefined;

    bool subNodeSelected = false;

    if (LODLevel != lodSelectInfo.SelectionObj->m_stopAtLODLevel)
    {
        // Subdivide if the min mip requirement is less than or equal to the next LOD level
        if (minMipReq <= LODLevel - 1)
        {
            bool weAreCompletelyInFrustum = (frustumIt == LODSelection::FrustumIntersectResult::Inside);

            auto updateMinMaxMipRequirement = [&outMinMipReq, &outMaxMipReq, &subNodeSelected](LODSelectResult selectionResult, int selectedNodeMinMipReq, int selectedNodeMaxMipReq)
                {
                    // only update the min/max if the nodes are selected
                    if (selectionResult == IT_Selected)
                    {
                        outMinMipReq = AZ::GetMin(outMinMipReq, selectedNodeMinMipReq);
                        outMaxMipReq = AZ::GetMax(outMaxMipReq, selectedNodeMaxMipReq);
                        subNodeSelected = true;
                    }
                };

            unsigned short halfSize = size / 2;
            int subNodeMinMipReq, subNodeMaxMipReq;

            subTLSelRes = Node::LODSelect_TextureStreaming(lodSelectInfo, weAreCompletelyInFrustum, x, y, halfSize, LODLevel - 1, subNodeMinMipReq, subNodeMaxMipReq);
            updateMinMaxMipRequirement(subTLSelRes, subNodeMinMipReq, subNodeMaxMipReq);

            subTRSelRes = Node::LODSelect_TextureStreaming(lodSelectInfo, weAreCompletelyInFrustum, x + halfSize, y, halfSize, LODLevel - 1, subNodeMinMipReq, subNodeMaxMipReq);
            updateMinMaxMipRequirement(subTRSelRes, subNodeMinMipReq, subNodeMaxMipReq);

            subBLSelRes = Node::LODSelect_TextureStreaming(lodSelectInfo, weAreCompletelyInFrustum, x, y + halfSize, halfSize, LODLevel - 1, subNodeMinMipReq, subNodeMaxMipReq);
            updateMinMaxMipRequirement(subBLSelRes, subNodeMinMipReq, subNodeMaxMipReq);

            subBRSelRes = Node::LODSelect_TextureStreaming(lodSelectInfo, weAreCompletelyInFrustum, x + halfSize, y + halfSize, halfSize, LODLevel - 1, subNodeMinMipReq, subNodeMaxMipReq);
            updateMinMaxMipRequirement(subBRSelRes, subNodeMinMipReq, subNodeMaxMipReq);
        }
    }

    if (!subNodeSelected)
    {
        // Base case: either we never attempted to subdivide OR the subdivide failed to find any valid nodes
        outMinMipReq = minMipReq;
        outMaxMipReq = maxMipReq;
    }

    // We want to include nodes whenever any child is selected
    bool bIncludeThisNode = (subTLSelRes == IT_Selected)
        || (subTRSelRes == IT_Selected)
        || (subBLSelRes == IT_Selected)
        || (subBRSelRes == IT_Selected);

    // we only want to include this node if the mip level falls between the interval requirement
    bIncludeThisNode |= (LODLevel >= outMinMipReq) && (LODLevel <= outMaxMipReq);

    if (bIncludeThisNode)
    {
        bool selectionBufferHasMoreSpace = lodSelectInfo.SelectionCount < maxSelectionCount;

        AZ_Error("CDLODQuadTree", selectionBufferHasMoreSpace, "CDLODQuadTree::Node::LODSelect_TextureStreaming | Selection buffer not large enough!");

        if (selectionBufferHasMoreSpace)
        {
            lodSelectInfo.SelectionObj->m_selectionBuffer[lodSelectInfo.SelectionCount++] = SelectedNode(boundingBox, LODLevel, true, true, true, true, minWorldDistanceToNode * minWorldDistanceToNode);
            return IT_Selected;
        }
    }

    // if any of child nodes are selected, then return selected - otherwise all of them are out of frustum, so we're out of frustum too
    if ((subTLSelRes == IT_Selected) || (subTRSelRes == IT_Selected) || (subBLSelRes == IT_Selected) || (subBRSelRes == IT_Selected))
    {
        // All subnodes are selected, but this node wasn't included
        return IT_Selected;
    }
    else
    {
        return IT_OutOfFrustum;
    }
}

void CDLODQuadTree::Clean()
{
    AZStd::lock_guard<AZStd::recursive_mutex> minMaxMapGuard(m_minMaxDataMutex);

    // Clear min/max height data
    m_topNodeIndexToMinMaxData.clear();
}

int compare_closerFirst(const void* arg1, const void* arg2)
{
    const SelectedNode* a = (const SelectedNode*)arg1;
    const SelectedNode* b = (const SelectedNode*)arg2;

    if (a->m_minDistSqrToCamera < b->m_minDistSqrToCamera)
    {
        return -1;
    }
    else if (a->m_minDistSqrToCamera > b->m_minDistSqrToCamera)
    {
        return 1;
    }

    return 0;
}

void CDLODQuadTree::LODSelect(LODSelection* selectionObj) const
{
    const AZ::Vector3& cameraPos = selectionObj->m_observerPos;
    const int   LODLevelCount = m_desc.m_lodLevelCount;

    float LODFar = selectionObj->m_visibilityDistance;

    selectionObj->m_quadTree = this;
    selectionObj->m_visDistTooSmall = false;

    // Fill in mesh LOD distances
    float currentLODDistance = selectionObj->m_initialLODDistance;
    for (int i = 0; i < LODLevelCount - 1; i++)
    {
        selectionObj->m_visibilityRanges[i] = currentLODDistance;
        selectionObj->m_visibilityRangesSqr[i] = currentLODDistance * currentLODDistance;
        currentLODDistance *= selectionObj->m_lodRatio;
    }

    // Final LOD distance based on max render distance
    selectionObj->m_visibilityRanges[LODLevelCount - 1] = selectionObj->m_visibilityDistance;
    selectionObj->m_visibilityRangesSqr[LODLevelCount - 1] = selectionObj->m_visibilityDistance * selectionObj->m_visibilityDistance;

    float prevPos = 0.0f;
    for (int i = 0; i < LODLevelCount; i++)
    {
        selectionObj->m_morphEnd[i] = selectionObj->m_visibilityRanges[i];
        selectionObj->m_morphStart[i] = prevPos + (selectionObj->m_morphEnd[i] - prevPos) * selectionObj->m_morphStartRatio;

        prevPos = selectionObj->m_morphEnd[i];
    }

    Node::LODSelectInfo lodSelInfo;
    lodSelInfo.MapDims = m_desc.m_mapDims;
    lodSelInfo.SelectionCount = 0;
    lodSelInfo.SelectionObj = selectionObj;
    lodSelInfo.LODLevelCount = LODLevelCount;

    bool textureStreamingLodSelection = (selectionObj->m_flags & LODSelection::Flags::TextureStreaming) != 0;

    AZ::u32 topNodeSize = GetTopNodeSize();

    for (unsigned int y = 0; y < m_topNodeCountY; y++)
    {
        for (unsigned int x = 0; x < m_topNodeCountX; x++)
        {
            if (textureStreamingLodSelection)
            {
                int minMipReq = LODLevelCount - 1;
                int maxMipReq = 0;
                Node::LODSelect_TextureStreaming(lodSelInfo, false, x * topNodeSize, y * topNodeSize, topNodeSize, LODLevelCount - 1, minMipReq, maxMipReq);
            }
            else
            {
                Node::LODSelect(lodSelInfo, false, x * topNodeSize, y * topNodeSize, topNodeSize, LODLevelCount - 1);
            }
        }
    }

    selectionObj->m_selectionCount = lodSelInfo.SelectionCount;

    if ((selectionObj->m_flags & LODSelection::SortByDistance) != 0)
    {
        qsort(selectionObj->m_selectionBuffer, selectionObj->m_selectionCount, sizeof(*selectionObj->m_selectionBuffer), compare_closerFirst);
    }
}

// assumes coordinates are local to quadtree, in meters
void CDLODQuadTree::GetMinMaxHeight(AZ::u32 localX, AZ::u32 localY, int lodLevel, float& minHeight, float& maxHeight) const
{
    // We don't need precise min/max height data for the lowest detailed lod
    if (lodLevel < m_desc.m_lodLevelCount - 1)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> minMaxMapGuard(m_minMaxDataMutex);

        AZ::u32 topNodeSize = GetTopNodeSize();

        AZ::u32 topNodeIndexX = localX / topNodeSize;
        AZ::u32 topNodeIndexY = localY / topNodeSize;

        AZ::u32 topNodeIndex = topNodeIndexX + topNodeIndexY * m_topNodeCountX;

        auto iter = m_topNodeIndexToMinMaxData.find(topNodeIndex);
        if (iter != m_topNodeIndexToMinMaxData.end())
        {
            // found!
            minHeight = m_desc.m_mapDims.MinZ();
            maxHeight = m_desc.m_mapDims.MaxZ();

            // convert to local indices and sample
            // node local coordinate in meters = localX % topNodeSize
            // node local index = (node local coordinate in meters) / (node size)
            AZ::u32 nodeLocalX = (localX % topNodeSize) / m_nodeSizes[lodLevel];
            AZ::u32 nodeLocalY = (localY % topNodeSize) / m_nodeSizes[lodLevel];

            m_topNodeIndexToMinMaxData[topNodeIndex]->GetMinMaxHeight(nodeLocalX, nodeLocalY, lodLevel, minHeight, maxHeight);

            return;
        }

        RequestMinMaxHeightData(topNodeIndexX, topNodeIndexY);
    }

    minHeight = m_desc.m_mapDims.MinZ();
    maxHeight = m_desc.m_mapDims.MaxZ();
}

void CDLODQuadTree::HeightmapVersionUpdate()
{
    // Clear min max data cache
    Clean();
}

// Keeping around just in-case
void CDLODQuadTree::RequestMinMaxHeightData(int topNodeX, int topNodeY) const
{
    AZStd::lock_guard<AZStd::recursive_mutex> minMaxMapGuard(m_minMaxDataMutex);

    AZ::u32 nodeIndex = topNodeX + topNodeY * m_topNodeCountX;

    // Upon creation, the min max height data entry is marked not ready, so any subsequent lookups will be gracefully handled
    AZStd::shared_ptr<CDLODMinMaxHeightData> pMinMaxData = AZStd::make_shared<CDLODMinMaxHeightData>(m_desc.m_lodLevelCount);
    m_topNodeIndexToMinMaxData[nodeIndex] = pMinMaxData;

    AZ::JobContext* jobContext = nullptr;
    AZ::JobManagerBus::BroadcastResult(jobContext, &AZ::JobManagerEvents::GetGlobalContext);

    auto buildMinMaxMapJob = [pMinMaxData]()
        {
            pMinMaxData->ProcessLod0AndMarkReady();
        };

    // Kick off lod processing in a job when the request is fulfilled
    auto buildMinMaxMapJobStart = [jobContext, buildMinMaxMapJob]()
        {
            AZ::Job* pJob = AZ::CreateJobFunction(buildMinMaxMapJob, true, jobContext);
            pJob->Start();
        };

    // Set up request
    int topNodeSize = GetTopNodeSize();
    AZ::Vector2 nodeMin = AZ::Vector2(topNodeX * topNodeSize + m_desc.m_mapDims.MinX(), topNodeY * topNodeSize + m_desc.m_mapDims.MinY());
    AZ::Vector2 nodeMax = AZ::Vector2(nodeMin.GetX() + topNodeSize, nodeMin.GetY() + topNodeSize);
    float leafNodeSize = static_cast<float>(m_nodeSizes[0]);
    float* tileMinMaxDataPtr = pMinMaxData->GetDataPtr();
    Terrain::TerrainProviderRequestBus::Broadcast(&Terrain::TerrainProviderRequestBus::Events::RequestMinMaxHeightmapData, nodeMin, nodeMax, leafNodeSize, tileMinMaxDataPtr, buildMinMaxMapJobStart);
}


// assumes that we are sampling in node local space
void CDLODMinMaxHeightData::GetMinMaxHeight(AZ::u32 nodeIndexX, AZ::u32 nodeIndexY, int lodLevel, float& minHeight, float& maxHeight) const
{
    if (m_dataReady)
    {
        int currentLodDimensions = 1 << (m_lodLevels - lodLevel - 1);
        int offset = m_lodOffsets[lodLevel] + nodeIndexY * currentLodDimensions * 2 + nodeIndexX * 2;

        minHeight = m_data[offset];
        maxHeight = m_data[offset + 1];
    }
}

void CDLODMinMaxHeightData::ProcessLod0AndMarkReady()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

    int lodIndex = 1;

    // Using a lambda to sample because we need to bypass the ready flag
    auto getMinMaxHeight = [=](AZ::u32 nodeIndexX, AZ::u32 nodeIndexY, int lodLevel, float& minHeight, float& maxHeight)
        {
            int currentLodDimensions = 1 << (m_lodLevels - lodLevel - 1);
            int offset = m_lodOffsets[lodLevel] + nodeIndexY * currentLodDimensions * 2 + nodeIndexX * 2;

            minHeight = m_data[offset];
            maxHeight = m_data[offset + 1];
        };

    auto samplePreviousLod = [=](int currentLod, AZ::u32 x, AZ::u32 y, float& minHeight, float& maxHeight)
        {
            minHeight = FLT_MAX;
            maxHeight = -FLT_MAX;

            float sampledMinHeight, sampledMaxHeight;
            getMinMaxHeight(2 * x, 2 * y, currentLod - 1, sampledMinHeight, sampledMaxHeight);
            minHeight = AZ::GetMin(minHeight, sampledMinHeight);
            maxHeight = AZ::GetMax(maxHeight, sampledMaxHeight);

            getMinMaxHeight(2 * x + 1, 2 * y, currentLod - 1, sampledMinHeight, sampledMaxHeight);
            minHeight = AZ::GetMin(minHeight, sampledMinHeight);
            maxHeight = AZ::GetMax(maxHeight, sampledMaxHeight);

            getMinMaxHeight(2 * x, 2 * y + 1, currentLod - 1, sampledMinHeight, sampledMaxHeight);
            minHeight = AZ::GetMin(minHeight, sampledMinHeight);
            maxHeight = AZ::GetMax(maxHeight, sampledMaxHeight);

            getMinMaxHeight(2 * x + 1, 2 * y + 1, currentLod - 1, sampledMinHeight, sampledMaxHeight);
            minHeight = AZ::GetMin(minHeight, sampledMinHeight);
            maxHeight = AZ::GetMax(maxHeight, sampledMaxHeight);
        };

    while (lodIndex < m_lodLevels)
    {
        int currentLodDimensions = 1 << (m_lodLevels - lodIndex - 1);

        for (int yIndex = 0; yIndex < currentLodDimensions; ++yIndex)
        {
            for (int xIndex = 0; xIndex < currentLodDimensions; ++xIndex)
            {
                int offset = m_lodOffsets[lodIndex] + yIndex * currentLodDimensions * 2 + xIndex * 2;
                samplePreviousLod(lodIndex, xIndex, yIndex, m_data[offset], m_data[offset + 1]);
            }
        }

        ++lodIndex;
    }

    m_dataReady = true;
}


LODSelection::LODSelection(SelectedNode* selectionBuffer, int maxSelectionCount, const LODSelectionDesc& lodSelectionDesc)
{
    m_selectionBuffer = selectionBuffer;
    m_maxSelectionCount = maxSelectionCount;

    m_observerPos = lodSelectionDesc.m_cameraPosition;
    if (lodSelectionDesc.m_frustumPlanes == nullptr)
    {
        memset(m_frustumPlanes, 0, sizeof(m_frustumPlanes));
        m_useFrustumCull = false;
    }
    else
    {
        memcpy(m_frustumPlanes, lodSelectionDesc.m_frustumPlanes, sizeof(m_frustumPlanes));
        m_useFrustumCull = true;
    }

    m_lodRatio = lodSelectionDesc.m_lodRatio;
    m_initialLODDistance = lodSelectionDesc.m_initialLODDistance;
    m_visibilityDistance = lodSelectionDesc.m_maxVisibilityDistance;

    m_morphStartRatio = lodSelectionDesc.m_morphStartRatio;
    m_stopAtLODLevel = lodSelectionDesc.m_stopAtLODLevel;
    m_flags = lodSelectionDesc.m_flags;

    m_quadTree = nullptr;
    m_selectionCount = 0;
    m_visDistTooSmall = false;
}

LODSelection::~LODSelection()
{
}

void LODSelection::GetMorphConsts(int LODLevel, float consts[4]) const
{
    float mStart = m_morphStart[LODLevel];
    float mEnd = m_morphEnd[LODLevel];

    const float errorFudge = 0.01f;
    mEnd = Lerp(mEnd, mStart, errorFudge);

    consts[0] = mStart;                     // Morph start
    consts[1] = 1.0f / (mEnd - mStart);     // Inverse morph range

    consts[2] = mEnd / (mEnd - mStart);     // Morph end / morph range
    consts[3] = 1.0f / (mEnd - mStart);     // Inverse morph range
}

void LODSelection::CalculateMipRequirements(float minDistance, float maxDistance, int& minMipReq, int& maxMipReq) const
{
    // NEW-TERRAIN LY-103230: precalculate tanf result
    float tanFactor = 2.0f * tanf(m_textureStreamingParams.m_fov * 0.5f / m_textureStreamingParams.m_screenPixelHeight);

    float minPixelWorldWidth = minDistance * tanFactor;
    float maxPixelWorldWidth = maxDistance * tanFactor;

    float minMipLevel = floorf(logf(minPixelWorldWidth * m_textureStreamingParams.m_texelsPerMeter) / logf(2.0f));
    float maxMipLevel = ceilf(logf(maxPixelWorldWidth * m_textureStreamingParams.m_texelsPerMeter) / logf(2.0f));

    if (maxMipLevel > (m_quadTree->GetLODLevelCount() - 1))
    {
        m_debugMaxMipLevelExceededCount++;
    }

    minMipReq = AZ::GetClamp(static_cast<int>(minMipLevel), 0, m_quadTree->GetLODLevelCount() - 1);
    maxMipReq = AZ::GetClamp(static_cast<int>(maxMipLevel), 0, m_quadTree->GetLODLevelCount() - 1);
}

LODSelection::FrustumIntersectResult LODSelection::CalculateFrustumIntersection(const AZ::Aabb& aabb) const
{
    if (m_useFrustumCull)
    {
        AZ::Vector3 center = aabb.GetCenter();
        AZ::Vector3 extents = 0.5f * aabb.GetExtents();
        bool bIntersecting = false;

        for (int p = 0; p < c_FrustumPlaneCount; p++)
        {
            AZ::Vector3 planeNormal = m_frustumPlanes[p].GetNormal();
            float r = planeNormal.GetAbs().Dot(extents);
            float dist = m_frustumPlanes[p].GetPointDist(center);

            if (dist > r)
            {
                return LODSelection::FrustumIntersectResult::Outside;
            }
            else if (abs(dist) < r)
            {
                bIntersecting = true;
            }
        }

        if (bIntersecting)
        {
            return LODSelection::FrustumIntersectResult::Intersect;
        }

        return LODSelection::FrustumIntersectResult::Inside;
    }

    return FrustumIntersectResult::Inside;
}

void CDLODQuadTree::Node::GetAABB(AZ::Aabb& aabb, const MapDimensions& mapDims, unsigned int x, unsigned int y, unsigned short size, float minZ, float maxZ)
{
    float fMinX = mapDims.MinX() + x;
    float fMaxX = mapDims.MinX() + (x + size);
    float fMinY = mapDims.MinY() + y;
    float fMaxY = mapDims.MinY() + (y + size);

    aabb.SetMin(AZ::Vector3(fMinX, fMinY, minZ));
    aabb.SetMax(AZ::Vector3(fMaxX, fMaxY, maxZ));
}

#endif