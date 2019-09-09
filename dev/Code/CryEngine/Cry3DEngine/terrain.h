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

#include <Terrain/Texture/MacroTexture.h>
#include <Terrain/Texture/TexturePool.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include <AzCore/std/function/function_fwd.h> // for callbacks
#include "Environment/OceanEnvironmentBus.h"

// lowest level of the outdoor world
#define TERRAIN_BOTTOM_LEVEL 0

#define TERRAIN_BASE_TEXTURES_NUM 2

// max view distance for objects shadow is size of object multiplied by this number
#define OBJ_MAX_SHADOW_VIEW_DISTANCE_RATIO 4

#define TERRAIN_NODE_TREE_DEPTH 16

#define OCEAN_IS_VERY_FAR_AWAY 1000000.f

#define ARR_TEX_OFFSETS_SIZE_DET_MAT 16

enum
{
    nHMCacheSize = 64
};

class CTerrainUpdateDispatcher;
class COcean;

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
    _smart_ptr<CMatInfo> pLayerMat;
    float fScale;
    PodArray<int> lstnVegetationGroups;
    float fMaxMatDistanceXY;
    float fMaxMatDistanceZ;
    float arrRECustomData[4][ARR_TEX_OFFSETS_SIZE_DET_MAT];
    uint8 ucDefProjAxis;
    uint8 ucThisSurfaceTypeId;
    float fCustomMaxDistance;
};

class CTerrainNode;

#pragma pack(push,4)

//! structure to vegetation group properties loading/saving
struct StatInstGroupChunk
{
    StatInstGroupChunk()
    {
        ZeroStruct(*this);
    }
    char  szFileName[256];
    float fBending;
    float fSpriteDistRatio;
    float fShadowDistRatio;
    float fMaxViewDistRatio;
    float   fBrightness;
    int32 nRotationRangeToTerrainNormal; // applied to a vegetation object that has been realigned in the terrain's Y/X direction
    float fAlignToTerrainCoefficient;
    uint32  nMaterialLayers;

    float fDensity;
    float fElevationMax;
    float fElevationMin;
    float fSize;
    float fSizeVar;
    float fSlopeMax;
    float fSlopeMin;

    float fStatObjRadius_NotUsed;
    int nIDPlusOne; // For backward compatibility, we need to save ID + 1

    float fLodDistRatio;
    uint32  nReserved;

    int nFlags;
    int nMaterialId;

    //! flags similar to entity render flags
    int m_dwRndFlags;

    float fStiffness;
    float fDamping;
    float fVariance;
    float fAirResistance;

    AUTO_STRUCT_INFO_LOCAL
};

struct SNameChunk
{
    SNameChunk() { memset(this, 0, sizeof(SNameChunk)); }

    char szFileName[256];

    AUTO_STRUCT_INFO_LOCAL
};

#pragma pack(pop)

