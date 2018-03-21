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

#ifndef CRYINCLUDE_CRY3DENGINE_TERRAIN_SECTOR_H
#define CRYINCLUDE_CRY3DENGINE_TERRAIN_SECTOR_H
#pragma once

#define MML_NOT_SET ((uint8) - 1)

#define ARR_TEX_OFFSETS_SIZE 4


#include "BasicArea.h"
#include "Array2d.h"
#include "Terrain/Texture/MacroTexture.h"
#include <ITerrain.h>

class BuildMeshData;
class InPlaceIndexBuffer;
struct SSurfaceType;

struct DetailLayerMesh
{
    DetailLayerMesh()
    {
        memset(this, 0, sizeof(DetailLayerMesh));
    }

    SSurfaceType* surfaceType;
    _smart_ptr<IRenderMesh> triplanarMeshes[3];

    bool HasRM()
    {
        return triplanarMeshes[0] || triplanarMeshes[1] || triplanarMeshes[2];
    }

    int GetIndexCount();

    void DeleteRenderMeshes(IRenderer* pRend);

    void GetMemoryUsage(ICrySizer* pSizer) const {}
};

struct SurfaceTile
{
    SurfaceTile()
    {
        memset(this, 0, sizeof(SurfaceTile));
    }

    ~SurfaceTile()
    {
        delete[] m_Heightmap;
        m_Heightmap = nullptr;

        delete[] m_Weightmap;
        m_Weightmap = nullptr;
    }

    void ResetMaps(uint16 size)
    {
        delete [] m_Heightmap;
        delete [] m_Weightmap;

        const int sizeSqr = size * size;
        m_Heightmap = new uint16[sizeSqr];
        m_Weightmap = new ITerrain::SurfaceWeight[sizeSqr];
        m_Size = size;
    }

    void AssignMaps(uint16 size, uint16* heightmap, ITerrain::SurfaceWeight* weights)
    {
        m_Size = size;
        m_Heightmap = heightmap;
        m_Weightmap = weights;
    }

    static uint16* UpgradeHeightmap(int actualSize, int expectedSize, uint16* heightmap)
    {
        if (actualSize != expectedSize)
        {
            uint16* heightmapOld = heightmap;
            heightmap = new uint16[expectedSize * expectedSize];

            for (int y = 0; y < expectedSize; ++y)
            {
                int old_y = (y * actualSize) / expectedSize;
                for (int x = 0; x < expectedSize; ++x)
                {
                    int old_x = (x * actualSize) / expectedSize;

                    int old_index = old_y * actualSize * old_x;
                    int new_index = y * expectedSize * x;

                    heightmap[new_index] = heightmapOld[old_index];
                }
            }

            actualSize = expectedSize;
            delete [] heightmapOld;
        }

        return heightmap;
    }

    void SetRange(float offset, float range)
    {
        m_Offset = offset;
        m_Range = range;

        const unsigned mask16bit = (1 << 16) - 1;
        const int      inv1cm = 100;

        iOffset = int(offset * inv1cm);
        iRange = int(range * inv1cm);
        iStep = iRange ? (iRange + mask16bit - 1) / mask16bit : 1;

        iRange /= iStep;
    }

    inline void SetHeightByIndex(int index, float height)
    {
        const int inv1cm = 100;
        const int compressedHeight = int((height - m_Offset) * inv1cm) / iStep;
        m_Heightmap[index] = static_cast<uint16>(compressedHeight);
    }

    inline float GetHeight(int nX, int nY) const
    {
        int nMask = m_Size - 2;
        nX &= nMask;
        nY &= nMask;

        assert(nX < m_Size);
        assert(nY < m_Size);
        assert(m_Heightmap);

        return GetHeightByIndex(nX * m_Size + nY);
    }

    inline void GetHeightQuad(int x, int y, float afZ[4]) const
    {
        int nMask = m_Size - 2;
        x &= nMask;
        y &= nMask;
        afZ[0] = GetHeightByIndex((x + 0) * m_Size + (y + 0));
        afZ[1] = GetHeightByIndex((x + 1) * m_Size + (y + 0));
        afZ[2] = GetHeightByIndex((x + 0) * m_Size + (y + 1));
        afZ[3] = GetHeightByIndex((x + 1) * m_Size + (y + 1));
    }

