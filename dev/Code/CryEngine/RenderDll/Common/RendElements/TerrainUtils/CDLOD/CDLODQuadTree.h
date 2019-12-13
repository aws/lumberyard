// Modifications copyright Amazon.com, Inc. or its affiliates.

////////////////////////////////////////////////////////////////////
// Original file: Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme file)
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// CDLODQuadTree below is the main class for working with CDLOD quadtree.
// This version has implicit nodes and MinMax maps and uses ~10% of memory
// used by the 'regular' one.
//
//
//
//  Notes:
//  * Currently the CDLODQuadTree does not support height (z-component), but
//    I did leave in the z-component code so we are more ready to add the support if needed/wanted
//  * Adapted from the CDLOD demo for use with AZ math and some cry math (for rendering)
//  * Height is assumed to be stored as a short, so the inline AABB functions will make this obvious
//////////////////////////////////////////////////////////////////////////

#ifdef LY_TERRAIN_RUNTIME

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/Aabb.h>

#include <RendElements/TerrainUtils/TerrainRenderingParameters.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

#include <Terrain/Bus/TerrainProviderBus.h>

class LODSelection;

// Wrapper for holding min/max height data for terrain
// Very primitive, must be manually managed by the CDLODQuadtree
class CDLODMinMaxHeightData
{
public:
    CDLODMinMaxHeightData() = default;

    CDLODMinMaxHeightData(int lodLevelCount)
        : m_lodLevels(lodLevelCount)
    {
        int offset = 0;
        for (int indexLodLevel = 0; indexLodLevel < m_lodLevels; ++indexLodLevel)
        {
            m_lodOffsets[indexLodLevel] = offset;

            int currentLodDimensions = 1 << (m_lodLevels - indexLodLevel - 1);
            int currentLodEntryCount = currentLodDimensions * currentLodDimensions;
            offset += currentLodEntryCount * 2;
        }

        const int minMaxBufferSize = offset;

        // allocate buffer to hold min/max height data
        m_data = new float[minMaxBufferSize];
        //m_data = nullptr;
    }

    ~CDLODMinMaxHeightData()
    {
        delete[] m_data;
    }

    bool IsDataReady()
    {
        return m_dataReady;
    }

    void SetDataReady(bool ready)
    {
        m_dataReady = ready;
    }

    float* GetDataPtr()
    {
        return m_data;
    }

    // assumes that we are sampling in node local space
    void GetMinMaxHeight(AZ::u32 nodeIndexX, AZ::u32 nodeIndexY, int lodLevel, float& minHeight, float& maxHeight) const;

    // Hand raw HM data to this object, will auto process and fill the min/max map
    //void Initialize(const CDLODQuadTree* m_quadTree, const float* rawHeightData, const AZ::Vector2& dataOrigin, float metersPerPixel, AZ::u32 dataWidth, AZ::u32 dataHeight);
    void ProcessLod0AndMarkReady();

private:
    friend class CDLODQuadTree;

    int m_lodLevels = 0;
    int m_lodOffsets[Terrain::TerrainRenderingParameters::c_VirtualTextureMaxMipLevels] = { 0 };

