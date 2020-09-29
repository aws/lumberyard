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

#ifndef CRYINCLUDE_CRY3DENGINE_TERRAIN_H
#define CRYINCLUDE_CRY3DENGINE_TERRAIN_H
#pragma once

#include <ISerialize.h>
#include <ITerrain.h>

#include "Terrain/LegacyTerrainBase.h"
#include "Terrain/Texture/MacroTexture.h"
#include "Terrain/Texture/TexturePool.h"
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <AzCore/std/function/function_fwd.h> // for callbacks
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include "TerrainProfiler.h"
#include <HeightmapUpdateNotificationBus.h>

#include "terrain_sector.h"

// lowest level of the outdoor world
#define TERRAIN_BOTTOM_LEVEL 0

#define TERRAIN_BASE_TEXTURES_NUM 2

// max view distance for objects shadow is size of object multiplied by this number
#define OBJ_MAX_SHADOW_VIEW_DISTANCE_RATIO 4

#define TERRAIN_NODE_TREE_DEPTH 16

#define ARR_TEX_OFFSETS_SIZE_DET_MAT 16

enum
{
    nHMCacheSize = 64
};

class CTerrainUpdateDispatcher;

namespace TerrainConstants
{
    // This value is used to increase the height terrain bounding boxes slightly so that if they have
    // merged meshes on them that use terrain color, the color will stay correct even when the vegetation
    // itself is visible, but the terrain is below the camera and not visible. Any vegetation higher than this
    // that tries to use terrain color could turn black at certain angles.
    static const float coloredVegetationMaxSafeHeight = 8.0f;
};

struct SSurfaceType
{
    SSurfaceType()
    {
        memset(this, 0, sizeof(SSurfaceType));
        ucThisSurfaceTypeId = 255;
    }

    bool IsMaterial3D() { return pLayerMat && pLayerMat->GetSubMtlCount() == 3; }

    bool HasMaterial() { return pLayerMat != NULL; }

    _smart_ptr<IMaterial> GetMaterialOfProjection(uint8 ucProjAxis)
    {
        if (pLayerMat)
        {
            if (pLayerMat->GetSubMtlCount() == 3)
            {
                return pLayerMat->GetSubMtl(ucProjAxis - 'X');
            }
            else if (ucProjAxis == ucDefProjAxis)
            {
                return pLayerMat;
            }
        }

        return NULL;
    }

    float GetMaxMaterialDistanceOfProjection(uint8 ucProjAxis)
    {
        if (ucProjAxis == 'X' || ucProjAxis == 'Y')
        {
            return fMaxMatDistanceXY;
        }

        return fMaxMatDistanceZ;
    }

    char szName[128];
    _smart_ptr<IMaterial> pLayerMat;
    float fScale;
    PodArray<int> lstnVegetationGroups;
    float fMaxMatDistanceXY;
    float fMaxMatDistanceZ;
    float arrRECustomData[4][ARR_TEX_OFFSETS_SIZE_DET_MAT];
    uint8 ucDefProjAxis;
    uint8 ucThisSurfaceTypeId;
    float fCustomMaxDistance;
    AZ::Crc32 nameTag; //CRC32 of szName.
};