class CTerrain
    : public ITerrain
    , public Cry3DEngineBase
    , public LegacyTerrain::CryTerrainRequestBus::Handler
{
    friend class CTerrainNode;
public:
    using Meter = int;
    using MeterF = float;
    using Unit = int;

    CTerrain(const STerrainInfo& TerrainInfo);
    ~CTerrain();

    virtual float GetZ(Meter x, Meter y) const;
    virtual float GetBilinearZ(MeterF x1, MeterF y1) const;

    virtual SurfaceWeight GetSurfaceWeight(Meter x, Meter y) const;

    virtual Vec3 GetTerrainSurfaceNormal(Vec3 vPos, float fRange);
    virtual void GetTerrainAlignmentMatrix(const Vec3& vPos, const float amount, Matrix33& matrix33);

    void GetMaterials(AZStd::vector<_smart_ptr<IMaterial>>& materials) override;

    virtual bool IsHole(Meter x, Meter y) const;
    virtual bool IsMeshQuadFlipped(const Meter x, const Meter y, const Meter nUnitSize) const;

    struct SRayTrace
    {
        float t;
        Vec3  hitPoint;
        Vec3  hitNormal;
        _smart_ptr<IMaterial> material;

        SRayTrace()
            : t(0)
            , hitPoint(0, 0, 0)
            , hitNormal(0, 0, 1)
            , material(nullptr)
        {}

        SRayTrace(float t_, Vec3 const& hitPoint_, Vec3 const& hitNormal_, _smart_ptr<IMaterial> material_)
            : t(t_)
            , hitPoint(hitPoint_)
            , hitNormal(hitNormal_)
            , material(material_)
        {}
    };

    bool RayTrace(Vec3 const& vStart, Vec3 const& vEnd, SRayTrace* prt);

    void InitHeightfieldPhysics();

    inline static const int GetTerrainSize();
    inline static const int GetSectorSize();
    inline static const int GetHeightMapUnitSize();
    inline static const int GetSectorsTableSize();
    inline static const float GetInvUnitSize();
    inline const int GetTerrainUnits() const;

    inline CTerrainNode* GetRootNode();
    CTerrainNode* GetLeafNodeAt(Meter x, Meter y);
    inline CTerrainNode* GetLeafNodeAt(const Vec3& pos);

    bool SetCompiledData(byte* pData, int nDataSize, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial>>** ppMatTable, bool bHotUpdate, SHotUpdateInfo* pExportInfo) override;
    bool GetCompiledData(byte* pData, int nDataSize, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial>>** ppMatTable, std::vector<struct IStatInstGroup*>** ppStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo) override;
    int  GetCompiledDataSize(SHotUpdateInfo* pExportInfo) override;
    void GetStatObjAndMatTables(DynArray<IStatObj*>* pStatObjTable, DynArray<_smart_ptr<IMaterial>>* pMatTable, DynArray<IStatInstGroup*>* pStatInstGroupTable, uint32 nObjTypeMask) override;

    void GetMemoryUsage(class ICrySizer* pSizer) const;

    int GetActiveProcObjNodesCount() { return m_lstActiveProcObjNodes.Count(); }

    bool TryGetTextureStatistics(MacroTexture::TileStatistics& statistics) const;
    bool IsTextureStreamingInProgress() const;
    void CloseTerrainTextureFile();
    void SetTerrainSectorTexture(int nTexSectorX, int nTexSectorY, unsigned int textureId, bool bMergeNotAllowed);

    _smart_ptr<IRenderMesh> MakeAreaRenderMesh(const Vec3& vPos, float fRadius, _smart_ptr<IMaterial> pMat, const char* szLSourceName, Plane* planes);
    bool RenderArea(Vec3 vPos, float fRadius, _smart_ptr<IRenderMesh>& arrLightRenderMeshs, CRenderObject* pObj, _smart_ptr<IMaterial> pMaterial, const char* szComment, float* pCustomData, Plane* planes, const SRenderingPassInfo& passInfo);

    void DrawVisibleSectors(const SRenderingPassInfo& passInfo);
    void UpdateNodesIncrementaly(const SRenderingPassInfo& passInfo);
    void CheckNodesGeomUnload(const SRenderingPassInfo& passInfo);
    void ResetTerrainVertBuffers();
    void UpdateSectorMeshes(const SRenderingPassInfo& passInfo);
    void CheckVis(const SRenderingPassInfo& passInfo);
    void AddVisSector(CTerrainNode* newsec);
    void ClearVisSectors();
    void ClearTextureSets();

    void SetTerrainElevation(int x1, int y1, int areaSize, const float* heightmap, int weightmapSize, const SurfaceWeight* weightmap) override;
    void SendLegacyTerrainUpdateNotifications(int tileMinX, int tileMinY, int tileMaxX, int tileMaxY);

    void SetDetailLayerProperties(int nId, float fScaleX, float fScaleY, uint8 ucProjAxis, const char* szSurfName, const PodArray<int>& lstnVegetationGroups, _smart_ptr<IMaterial> pMat);
    void LoadSurfaceTypesFromXML(XmlNodeRef pDoc);
    void UpdateSurfaceTypes();

    CTerrainNode* FindMinNodeContainingBox(const AABB& someBox);
    int FindMinNodesContainingBox(const AABB& someBox, PodArray<CTerrainNode*>& arrNodes);
    void IntersectWithShadowFrustum(PodArray<CTerrainNode*>* plstResult, ShadowMapFrustum* pFrustum, const SRenderingPassInfo& passInfo);
    void IntersectWithBox(const AABB& aabbBox, PodArray<CTerrainNode*>* plstResult);
    void MarkAllSectorsAsUncompiled();

    void GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& crstAABB);

    void GetVegetationMaterials(std::vector<_smart_ptr<IMaterial>>*& pMatTable);
    void LoadVegetationData(PodArray<struct StatInstGroup>& rTable, PodArray<StatInstGroupChunk>& lstFileChunks, int i);
    virtual IRenderNode* AddVegetationInstance(int nStaticGroupID, const Vec3& vPos, const float fScale, uint8 ucBright, uint8 angle, uint8 angleX, uint8 angleY) override;

    inline SSurfaceType* GetSurfaceTypes();

    inline int GetTerrainTextureNodeSizeMeters();

    int m_nWhiteTexId;
    int m_nBlackTexId;

    PodArray<Array2d<CTerrainNode*> > m_NodePyramid;

    void ActivateNodeTexture(CTerrainNode* pNode, const SRenderingPassInfo& passInfo);
    void ActivateNodeProcObj(CTerrainNode* pNode);

    bool Load(AZ::IO::HandleType fileHandle, int nDataSize, struct STerrainChunkHeader* pTerrainChunkHeader, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial>>** ppMatTable);

    // REFACTOR IN PROGRESS
    //////////////////////////////////////////////////////////////////////////

    // (bethelz) TODO: These belong in 3DEngine
    static void RemoveAllStaticObjects();
    static bool RemoveObjectsInArea(Vec3 vExploPos, float fExploRadius);
    static void GetObjectsAround(Vec3 vPos, float fRadius, PodArray<struct SRNInfo>* pEntList, bool bSkip_ERF_NO_DECALNODE_DECALS, bool bSkipDynamicObjects);
    static bool Recompile_Modified_Incrementaly_RoadRenderNodes();
    //

    // (bethelz) TODO: Remove global ocean dependency
    inline float GetWaterLevel();
    inline void SetWaterLevel(float fOceanWaterLevel);
    inline bool IsOceanVisible() const;
    inline COcean* GetOcean();
    inline float GetDistanceToSectorWithWater() const;
    int UpdateOcean(const SRenderingPassInfo& passInfo);
    int RenderOcean(const SRenderingPassInfo& passInfo);
    void InitTerrainWater(_smart_ptr<IMaterial> pTerrainWaterMat) override;
    virtual void SetOceanWaterLevel(float fOceanWaterLevel) override;
    virtual void ChangeOceanMaterial(_smart_ptr<IMaterial> pMat) override;
    //

    // (bethelz) TODO: Move to utility class.
    template <class T>
    static bool LoadDataFromFile(T* data, size_t elems, AZ::IO::HandleType& fileHandle, int& nDataSize, EEndian eEndian, int* pSeek = 0);
    static bool LoadDataFromFile_Seek(size_t elems, AZ::IO::HandleType& fileHandle, int& nDataSize, EEndian eEndian);

    template <class T>
    static bool LoadDataFromFile(T* data, size_t elems, uint8*& f, int& nDataSize, EEndian eEndian, int* pSeek = 0);
    static bool LoadDataFromFile_Seek(size_t elems, uint8*& f, int& nDataSize, EEndian eEndian);
    static void LoadDataFromFile_FixAlignment(AZ::IO::HandleType& fileHandle, int& nDataSize);
    static void LoadDataFromFile_FixAlignment(uint8*& f, int& nDataSize);
    //

    // LegacyTerrain::CryTerrainRequestBus
    void RequestTerrainUpdate() override;