    AZStd::atomic_bool m_dataReady = {false};
    float* m_data = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// Main class for storing and working with CDLOD quadtree
//////////////////////////////////////////////////////////////////////////
class CDLODQuadTree
    : public Terrain::HeightmapDataNotificationBus::Handler
{
    // Constants

public:

#define CDLODQUADTREE_DEFAULT_MORPH_START_RATIO  (0.70f)

    // Types

    struct MapDimensions
    {
        AZ::Vector3 m_worldMin;
        AZ::Vector3 m_worldSize;

        float MinX() const { return m_worldMin.GetX(); }
        float MinY() const { return m_worldMin.GetY(); }
        float MinZ() const { return m_worldMin.GetZ(); }

        float SizeX() const { return m_worldSize.GetX(); }
        float SizeY() const { return m_worldSize.GetY(); }
        float SizeZ() const { return m_worldSize.GetZ(); }

        float MaxX() const   { return m_worldMin.GetX() + m_worldSize.GetX(); }
        float MaxY() const   { return m_worldMin.GetY() + m_worldSize.GetY(); }
        float MaxZ() const   { return m_worldMin.GetZ() + m_worldSize.GetZ(); }
    };

    struct CreateDesc
    {
        int               m_leafRenderNodeSize;
        int               m_lodLevelCount;

        // Heightmap world dimensions
        MapDimensions     m_mapDims;
    };

    //////////////////////////////////////////////////////////////////////////
    // Quadtree node
    //////////////////////////////////////////////////////////////////////////
    struct Node
    {
        enum LODSelectResult
        {
            IT_Undefined,
            IT_OutOfFrustum,
            IT_OutOfRange,
            IT_Selected,
        };

        struct LODSelectInfo
        {
            LODSelection*       SelectionObj;
            int                  SelectionCount;
            int                  LODLevelCount;
            MapDimensions        MapDims;
        };

        friend class CDLODQuadTree;

        static void       GetWorldMinMaxZ(unsigned short rminZ, unsigned short rmaxZ, float& wminZ, float& wmaxZ, const MapDimensions& mapDims);
        static void       GetAABB(AZ::Aabb& aabb, const MapDimensions& mapDims, unsigned int x, unsigned int y, unsigned short size, float minZ, float maxZ);
        static void       GetBSphere(AZ::Vector3& sphereCenter, float& sphereRadiusSq, const CDLODQuadTree& quadTree, unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ);

    private:
        static LODSelectResult
        LODSelect(LODSelectInfo& lodSelectInfo, bool parentCompletelyInFrustum, unsigned int x, unsigned int y, unsigned short size, int LODLevel);

        static LODSelectResult
        LODSelect_TextureStreaming(LODSelectInfo& lodSelectInfo, bool parentCompletelyInFrustum, unsigned int x, unsigned int y, unsigned short size, int LODLevel, int& outMinMipReq, int& outMaxMipReq);

        Node()            {}
    };


private:

    CreateDesc        m_desc;

    AZ::u32 m_nodeSizes[Terrain::TerrainRenderingParameters::c_TerrainMaxLODLevels];
    AZ::u32 m_topNodeCountX;
    AZ::u32 m_topNodeCountY;

public:
    CDLODQuadTree();
    virtual ~CDLODQuadTree();

    bool                    Create(const CreateDesc& desc);
    void                    Clean();

    int                     GetLODLevelCount() const                    { return m_desc.m_lodLevelCount; }

    void                    LODSelect(LODSelection* selectionObj) const;

    const MapDimensions&   GetWorldMapDims() const                            { return m_desc.m_mapDims; }
    int                     GetLeafNodeSize() const                            { return m_nodeSizes[0]; }

    unsigned int            GetTopNodeSize() const                             { return m_nodeSizes[m_desc.m_lodLevelCount - 1]; }
    unsigned int            GetTopNodeCountX() const                           { return m_topNodeCountX; }
    unsigned int            GetTopNodeCountY() const                           { return m_topNodeCountY; }

    mutable AZStd::recursive_mutex m_minMaxDataMutex;
    mutable AZStd::unordered_map<AZ::u32, AZStd::shared_ptr<CDLODMinMaxHeightData> > m_topNodeIndexToMinMaxData;
    //AZStd::unordered_set<AZ::u32> m_heightDataRequestsInFlight;

    // assumes coordinates are local to quadtree, in meters
    void GetMinMaxHeight(AZ::u32 localX, AZ::u32 localY, int lodLevel, float& minHeight, float& maxHeight) const;
    void RequestMinMaxHeightData(int topNodeX, int topNodeY) const;

    // Terrain::HeightmapDataNotificationBus::Handler Impl
    void HeightmapVersionUpdate() override;
};


//////////////////////////////////////////////////////////////////////////
// LODSelection: used for querying the quadtree for visible nodes
//////////////////////////////////////////////////////////////////////////

struct SelectedNode
{
    AZ::Aabb m_aabb;

    // Always set by the quadtree during LOD selection (the value is calculated during LOD selection anyways)
    float m_minDistSqrToCamera = 0.0f;

    int m_lodLevel = 0;

    bool TL = false;
    bool TR = false;
    bool BL = false;
    bool BR = false;

    SelectedNode() = default;
    SelectedNode(const SelectedNode&) = default;
    SelectedNode& operator=(const SelectedNode&) = default;

    SelectedNode(const AZ::Aabb& aabb, int LODLevel, bool tl, bool tr, bool bl, bool br, float minDistSqrToCamera)
        : m_aabb(aabb)
        , m_lodLevel(LODLevel)
        , TL(tl)
        , TR(tr)
        , BL(bl)
        , BR(br)
        , m_minDistSqrToCamera(minDistSqrToCamera)
    {
    }

    const AZ::Vector3 GetMin() const
    {
        return m_aabb.GetMin();
    }

    const AZ::Vector3 GetSize() const
    {
        return m_aabb.GetExtents();
    }
};

class LODSelection
{
public:
    static const int c_FrustumPlaneCount = 6;

    enum Flags
    {
        SortByDistance = (1 << 0),
        IncludeAllNodesInRange = (1 << 1),     // for selection used by streaming
        TextureStreaming = (1 << 2)
    };

    struct LODSelectionDesc
    {
        AZ::Vector3 m_cameraPosition;
        AZ::Plane m_frustumPlanes[c_FrustumPlaneCount] = {};

        float m_lodRatio = 2.0f;
        float m_initialLODDistance = 0.0f;
        float m_maxVisibilityDistance = 0.0f;

        int m_stopAtLODLevel = 0;
        float m_morphStartRatio = CDLODQUADTREE_DEFAULT_MORPH_START_RATIO;
        Flags m_flags = (Flags)0;
    };

private:
    friend class CDLODQuadTree;
    friend struct CDLODQuadTree::Node;

    // Input
    SelectedNode*       m_selectionBuffer;
    int                  m_maxSelectionCount;