    inline void SetWeightByIndex(int index, ITerrain::SurfaceWeight weight)
    {
        m_Weightmap[index] = weight;
    }

    inline const ITerrain::SurfaceWeight& GetWeight(int x, int y) const
    {
        assert(m_Weightmap);
        int nMask = m_Size - 2;
        x &= nMask;
        y &= nMask;
        return m_Weightmap[x * m_Size + y];
    }

    inline uint8 GetWeightOfSurfaceId(int x, int y, int surfaceId)
    {
        const ITerrain::SurfaceWeight& weight = GetWeight(x, y);

        if (weight.Ids[0] == surfaceId)
        {
            return weight.Weights[0];
        }
        if (weight.Ids[1] == surfaceId)
        {
            return weight.Weights[1];
        }
        if (weight.Ids[2] == surfaceId)
        {
            return weight.Weights[2];
        }

        return ITerrain::SurfaceWeight::Undefined;
    }

    inline float GetRange() const
    {
        return m_Range;
    }

    inline float GetOffset() const
    {
        return m_Offset;
    }

    inline uint16 GetSize() const
    {
        return m_Size;
    }

    inline const ITerrain::SurfaceWeight* GetWeightmap() const
    {
        return m_Weightmap;
    }

    inline const uint16* GetHeightmap() const
    {
        return m_Heightmap;
    }

    static const int Hole = 128;
    static const int MaxSurfaceCount = 129;

private:
    float m_Offset;
    float m_Range;

    int iOffset;
    int iRange;
    int iStep;

    uint16 m_Size;

    uint16* m_Heightmap;

    ITerrain::SurfaceWeight* m_Weightmap;

    inline float UncompressHeight(uint16 data) const
    {
        const float OneCM = 0.01f;
        return OneCM * iOffset + data * iStep * OneCM;
    }

    inline float GetHeightByIndex(int i) const
    {
        assert(m_Heightmap);
        return UncompressHeight(m_Heightmap[i]);
    }
};

template <class T>
class TPool
{
public:

    TPool(int nPoolSize)
    {
        m_nPoolSize = nPoolSize;
        m_pPool = new T[nPoolSize];
        m_lstFree.PreAllocate(nPoolSize, 0);
        m_lstUsed.PreAllocate(nPoolSize, 0);
        for (int i = 0; i < nPoolSize; i++)
        {
            m_lstFree.Add(&m_pPool[i]);
        }
    }

    ~TPool()
    {
        delete[] m_pPool;
    }

    void ReleaseObject(T* pInst)
    {
        if (m_lstUsed.Delete(pInst))
        {
            m_lstFree.Add(pInst);
        }
    }

    int GetUsedInstancesCount(int& nAll)
    {
        nAll = m_nPoolSize;
        return m_lstUsed.Count();
    }

    T* GetObject()
    {
        T* pInst = NULL;
        if (m_lstFree.Count())
        {
            pInst = m_lstFree.Last();
            m_lstFree.DeleteLast();
            m_lstUsed.Add(pInst);
        }
        else
        {
            assert(!"TPool::GetObject: Out of free elements error");
        }

        return pInst;
    }

    void GetMemoryUsage(class ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_lstFree);
        pSizer->AddObject(m_lstUsed);

        if (m_pPool)
        {
            for (int i = 0; i < m_nPoolSize; i++)
            {
                m_pPool[i].GetMemoryUsage(pSizer);
            }
        }
    }

    PodArray<T*> m_lstFree;
    PodArray<T*> m_lstUsed;
    T* m_pPool;
    int m_nPoolSize;
};

#define MAX_PROC_OBJ_CHUNKS_NUM 128

struct SProcObjChunk
    : public Cry3DEngineBase
{
    CVegetation* m_pInstances;
    SProcObjChunk();
    ~SProcObjChunk();
    void GetMemoryUsage(class ICrySizer* pSizer) const;
};

typedef TPool<SProcObjChunk> SProcObjChunkPool;