class CTerrain
    : public ITerrain
    , public LegacyTerrainBase
    , public LegacyTerrain::CryTerrainRequestBus::Handler
    , public AzFramework::Terrain::TerrainDataRequestBus::Handler
    , public LegacyTerrain::LegacyTerrainDataRequestBus::Handler
{
    friend class CTerrainNode;
public:
    using Meter = int;
    using MeterF = float;
    using Unit = int;

    ~CTerrain();

    static bool CreateTerrain(const STerrainInfo& terrainInfo);
    static void DestroyTerrain();
    static CTerrain* GetTerrain() { return m_pTerrain; }

    ///////////////////////////////////////////////////////////////////////////
    // AzFramework::Terrain::TerrainSystemRequestBus START
    AZ::Vector2 GetTerrainGridResolution() const override;
    AZ::Aabb GetTerrainAabb() const override;

    float GetHeight(AZ::Vector3 position, Sampler sampler = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
    float GetHeightFromFloats(float x
        , float y
        , AzFramework::Terrain::TerrainDataRequests::Sampler sampler = AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR
        , bool* terrainExistsPtr = nullptr) const override;

    AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeight(AZ::Vector3 position
        , Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
    AzFramework::SurfaceData::SurfaceTagWeight GetMaxSurfaceWeightFromFloats(float x
        , float y
        , AzFramework::Terrain::TerrainDataRequests::Sampler sampleFilter = AzFramework::Terrain::TerrainDataRequests::Sampler::BILINEAR
        , bool* terrainExistsPtr = nullptr) const override;
    const char* GetMaxSurfaceName(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;

    bool GetIsHoleFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR) const override;
    AZ::Vector3 GetNormal(AZ::Vector3 position, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
    AZ::Vector3 GetNormalFromFloats(float x, float y, Sampler sampleFilter = Sampler::BILINEAR, bool* terrainExistsPtr = nullptr) const override;
    // AzFramework::Terrain::TerrainSystemRequestBus END
    ///////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////
    // Legacy::Terrain LegacyTerrainDataRequestBus START
    void SetTerrainElevationAndSurfaceWeights(int left, int bottom, int areaSize, const float* heightmap, int weightmapSize, const ITerrain::SurfaceWeight* surfaceWeightSet) override
    {
        SetTerrainElevation(left, bottom, areaSize, heightmap, weightmapSize, surfaceWeightSet);
    }
    void LoadTerrainSurfacesFromXML(XmlNodeRef pDoc) override;
    int GetTerrainSectorSize() const override { return GetSectorSize(); }
    int GetTerrainSectorsTableSize() const override { return GetSectorsTableSize(); }
    int GetTerrainSurfaceId(int x, int y) const override { return GetSurfaceWeight(x, y).PrimaryId(); }
    bool IsTerrainMeshQuadFlipped(int x, int y, int nUnitSize) const override { return IsMeshQuadFlipped(x, y, nUnitSize); }
    void GetTerrainMaterials(AZStd::vector<_smart_ptr<IMaterial>>& materials) override { return GetMaterials(materials); };
    int GetNodesData(byte*& pData, int& nDataSize, EEndian eEndian, SHotUpdateInfo* pExportInfo) override;
    ITerrainNode* FindMinNodeContainingBox(const AABB& someBox) override;
    void IntersectWithBox(const AABB& aabbBox, PodArray<ITerrainNode*>* plstResult) override;
    void IntersectWithShadowFrustum(PodArray<ITerrainNode*>* plstResult, ShadowMapFrustum* pFrustum, const SRenderingPassInfo& passInfo) override;
    void AddVisSector(ITerrainNode* newsec) override;
    void MarkAllSectorsAsUncompiled() override;
    void ResetTerrainVertBuffers() override;
    int GetActiveProcObjNodesCount() const override { return m_lstActiveProcObjNodes.Count(); }
    float GetDistanceToSectorWithWater() const override { return m_fDistanceToSectorWithWater; }
    void ClearVisSectors() override;
    void UpdateNodesIncrementally(const SRenderingPassInfo& passInfo) override;
    void CheckVis(const SRenderingPassInfo& passInfo) override;
    void ClearTextureSetsAndDrawVisibleSectors(bool clearTextureSets, const SRenderingPassInfo& passInfo) override;
    void UpdateSectorMeshes(const SRenderingPassInfo& passInfo) override;
    void CheckNodesGeomUnload(const SRenderingPassInfo& passInfo) override;
    bool IsOceanVisible() const override { return m_bOceanIsVisible != 0; }
    bool TryGetTextureStatistics(LegacyTerrain::MacroTexture::TileStatistics& statistics) const override;
    bool RayTrace(Vec3 const& vStart, Vec3 const& vEnd, LegacyTerrain::SRayTrace* prt) override;
    bool RenderArea(Vec3 vPos, float fRadius, _smart_ptr<IRenderMesh>& arrLightRenderMeshs, CRenderObject* pObj, _smart_ptr<IMaterial> pMaterial, const char* szComment, float* pCustomData, Plane* planes, const SRenderingPassInfo& passInfo) override;
    bool IsTextureStreamingInProgress() const override;
    float GetSlope(int x, int y) const override;
    void SetTerrainSectorTexture(int nTexSectorX, int nTexSectorY, unsigned int textureId, unsigned int textureSizeX, unsigned int textureSizeY, bool bMergeNotAllowed) override;
    void CloseTerrainTextureFile() override;
    bool ReadMacroTextureFile(const char* filepath, LegacyTerrain::MacroTextureConfiguration& configuration) const override;
    void GetMemoryUsage(class ICrySizer* pSizer) const override;
    void GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& crstAABB) override;
    // Legacy::Terrain LegacyTerrainDataRequestBus END
    ///////////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////////////
    // ITerrain START
    void SetTerrainElevation(int left, int bottom, int areaSize, const float* heightmap, int weightmapSize, const ITerrain::SurfaceWeight* surfaceWeightSet) override;
    void GetMaterials(AZStd::vector<_smart_ptr<IMaterial>>& materials) override;

    ITerrain::SurfaceWeight GetSurfaceWeight(Meter x, Meter y) const override;
    Vec3 GetTerrainSurfaceNormal(Vec3 vPos, float fRange) const override;
    void GetTerrainAlignmentMatrix(const Vec3& vPos, const float amount, Matrix33& matrix33) const override;

    bool IsHole(int x, int y) const override;
    bool IsMeshQuadFlipped(const int x, const int y, const int nUnitSize) const override;
    // ITerrain END
    ///////////////////////////////////////////////////////////////////////////

    inline static const int GetTerrainSize();
    inline static const int GetSectorSize();
    inline static const int GetHeightMapUnitSize();
    inline static const int GetSectorsTableSize();
    inline static const float GetInvUnitSize();
    inline const int GetTerrainUnits() const;

    inline CTerrainNode* GetRootNode();
    CTerrainNode* GetLeafNodeAt(Meter x, Meter y);
    inline CTerrainNode* GetLeafNodeAt(const Vec3& pos);

    bool HasMacroTexture() { return m_MacroTexture.get() != nullptr; }

    _smart_ptr<IRenderMesh> MakeAreaRenderMesh(const Vec3& vPos, float fRadius, _smart_ptr<IMaterial> pMat, const char* szLSourceName, Plane* planes);

    void DrawVisibleSectors(const SRenderingPassInfo& passInfo);
    void ClearTextureSets();

    void SendLegacyTerrainUpdateNotifications(int tileMinX, int tileMinY, int tileMaxX, int tileMaxY);

    void SetDetailLayerProperties(int nId, float fScaleX, float fScaleY, uint8 ucProjAxis, const char* szSurfName, const PodArray<int>& lstnVegetationGroups, _smart_ptr<IMaterial> pMat);
    void LoadSurfaceTypesFromXML(XmlNodeRef pDoc);
    void UpdateSurfaceTypes();

    int FindMinNodesContainingBox(const AABB& someBox, PodArray<CTerrainNode*>& arrNodes);

    inline SSurfaceType* GetSurfaceTypes();

    inline int GetTerrainTextureNodeSizeMeters();

    int m_nWhiteTexId;
    int m_nBlackTexId;

    PodArray<Array2d<CTerrainNode*> > m_NodePyramid;

    void ActivateNodeTexture(CTerrainNode* pNode, const SRenderingPassInfo& passInfo);
    void ActivateNodeProcObj(CTerrainNode* pNode);

    // LegacyTerrain::CryTerrainRequestBus
    void RequestTerrainUpdate() override;

    //! Returns the number of CTerrainNode that were loaded from "f".
    template <class T>
    int Load_T(XmlNodeRef pDoc, T& f, int& nDataSize, const STerrainInfo& terrainInfo, bool bHotUpdate, bool bHMap, bool bSectorPalettes, EEndian eEndian, SHotUpdateInfo* pExportInfo, bool loadMacroTexture = false)
    {
        TERRAIN_SCOPE_PROFILE(AZ::Debug::ProfileCategory::LegacyTerrain, LegacyTerrain::Debug::StatisicLoadTimeFromDisk);

        LoadSurfaceTypesFromXML(pDoc);

        // get terrain settings
        terrainInfo.LoadTerrainSettings(m_nUnitSize, m_fInvUnitSize, m_nTerrainSize,
                                        m_MeterToUnitBitShift, m_nTerrainSizeDiv,
                                        m_nSectorSize, m_nSectorsTableSize, m_UnitToSectorBitShift);

        if (bHMap && !m_RootNode)
        {
            LOADING_TIME_PROFILE_SECTION_NAMED("BuildSectorsTree");

            // build nodes tree in fast way
            BuildSectorsTree(false);
        }

        if (!bHotUpdate)
        {
            AZ_Printf("LegacyTerrain", "===== Initializing terrain nodes ===== ");
        }
        int nNodesLoaded = 1;

        if (m_RootNode)
        {
            if (bHMap)
            {
                nNodesLoaded = m_RootNode->Load_T(f, nDataSize, eEndian, bSectorPalettes, pExportInfo);

                if (nNodesLoaded > 0)
                {
                    // pass heightmap to the physics
                    InitHeightfieldPhysics();
                }
            }
            // reopen texture file if needed, texture pack may be randomly closed by editor so automatic reopening used
            if (loadMacroTexture && !m_MacroTexture)
            {
                OpenTerrainTextureFile(COMPILED_TERRAIN_TEXTURE_FILE_NAME);
            }
        }

        if (bHMap)
        {
            LOADING_TIME_PROFILE_SECTION_NAMED("PostTerrain");
            ResetTerrainVertBuffers();
        }

        int numTiles = CTerrain::m_NodePyramid[0].GetSize();
        SendLegacyTerrainUpdateNotifications(0, 0, numTiles, numTiles);
        AZ::HeightmapUpdateNotificationBus::Broadcast(&AZ::HeightmapUpdateNotificationBus::Events::HeightmapModified, AZ::Aabb::CreateNull());

        return nNodesLoaded;
    }

private:
    CTerrain(const STerrainInfo& terrainInfo);
    CTerrain() = delete;

    inline void Clamp_Unit(Unit& x, Unit& y) const;
    float GetZ_Unit(Unit nX_units, Unit nY_units) const;
    ITerrain::SurfaceWeight GetSurfaceWeight_Units(Unit nX_units, Unit nY_units) const;
    float GetSlope_Unit(Unit nX_units, Unit nY_units) const;
    inline Unit SectorSize_Units() const;

    float GetZ(Meter x, Meter y) const;
    virtual float GetBilinearZ(MeterF x1, MeterF y1) const;

    Vec3 GetTerrainNormal(int x, int y) const;

    inline CTerrainNode* GetLeafNodeAt_Units(Unit xu, Unit yu);
    inline const CTerrainNode* GetLeafNodeAt_Units(Unit xu, Unit yu) const;
    inline CTerrainNode* GetLeafNodeAt_Units(Unit xu, Unit yu, int nUnitsToSectorBitShift);
    inline const CTerrainNode* GetLeafNodeAt_Units(Unit xu, Unit yu, int nUnitsToSectorBitShift) const;

    friend class CTerrainUpdateDispatcher;

    void ProcessTextureStreamingRequests(const SRenderingPassInfo& passInfo);

    void TraverseTree(AZStd::function<void(CTerrainNode*)> callback);

    int m_nLoadedSectors;
    int m_bOceanIsVisible;
    float m_fDistanceToSectorWithWater;

    MacroTexture::UniquePtr m_MacroTexture;

    _smart_ptr<IMaterial> m_pTerrainEf;

    PodArray<CTerrainNode*> m_lstVisSectors;
    PodArray<CTerrainNode*> m_lstUpdatedSectors;

    PodArray<SSurfaceType> m_SurfaceTypes;

    static CTerrain* m_pTerrain; //Pointer to the singleton.
    static int      m_nUnitSize;    // in meters
    static float    m_fInvUnitSize; // in 1/meters
    static int      m_nTerrainSize; // in meters
    int             m_nTerrainSizeDiv;
    static int      m_nSectorSize;  // in meters
    static int      m_nSectorsTableSize;    // sector width/height of the finest LOD level (sector count is the square of this value)

    AZ::Aabb        m_aabb;

    PodArray<CTerrainNode*> m_lstActiveTextureNodes;
    PodArray<CTerrainNode*> m_lstActiveProcObjNodes;

    CTerrainUpdateDispatcher* m_pTerrainUpdateDispatcher;

    CTerrainNode* m_RootNode;

    PodArray<CTerrainNode*> m_lstSectors;

#if !defined(_RELEASE) && defined(AZ_STATISTICAL_PROFILING_ENABLED)
    LegacyTerrain::Debug::TerrainProfiler m_terrainProfiler;
#endif

    static void BuildErrorsTableForArea(
        float* pLodErrors, int nMaxLods,
        int X1, int Y1, int X2, int Y2,
        const float* heightmap,
        bool sectorHasHoles,
        bool sectorHasMeshData);

    void BuildSectorsTree(bool bBuildErrorsTable);
    bool OpenTerrainTextureFile(const char* szFileName);

    int m_MeterToUnitBitShift;
    int m_UnitToSectorBitShift;

    static float GetHeightFromTerrain_Callback(int ix, int iy);
    static unsigned char GetSurfaceTypeFromTerrain_Callback(int ix, int iy);

    struct SCachedHeight
    {
        // Initialize the cached coordinates to 0xFFFF, which is an invalid value.
        // (The heightmap supports a max value of 4096)  If we initialize to 0, it
        // could be mistaken as a valid cached entry.
        SCachedHeight() { x = y = 0xFFFF; fHeight = 0; }
        uint16 x, y;
        float fHeight;
    };

    struct SCachedSurfType
    {
        // Initialize the cached coordinates to 0xFFFF, which is an invalid value.
        // (The heightmap supports a max value of 4096)  If we initialize to 0, it
        // could be mistaken as a valid cached entry.
        SCachedSurfType() { x = y = 0xFFFF; surfType = 0; }
        uint16 x, y;
        uint32 surfType;
    };

    static SCachedHeight m_arrCacheHeight[nHMCacheSize * nHMCacheSize] _ALIGN(128);
    static SCachedSurfType m_arrCacheSurfType[nHMCacheSize * nHMCacheSize] _ALIGN(128);

    void ResetHeightMapCache()
    {
        // Wipe cache to 0xff, otherwise CTerrain::GetHeightFromUnits_Callback and CTerrrain::GetSurfaceTypeFromUnits_Callback
        // incorrectly uses the cache for x=0, y=0 coordinate despite having no value written.
        memset(m_arrCacheHeight, 0xFF, sizeof(m_arrCacheHeight));
        memset(m_arrCacheSurfType, 0xFF, sizeof(m_arrCacheSurfType));
    }

    void InitHeightfieldPhysics();
    void ClearHeightfieldPhysics();
};


inline const int CTerrain::GetTerrainSize()
{
    return m_nTerrainSize;
}

inline const int CTerrain::GetSectorSize()
{
    return m_nSectorSize;
}

inline const int CTerrain::GetHeightMapUnitSize()
{
    return m_nUnitSize;
}

inline const int CTerrain::GetSectorsTableSize()
{
    return m_nSectorsTableSize;
}

inline const float CTerrain::GetInvUnitSize()
{
    return m_fInvUnitSize;
}

inline const int CTerrain::GetTerrainUnits() const
{
    return m_nTerrainSizeDiv;
}

inline void CTerrain::Clamp_Unit(Unit& x, Unit& y) const
{
    x = (Unit)x < 0 ? 0 : (Unit)x < GetTerrainUnits() ? x : GetTerrainUnits();
    y = (Unit)y < 0 ? 0 : (Unit)y < GetTerrainUnits() ? y : GetTerrainUnits();
}

inline CTerrainNode* CTerrain::GetRootNode()
{
    return m_RootNode;
}

inline CTerrainNode* CTerrain::GetLeafNodeAt(Meter x, Meter y)
{
    if (x < 0 || y < 0 || x >= m_nTerrainSize || y >= m_nTerrainSize || !m_RootNode)
    {
        return 0;
    }

    return GetLeafNodeAt_Units(x >> m_MeterToUnitBitShift, y >> m_MeterToUnitBitShift);
}

inline CTerrainNode* CTerrain::GetLeafNodeAt(const Vec3& pos)
{
    return GetLeafNodeAt((Meter)pos.x, (Meter)pos.y);
}

inline SSurfaceType* CTerrain::GetSurfaceTypes()
{
    return &m_SurfaceTypes[0];
}

inline int CTerrain::GetTerrainTextureNodeSizeMeters()
{
    return GetSectorSize();
}

inline CTerrain::Unit CTerrain::SectorSize_Units() const
{
    return (1 << m_UnitToSectorBitShift) + 1;
}

inline const CTerrainNode* CTerrain::GetLeafNodeAt_Units(Unit xu, Unit yu) const
{
    if (m_NodePyramid.size() == 0)
    {
        return nullptr;
    }

    if (m_NodePyramid[0].GetSize() == 0)
    {
        return nullptr;
    }

    return m_NodePyramid[0][xu >> m_UnitToSectorBitShift][yu >> m_UnitToSectorBitShift];
}

inline CTerrainNode* CTerrain::GetLeafNodeAt_Units(Unit xu, Unit yu)
{
    return const_cast<CTerrainNode*>(static_cast<const CTerrain*>(this)->GetLeafNodeAt_Units(xu, yu));
}

inline const CTerrainNode* CTerrain::GetLeafNodeAt_Units(Unit xu, Unit yu, int nUnitsToSectorBitShift) const
{
    return m_NodePyramid[0][xu >> nUnitsToSectorBitShift][yu >> nUnitsToSectorBitShift];
}

inline CTerrainNode* CTerrain::GetLeafNodeAt_Units(Unit xu, Unit yu, int nUnitsToSectorBitShift)
{
    return const_cast<CTerrainNode*>(static_cast<const CTerrain*>(this)->GetLeafNodeAt_Units(xu, yu, nUnitsToSectorBitShift));
}

#endif // CRYINCLUDE_CRY3DENGINE_TERRAIN_H