    // Camera Params
    AZ::Vector3          m_observerPos;
    AZ::Plane            m_frustumPlanes[6];
    bool                 m_useFrustumCull;

    // Visibility Params
    float                m_lodRatio;
    float                m_initialLODDistance;
    float                m_visibilityDistance;

    float                m_morphStartRatio;                  // [0, 1] - when to start morphing to the next (lower-detailed) LOD level; default is 0.667 - first 0.667 part will not be morphed, and the morph will go from 0.667 to 1.0
    int                  m_stopAtLODLevel;
    Flags                m_flags;

    // Output
    const CDLODQuadTree* m_quadTree;
    float                m_visibilityRanges[Terrain::TerrainRenderingParameters::c_TerrainMaxLODLevels] = {};
    float                m_visibilityRangesSqr[Terrain::TerrainRenderingParameters::c_TerrainMaxLODLevels] = {};
    float                m_morphEnd[Terrain::TerrainRenderingParameters::c_TerrainMaxLODLevels] = {};
    float                m_morphStart[Terrain::TerrainRenderingParameters::c_TerrainMaxLODLevels] = {};
    int                  m_selectionCount;
    bool                 m_visDistTooSmall;

public:
    LODSelection(SelectedNode* selectionBuffer, int maxSelectionCount, const LODSelectionDesc& lodSelectionDesc);
    ~LODSelection();

    const CDLODQuadTree*          GetQuadTree() const              { return m_quadTree; }

    // Get morph constant data (0: morph start, 1: inverse morph range, 2: morph end/morph range, 3: inverse morph range)
    void                          GetMorphConsts(int LODLevel, float consts[4]) const;

    inline const SelectedNode*   GetSelection() const             { return m_selectionBuffer; }
    inline SelectedNode*         GetSelection()                   { return m_selectionBuffer; }
    inline int                    GetSelectionCount() const        { return m_selectionCount; }

    const float*                 GetLODLevelRanges() const        { return m_morphEnd; }

    // Ugly brute-force mechanism - could be replaced by deterministic one possibly
    bool                          IsVisDistTooSmall() const        { return m_visDistTooSmall; }

    mutable int m_debugMaxMipLevelExceededCount = 0;

    void CalculateMipRequirements(float minDistance, float maxDistance, int& minMipReq, int& maxMipReq) const;

    enum FrustumIntersectResult
    {
        Outside,
        Intersect,
        Inside
    };
    FrustumIntersectResult CalculateFrustumIntersection(const AZ::Aabb& aabb) const;

    // To more quickly modify the quadtree for handling texture streaming calculations for terrain,
    // these texture streaming parameters have been added to the same LODSelection object used for
    // mesh LOD selection.
    // The codepath is very similar, and we also should do a cleanup pass on the fields on this struct
    struct TextureStreamingSelection
    {
        int m_textureTileSize = 1024;
        float m_texelsPerMeter = 512.0f;
        float m_screenPixelHeight = 1080.0f;
        float m_fov = DEG2RAD(50.0f);
    } m_textureStreamingParams;
};

template< int maxSelectionCount >
class LODSelectionOnStack
    : public LODSelection
{
    SelectedNode      m_selectionBufferOnStack[maxSelectionCount];

public:
    LODSelectionOnStack(const LODSelectionDesc& lodSelectionDesc)
        : LODSelection(m_selectionBufferOnStack, maxSelectionCount, lodSelectionDesc)
    { }
};


//////////////////////////////////////////////////////////////////////////
// Inline
//////////////////////////////////////////////////////////////////////////

void inline CDLODQuadTree::Node::GetBSphere(AZ::Vector3& sphereCenter, float& sphereRadiusSq, const CDLODQuadTree& quadTree, unsigned int x, unsigned int y, unsigned short size, unsigned short minZ, unsigned short maxZ)
{
    float sizeHalfX = size / 2.0f;
    float sizeHalfY = size / 2.0f;
    float sizeHalfZ = (maxZ - minZ) / 2.0f;

    const float midX = x + sizeHalfX;
    const float midY = y + sizeHalfY;
    const float midZ = minZ + sizeHalfZ;

    sphereCenter = quadTree.m_desc.m_mapDims.m_worldMin + AZ::Vector3(midX, midY, midZ);

    sizeHalfX = sizeHalfX;
    sizeHalfY = sizeHalfY;
    sizeHalfZ = sizeHalfZ;

    sphereRadiusSq = sizeHalfX * sizeHalfX + sizeHalfY * sizeHalfY + sizeHalfZ * sizeHalfZ;
}
//
void inline CDLODQuadTree::Node::GetWorldMinMaxZ(unsigned short rminZ, unsigned short rmaxZ, float& wminZ, float& wmaxZ, const MapDimensions& mapDims)
{
    wminZ = mapDims.MinZ() + rminZ * mapDims.SizeZ() / 65535.0f;
    wmaxZ = mapDims.MinZ() + rmaxZ * mapDims.SizeZ() / 65535.0f;
}

#endif