class CProcObjSector
    : public Cry3DEngineBase
{
public:
    CProcObjSector() { m_nProcVegetNum = 0; m_ProcVegetChunks.PreAllocate(32); }
    ~CProcObjSector();
    CVegetation* AllocateProcObject();
    void ReleaseAllObjects();
    int GetUsedInstancesCount(int& nAll) { nAll = m_ProcVegetChunks.Count(); return m_nProcVegetNum; }
    void GetMemoryUsage(ICrySizer* pSizer) const;

protected:
    PodArray<SProcObjChunk*> m_ProcVegetChunks;
    int m_nProcVegetNum;
};

typedef TPool<CProcObjSector> CProcVegetPoolMan;


struct STerrainNodeLeafData
{
    STerrainNodeLeafData()
    {
        memset(this, 0, sizeof(*this));
    }
    ~STerrainNodeLeafData();

    struct TextureParams
    {
        void Set(const SSectorTextureSet& set)
        {
            offsetX = set.fTexOffsetX;
            offsetY = set.fTexOffsetY;
            scale = set.fTexScale;
            id = set.nTex0;
        }

        float offsetX;
        float offsetY;
        float scale;
        uint32 id;
    };

    TextureParams m_TextureParams[MAX_RECURSION_LEVELS];

    int m_SurfaceAxisIndexCount[SurfaceTile::MaxSurfaceCount][4];
    PodArray<CTerrainNode*> m_Neighbors;
    PodArray<uint8> m_NeighborLods;
    _smart_ptr<IRenderMesh> m_pRenderMesh;
};