private:
    template <class T>
    bool Load_T(T& f, int& nDataSize, STerrainChunkHeader* pTerrainChunkHeader, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial>>** ppMatTable, bool bHotUpdate, SHotUpdateInfo* pExportInfo);

    int GetTablesSize(SHotUpdateInfo* pExportInfo);
    void SaveTables(byte*& pData, int& nDataSize, std::vector<struct IStatObj*>*& pStatObjTable, std::vector<_smart_ptr<IMaterial>>*& pMatTable, std::vector<struct IStatInstGroup*>*& pStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo);

    inline void Clamp_Unit(Unit& x, Unit& y) const;
    float GetZ_Unit(Unit nX_units, Unit nY_units) const;
    SurfaceWeight GetSurfaceWeight_Units(Unit nX_units, Unit nY_units) const;

    inline Unit SectorSize_Units() const;

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

    float   m_fOceanWaterLevel;

    PodArray<CTerrainNode*> m_lstVisSectors;
    PodArray<CTerrainNode*> m_lstUpdatedSectors;

    PodArray<SSurfaceType> m_SurfaceTypes;

    static int      m_nUnitSize;    // in meters
    static float    m_fInvUnitSize; // in 1/meters
    static int      m_nTerrainSize; // in meters
    int             m_nTerrainSizeDiv;
    static int      m_nSectorSize;  // in meters
    static int      m_nSectorsTableSize;    // sector width/height of the finest LOD level (sector count is the square of this value)

    class COcean*  m_pOcean;

    PodArray<CTerrainNode*> m_lstActiveTextureNodes;
    PodArray<CTerrainNode*> m_lstActiveProcObjNodes;

    CTerrainUpdateDispatcher* m_pTerrainUpdateDispatcher;

    CTerrainNode* m_RootNode;

    PodArray<CTerrainNode*> m_lstSectors;

    static void BuildErrorsTableForArea(
        float* pLodErrors, int nMaxLods,
        int X1, int Y1, int X2, int Y2,
        const float* heightmap,
        int weightmapSize,
        const SurfaceWeight* weightmap);

    void BuildSectorsTree(bool bBuildErrorsTable);
    bool OpenTerrainTextureFile(const char* szFileName);

    int m_MeterToUnitBitShift;
    int m_UnitToSectorBitShift;

    static float GetHeightFromUnits_Callback(int ix, int iy);
    static unsigned char GetSurfaceTypeFromUnits_Callback(int ix, int iy);

    struct SCachedHeight
    {
        SCachedHeight() { x = y = 0; fHeight = 0; }
        uint16 x, y;
        float fHeight;
    };

    struct SCachedSurfType
    {
        SCachedSurfType() { x = y = 0; surfType = 0;}
        uint16 x, y;
        uint32 surfType;
    };

    static SCachedHeight m_arrCacheHeight[nHMCacheSize * nHMCacheSize] _ALIGN(128);
    static SCachedSurfType m_arrCacheSurfType[nHMCacheSize * nHMCacheSize] _ALIGN(128);

    void ResetHeightMapCache()
    {
        memset(m_arrCacheHeight, 0, sizeof(m_arrCacheHeight));
        memset(m_arrCacheSurfType, 0, sizeof(m_arrCacheSurfType));
    }
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