class CTerrainNode
    : public Cry3DEngineBase
    , public IShadowCaster
{
public:
    friend class CTerrain;
    friend class CTerrainUpdateDispatcher;

    virtual void Render(const SRendParams& RendParams, const SRenderingPassInfo& passInfo);
    const AABB GetBBox() const;
    virtual const AABB GetBBoxVirtual() { return GetBBox(); }
    virtual void FillBBox(AABB& aabb);
    virtual struct ICharacterInstance* GetEntityCharacter(unsigned int nSlot, Matrix34A* pMatrix = NULL, bool bReturnOnlyVisible = false) { return NULL; };
    virtual bool IsRenderNode() { return false; }
    virtual EERType GetRenderNodeType();

    void Init(int x1, int y1, int nNodeSize, CTerrainNode* pParent, bool bBuildErrorsTable);
    CTerrainNode()
        : m_TextureSet(0)
        , m_Parent()
        , m_pLeafData(NULL)
        , m_MeshData(NULL)
        , m_Children(0)
        , m_nLastTimeUsed(0)
        , m_nSetLodFrameId(0)
        , m_ZErrorFromBaseLOD(0)
        , m_pProcObjPoolPtr(0)
        , m_nGSMFrameId(0)
        , m_pRNTmpData(0)
    {
        memset(&m_DistanceToCamera, 0, sizeof(m_DistanceToCamera));
    }
    ~CTerrainNode();

    static void ResetStaticData();

    static void GetStaticMemoryUsage(ICrySizer* sizer);

    static CProcVegetPoolMan* GetProcObjPoolMan() { return m_pProcObjPoolMan; }

    static SProcObjChunkPool* GetProcObjChunkPool() { return m_pProcObjChunkPool; }

    static void SetProcObjPoolMan(CProcVegetPoolMan* pProcObjPoolMan) { m_pProcObjPoolMan = pProcObjPoolMan; }

    static void SetProcObjChunkPool(SProcObjChunkPool* pProcObjChunkPool) { m_pProcObjChunkPool = pProcObjChunkPool; }

    bool CheckVis(bool bAllIN, bool bAllowRenderIntoCBuffer, const SRenderingPassInfo& passInfo);

    void SetSectorTexture(unsigned int textureId);

    void CheckNodeGeomUnload(const SRenderingPassInfo& passInfo);

    IRenderMesh* MakeSubAreaRenderMesh(const Vec3& vPos, float fRadius, IRenderMesh* pPrevRenderMesh, _smart_ptr<IMaterial> pMaterial, bool bRecalIRenderMeshconst, const char* szLSourceName);

    void SetChildsLod(int nNewGeomLOD, const SRenderingPassInfo& passInfo);

    int GetAreaLOD(const SRenderingPassInfo& passInfo);

    bool CheckUpdateProcObjects(const SRenderingPassInfo& passInfo);

    void IntersectTerrainAABB(const AABB& aabbBox, PodArray<CTerrainNode*>& lstResult);
    void IntersectWithShadowFrustum(bool bAllIn, PodArray<CTerrainNode*>* plstResult, ShadowMapFrustum* pFrustum, const float fHalfGSMBoxSize, const SRenderingPassInfo& passInfo);
    void IntersectWithBox(const AABB& aabbBox, PodArray<CTerrainNode*>* plstResult);
    CTerrainNode* FindMinNodeContainingBox(const AABB& aabbBox);

    void UpdateDetailLayersInfo(bool bRecursive);
    void RemoveProcObjects(bool bRecursive = false);

    template<class T>
    int Load_T(T& f, int& nDataSize, EEndian eEndian, bool bSectorPalettes, SHotUpdateInfo* pExportInfo);
    int Load(uint8*& f, int& nDataSize, EEndian eEndian, bool bSectorPalettes, SHotUpdateInfo* pExportInfo);
    int Load(AZ::IO::HandleType& fileHandle, int& nDataSize, EEndian eEndian, bool bSectorPalettes, SHotUpdateInfo* pExportInfo);
    void ReleaseHoleNodes();

    int GetData(byte*& pData, int& nDataSize, EEndian eEndian, SHotUpdateInfo* pExportInfo);

    float GetSurfaceTypeAmount(Vec3 vPos, int nSurfType);
    void GetMemoryUsage(ICrySizer* pSizer) const;
    void GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& cstAABB);

    void ReleaseHeightMapGeometry(bool bRecursive = false, const AABB* pBox = NULL);
    void ResetHeightMapGeometry(bool bRecursive = false, const AABB* pBox = NULL);
    int GetSecIndex();

    uint32 GetLastTimeUsed() { return m_nLastTimeUsed; }

    const float GetDistance(const SRenderingPassInfo& passInfo);
    bool IsProcObjectsReady() { return m_bProcObjectsReady != 0; }

    int GetSectorSizeInHeightmapUnits() const;

    inline STerrainNodeLeafData* GetLeafData() { return m_pLeafData; }

    inline const SurfaceTile& GetSurfaceTile() const
    {
        return m_SurfaceTile;
    }

    void PropagateChangesToRoot();

    CTerrainNode* m_Children;
    CTerrainNode* m_Parent;

    AABB m_LocalAABB;

    // flags
    uint8 m_bProcObjectsReady : 1;
    uint8 m_bForceHighDetail : 1;
    uint8 m_bHasHoles : 2;
    uint8 m_bNoOcclusion : 1; // sector has visareas under terrain surface
    uint8 m_QueuedLOD, m_CurrentLOD, m_TextureLOD;
    uint8 m_nTreeLevel;

    uint32 m_nEditorDiffuseTex;

    uint16 m_nOriginX, m_nOriginY;
    int m_nLastTimeUsed;
    int m_nSetLodFrameId;

    float* m_ZErrorFromBaseLOD;

    SurfaceTile m_SurfaceTile;

    SSectorTextureSet m_TextureSet;

    int m_nGSMFrameId;

    float m_DistanceToCamera[MAX_RECURSION_LEVELS];
    int FTell(uint8*& f);
    int FTell(AZ::IO::HandleType& fileHandle);

    void BuildIndices_Wrapper(const SRenderingPassInfo& passInfo);
    void BuildVertices_Wrapper();

private:
    bool bMacroTextureExists() const;

    MacroTexture* GetMacroTexture();

    const MacroTexture* GetMacroTexture() const;

    void SetupTexturing(const SRenderingPassInfo& passInfo);

    void RequestTextures(const SRenderingPassInfo& passInfo);

    void SetLOD(const SRenderingPassInfo& passInfo);

    uint8 GetTextureLOD(float fDistance, const SRenderingPassInfo& passInfo);

    void UpdateDistance(const SRenderingPassInfo& passInfo);

    void DrawArray(const SRenderingPassInfo& passInfo);

    bool RenderSector(const SRenderingPassInfo& passInfo); // returns true only if the sector rendermesh is valid and does not need to be updated

    void BuildVertices(int step);

    void AddIndexAliased(int _x, int _y, int _step, int nSectorSize, PodArray<CTerrainNode*>* plstNeighbourSectors, InPlaceIndexBuffer& indices, const SRenderingPassInfo& passInfo);

    void BuildIndices(InPlaceIndexBuffer& si, PodArray<CTerrainNode*>* pNeighbourSectors, const SRenderingPassInfo& passInfo);

    void RenderSectorUpdate_Finish(const SRenderingPassInfo& passInfo);

    static void UpdateSurfaceRenderMeshes(
        const _smart_ptr<IRenderMesh> pSrcRM,
        struct SSurfaceType* pSurface,
        _smart_ptr<IRenderMesh>& pMatRM,
        int nProjectionId,
        PodArray<vtx_idx>& lstIndices,
        const char* szComment,
        int nNonBorderIndicesCount,
        const SRenderingPassInfo& passInfo);

    void CheckLeafData();

    MacroTexture::Region GetTextureRegion() const;

    bool RenderNodeHeightmap(const SRenderingPassInfo& passInfo);

    STerrainNodeLeafData* m_pLeafData;

    CProcObjSector* m_pProcObjPoolPtr;

    PodArray<DetailLayerMesh> m_DetailLayers;


    struct CRNTmpData* m_pRNTmpData;

    BuildMeshData* m_MeshData;

    static PodArray<vtx_idx> m_SurfaceIndices[SurfaceTile::MaxSurfaceCount][4];
    static CProcVegetPoolMan* m_pProcObjPoolMan;
    static SProcObjChunkPool* m_pProcObjChunkPool;

    static void SetupTexGenParams(SSurfaceType* pLayer, float* pOutParams, uint8 ucProjAxis, bool bOutdoor, float fTexGenScale = 1.f);

    static void GenerateIndicesForAllSurfaces(IRenderMesh * mesh, int surfaceAxisIndexCount[SurfaceTile::MaxSurfaceCount][4], BuildMeshData * meshData);
};


// Container to manager temp memory as well as running update jobs
class CTerrainUpdateDispatcher
    : public Cry3DEngineBase
{
public:
    CTerrainUpdateDispatcher();
    ~CTerrainUpdateDispatcher();

    void QueueJob(CTerrainNode*, const SRenderingPassInfo& passInfo);
    void SyncAllJobs(bool bForceAll, const SRenderingPassInfo& passInfo);
    bool Contains(CTerrainNode* pNode)
    {
        return (m_queuedJobs.Find(pNode) != -1 || m_arrRunningJobs.Find(pNode) != -1);
    };

    void GetMemoryUsage(ICrySizer* pSizer) const;

    void RemoveJob(CTerrainNode* pNode);

private:
    bool AddJob(CTerrainNode*, bool executeAsJob, const SRenderingPassInfo& passInfo);

    static const size_t TempPoolSize = (4U << 20);

    void* m_pHeapStorage;
    _smart_ptr<IGeneralMemoryHeap> m_pHeap;

    PodArray<CTerrainNode*>     m_queuedJobs;
    PodArray<CTerrainNode*>     m_arrRunningJobs;
};

#pragma pack(push,4)

struct STerrainNodeChunk
{
    int16   nChunkVersion;
    int16 bHasHoles;
    AABB    boxHeightmap;
    float fOffset;
    float fRange;
    int     nSize;
    int     nSurfaceTypesNum;

    AUTO_STRUCT_INFO
};

#pragma pack(pop)

#include "terrain.h"

inline const AABB CTerrainNode::GetBBox() const
{
    return AABB(m_LocalAABB.min, m_LocalAABB.max);
}

#endif // CRYINCLUDE_CRY3DENGINE_TERRAIN_SECTOR_H