inline float CTerrain::GetWaterLevel()
{
    return OceanToggle::IsActive() ? OceanRequest::GetOceanLevel() : m_fOceanWaterLevel;
}

inline void CTerrain::SetWaterLevel(float fOceanWaterLevel)
{
    m_fOceanWaterLevel = fOceanWaterLevel;
}

inline bool CTerrain::IsOceanVisible() const
{
    return m_bOceanIsVisible != 0;
}

inline COcean* CTerrain::GetOcean()
{
    return m_pOcean;
}

inline float CTerrain::GetDistanceToSectorWithWater() const
{
    return m_fDistanceToSectorWithWater;
}

inline SSurfaceType* CTerrain::GetSurfaceTypes()
{
    return &m_SurfaceTypes[0];
}

inline int CTerrain::GetTerrainTextureNodeSizeMeters()
{
    return GetSectorSize();
}

template <class T>
bool CTerrain::LoadDataFromFile(T* data, size_t elems, AZ::IO::HandleType& fileHandle, int& nDataSize, EEndian eEndian, int* pSeek)
{
    if (pSeek)
    {
        *pSeek = GetPak()->FTell(fileHandle);
    }

    if (GetPak()->FRead(data, elems, fileHandle, eEndian) != elems)
    {
        assert(0);
        return false;
    }
    nDataSize -= sizeof(T) * elems;
    assert(nDataSize >= 0);
    return true;
}

inline bool CTerrain::LoadDataFromFile_Seek(size_t elems, AZ::IO::HandleType& fileHandle, int& nDataSize, EEndian eEndian)
{
    GetPak()->FSeek(fileHandle, elems, SEEK_CUR);
    nDataSize -= elems;
    assert(nDataSize >= 0);
    return (nDataSize >= 0);
}

template <class T>
bool CTerrain::LoadDataFromFile(T* data, size_t elems, uint8*& f, int& nDataSize, EEndian eEndian, int* pSeek)
{
    StepDataCopy(data, f, elems, eEndian);
    nDataSize -= elems * sizeof(T);
    assert(nDataSize >= 0);
    return (nDataSize >= 0);
}

inline bool CTerrain::LoadDataFromFile_Seek(size_t elems, uint8*& f, int& nDataSize, EEndian eEndian)
{
    nDataSize -= elems;
    f += elems;
    assert(nDataSize >= 0);
    return true;
}

inline void CTerrain::LoadDataFromFile_FixAlignment(AZ::IO::HandleType& fileHandle, int& nDataSize)
{
    while (nDataSize & 3)
    {
        int nRes = GetPak()->FSeek(fileHandle, 1, SEEK_CUR);
        assert(nRes == 0);
        assert(nDataSize);
        nDataSize--;
    }
    assert(nDataSize >= 0);
}

inline void CTerrain::LoadDataFromFile_FixAlignment(uint8*& f, int& nDataSize)
{
    while (nDataSize & 3)
    {
        assert(*f == 222);
        f++;
        assert(nDataSize);
        nDataSize--;
    }
    assert(nDataSize >= 0);
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