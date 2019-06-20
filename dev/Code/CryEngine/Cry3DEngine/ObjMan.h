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

#ifndef CRYINCLUDE_CRY3DENGINE_OBJMAN_H
#define CRYINCLUDE_CRY3DENGINE_OBJMAN_H
#pragma once


#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>

#include "StatObj.h"
#include "../RenderDll/Common/Shadow_Renderer.h"
#include "terrain_sector.h"
#include "StlUtils.h"
#include "cbuffer.h"
#include "CZBufferCuller.h"
#include "PoolAllocator.h"
#include "CCullThread.h"
#include <StatObjBus.h>

#include <map>
#include <vector>

#define ENTITY_MAX_DIST_FACTOR 100
#define MAX_VALID_OBJECT_VOLUME (10000000000.f)
#define DEFAULT_CGF_NAME ("engineassets/objects/default.cgf")

struct IStatObj;
struct IIndoorBase;
struct IRenderNode;
struct ISystem;
struct IDecalRenderNode;
struct SCheckOcclusionJobData;
struct SCheckOcclusionOutput;
struct CVisArea;

class CVegetation;

class C3DEngine;
struct IMaterial;

#define SMC_EXTEND_FRUSTUM 8
#define SMC_SHADOW_FRUSTUM_TEST 16

#define OCCL_TEST_HEIGHT_MAP    1
#define OCCL_TEST_CBUFFER           2
#define OCCL_TEST_INDOOR_OCCLUDERS_ONLY     4
#define POOL_STATOBJ_ALLOCS

//! contains stat obj instance group properties (vegetation object properties)
struct StatInstGroup
    : public IStatInstGroup
{
    StatInstGroup()
    {
        pStatObj = nullptr;
    }

    IStatObj* GetStatObj()
    {
        IStatObj* p = pStatObj;
        return (IStatObj*)p;
    }
    const IStatObj* GetStatObj() const
    {
        const IStatObj* p = pStatObj;
        return (const IStatObj*)p;
    }

    void Update(struct CVars* pCVars, int nGeomDetailScreenRes);
    void GetMemoryUsage(ICrySizer* pSizer) const{}
    float GetAlignToTerrainAmount() const;
};

struct SExportedBrushMaterial
{
    int size;
    char material[64];
};

struct SRenderMeshInfoOutput
{
    SRenderMeshInfoOutput() { memset(this, 0, sizeof(*this)); }
    _smart_ptr<IRenderMesh> pMesh;
    _smart_ptr<IMaterial> pMat;
};

// Inplace object for IStreamable* to cache StreamableMemoryContentSize
struct SStreamAbleObject
{
    explicit SStreamAbleObject(IStreamable* pObj, bool bUpdateMemUsage = true)
        : m_pObj(pObj)
        , fCurImportance(-1000.f)
    {
        if (pObj && bUpdateMemUsage)
        {
            m_nStreamableContentMemoryUsage = pObj->GetStreamableContentMemoryUsage();
        }
        else
        {
            m_nStreamableContentMemoryUsage = 0;
        }
    }

    bool operator==(const SStreamAbleObject& rOther) const
    {
        return m_pObj == rOther.m_pObj;
    }

    int GetStreamableContentMemoryUsage() const { return m_nStreamableContentMemoryUsage; }
    IStreamable* GetStreamAbleObject() const { return m_pObj; }
    uint32 GetLastDrawMainFrameId() const
    {
        return m_pObj->GetLastDrawMainFrameId();
    }
    float fCurImportance;
private:
    IStreamable*m_pObj;
    int m_nStreamableContentMemoryUsage;
};

struct SObjManPrecacheCamera
{
    SObjManPrecacheCamera()
        : vPosition(ZERO)
        , vDirection(ZERO)
        , bbox(AABB::RESET)
        , fImportanceFactor(1.0f)
    {
    }

    Vec3 vPosition;
    Vec3 vDirection;
    AABB bbox;
    float fImportanceFactor;
};

struct SObjManPrecachePoint
{
    SObjManPrecachePoint()
        : nId(0)
    {
    }

    int nId;
    CTimeValue expireTime;
};

struct SObjManRenderDebugInfo
{
    SObjManRenderDebugInfo(IRenderNode* _pEnt, float _fEntDistance)
        : pEnt(_pEnt)
        , fEntDistance(_fEntDistance) {}

    IRenderNode* pEnt;
    float fEntDistance;
};

struct IObjManager
{
    virtual ~IObjManager() {}

    virtual void PreloadLevelObjects() = 0;
    virtual void UnloadObjects(bool bDeleteAll) = 0;
    virtual void UnloadVegetationModels(bool bDeleteAll) = 0;
    virtual void CheckTextureReadyFlag() = 0;
    virtual void FreeStatObj(IStatObj* pObj) = 0;
    virtual _smart_ptr<IStatObj> GetDefaultCGF() = 0;

    typedef std::vector< IDecalRenderNode* > DecalsToPrecreate;
    virtual DecalsToPrecreate& GetDecalsToPrecreate() = 0;
    virtual PodArray<SStreamAbleObject>& GetArrStreamableObjects() = 0;
    virtual PodArray<SObjManPrecacheCamera>& GetStreamPreCacheCameras() = 0;
    virtual PodArray<COctreeNode*>& GetArrStreamingNodeStack() = 0;
    virtual PodArray<SObjManPrecachePoint>& GetStreamPreCachePointDefs() = 0;

    typedef std::map<string, IStatObj*, stl::less_stricmp<string> > ObjectsMap;
    virtual ObjectsMap& GetNameToObjectMap() = 0;

    typedef std::set<IStatObj*> LoadedObjects;
    virtual LoadedObjects& GetLoadedObjects() = 0;

    virtual Vec3 GetSunColor() = 0;
    virtual void SetSunColor(const Vec3& color) = 0;

    virtual Vec3 GetSunAnimColor() = 0;
    virtual void SetSunAnimColor(const Vec3& color) = 0;

    virtual float GetSunAnimSpeed() = 0;
    virtual void SetSunAnimSpeed(float sunAnimSpeed) = 0;

    virtual AZ::u8 GetSunAnimPhase() = 0;
    virtual void SetSunAnimPhase(AZ::u8 sunAnimPhase) = 0;

    virtual AZ::u8 GetSunAnimIndex() = 0;
    virtual void SetSunAnimIndex(AZ::u8 sunAnimIndex) = 0;

    virtual float GetSSAOAmount() = 0;
    virtual void SetSSAOAmount(float amount) = 0;

    virtual float GetSSAOContrast() = 0;
    virtual void SetSSAOContrast(float amount) = 0;

    virtual SRainParams& GetRainParams() = 0;
    virtual SSnowParams& GetSnowParams() = 0;

    virtual bool IsCameraPrecacheOverridden() = 0;
    virtual void SetCameraPrecacheOverridden(bool state) = 0;

    virtual IStatObj* LoadNewCGF(IStatObj* pObject, int flagCloth, bool bUseStreaming, bool bForceBreakable, unsigned long nLoadingFlags, const char* normalizedFilename, const void* pData, int nDataSize, const char* originalFilename, const char* geomName, IStatObj::SSubObject** ppSubObject) = 0;
    virtual IStatObj* LoadFromCacheNoRef(IStatObj* pObject, bool bUseStreaming, unsigned long nLoadingFlags, const char* geomName, IStatObj::SSubObject** ppSubObject) = 0;

    virtual IStatObj* AllocateStatObj() = 0;
    virtual IStatObj* LoadStatObjUnsafeManualRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL) = 0;
    virtual _smart_ptr<IStatObj> LoadStatObjAutoRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL) = 0;
    virtual void GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount) = 0;
    virtual bool InternalDeleteObject(IStatObj* pObject) = 0;
    virtual void MakeShadowCastersList(CVisArea* pReceiverArea, const AABB& aabbReceiver, int dwAllowedTypes, int32 nRenderNodeFlags, Vec3 vLightPos, CDLight* pLight, ShadowMapFrustum* pFr, PodArray<struct SPlaneObject>* pShadowHull, const SRenderingPassInfo& passInfo) = 0;
    virtual int MakeStaticShadowCastersList(IRenderNode* pIgnoreNode, ShadowMapFrustum* pFrustum, int renderNodeExcludeFlags, int nMaxNodes, const SRenderingPassInfo& passInfo) = 0;
    virtual void MakeDepthCubemapRenderItemList(CVisArea* pReceiverArea, const AABB& cubemapAABB, int renderNodeFlags, PodArray<struct IShadowCaster*>* objectsList, const SRenderingPassInfo& passInfo) = 0;
    virtual void PrecacheStatObjMaterial(_smart_ptr<IMaterial> pMaterial, const float fEntDistance, IStatObj* pStatObj, bool bFullUpdate, bool bDrawNear) = 0;
    virtual void PrecacheCharacter(IRenderNode* pObj, const float fImportance, ICharacterInstance* pCharacter, _smart_ptr<IMaterial> pSlotMat, const Matrix34& matParent, const float fEntDistance, const float fScale, int nMaxDepth, bool bFullUpdate, bool bDrawNear, int nLod) = 0;
    virtual void PrecacheStatObj(IStatObj* pStatObj, int nLod, const Matrix34A& statObjMatrix, _smart_ptr<IMaterial> pMaterial, float fImportance, float fEntDistance, bool bFullUpdate, bool bHighPriority) = 0;

    virtual int GetLoadedObjectCount() = 0;

    virtual uint16 CheckCachedNearestCubeProbe(IRenderNode* pEnt) = 0;
    virtual int16 GetNearestCubeProbe(IVisArea* pVisArea, const AABB& objBox, bool bSpecular = true) = 0;

    virtual void RenderObject(IRenderNode* o, const AABB& objBox, float fEntDistance, EERType eERType, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;
    virtual void RenderVegetation(class CVegetation* pEnt, const AABB& objBox, float fEntDistance, SSectorTextureSet* pTerrainTexInfo, bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;
    virtual void RenderBrush(class CBrush* pEnt, const SSectorTextureSet* pTerrainTexInfo, const AABB& objBox, float fEntDistance, CVisArea* pVisArea, bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;
    virtual void RenderDecalAndRoad(IRenderNode* pEnt, const AABB& objBox, float fEntDistance, bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter) = 0;
    virtual void RenderObjectDebugInfo(IRenderNode* pEnt, float fEntDistance, const SRenderingPassInfo& passInfo) = 0;
    virtual void RenderAllObjectDebugInfo() = 0;
    virtual void RenderObjectDebugInfo_Impl(IRenderNode* pEnt, float fEntDistance) = 0;
    virtual void RemoveFromRenderAllObjectDebugInfo(IRenderNode* pEnt) = 0;

    virtual float GetXYRadius(int nType, int nSID = DEFAULT_SID) = 0;
    virtual bool GetStaticObjectBBox(int nType, Vec3& vBoxMin, Vec3& vBoxMax, int nSID = DEFAULT_SID) = 0;

    virtual IStatObj* GetStaticObjectByTypeID(int nTypeID, int nSID = DEFAULT_SID) = 0;
    virtual IStatObj* FindStaticObjectByFilename(const char* filename) = 0;

    //virtual float GetBendingRandomFactor() = 0;
    virtual float GetGSMMaxDistance() const = 0;
    virtual void SetGSMMaxDistance(float value) = 0;

    virtual int GetUpdateStreamingPrioriryRoundIdFast() = 0;
    virtual int GetUpdateStreamingPrioriryRoundId() = 0;
    virtual void IncrementUpdateStreamingPrioriryRoundIdFast(int amount) = 0;
    virtual void IncrementUpdateStreamingPrioriryRoundId(int amount) = 0;

    virtual NAsyncCull::CCullThread& GetCullThread() = 0;

    virtual void SetLockCGFResources(bool state) = 0;
    virtual bool IsLockCGFResources() = 0;

    virtual bool IsBoxOccluded(const AABB& objBox, float fDistance, OcclusionTestClient* const __restrict pOcclTestVars, bool bIndoorOccludersOnly, EOcclusionObjectType eOcclusionObjectType, const SRenderingPassInfo& passInfo) = 0;

    virtual void AddDecalToRenderer(float fDistance, _smart_ptr<IMaterial> pMat, const uint8 sortPrio, Vec3 right, Vec3 up, const UCol& ucResCol, const uint8 uBlendType, const Vec3& vAmbientColor, Vec3 vPos, const int nAfterWater, const SRenderingPassInfo& passInfo, CVegetation* pVegetation, const SRendItemSorter& rendItemSorter) = 0;

    virtual void RegisterForStreaming(IStreamable* pObj) = 0;
    virtual void UnregisterForStreaming(IStreamable* pObj) = 0;
    virtual void UpdateRenderNodeStreamingPriority(IRenderNode* pObj, float fEntDistance, float fImportanceFactor, bool bFullUpdate, const SRenderingPassInfo& passInfo, bool bHighPriority = false) = 0;

    virtual void GetMemoryUsage(class ICrySizer* pSizer) const = 0;
    virtual void GetBandwidthStats(float* fBandwidthRequested) = 0;

    virtual void ReregisterEntitiesInArea(Vec3 vBoxMin, Vec3 vBoxMax) = 0;
    virtual void UpdateObjectsStreamingPriority(bool bSyncLoad, const SRenderingPassInfo& passInfo) = 0;
    virtual void ProcessObjectsStreaming(const SRenderingPassInfo& passInfo) = 0;

    virtual void ProcessObjectsStreaming_Impl(bool bSyncLoad, const SRenderingPassInfo& passInfo) = 0;
    virtual void ProcessObjectsStreaming_Sort(bool bSyncLoad, const SRenderingPassInfo& passInfo) = 0;
    virtual void ProcessObjectsStreaming_Release() = 0;
    virtual void ProcessObjectsStreaming_InitLoad(bool bSyncLoad) = 0;
    virtual void ProcessObjectsStreaming_Finish() = 0;

#ifdef OBJMAN_STREAM_STATS
    virtual void ProcessObjectsStreaming_Stats(const SRenderingPassInfo& passInfo) = 0;
#endif

    // time counters
    virtual bool IsAfterWater(const Vec3& vPos, const SRenderingPassInfo& passInfo) = 0;
    virtual void GetObjectsStreamingStatus(I3DEngine::SObjectsStreamingStatus& outStatus) = 0;
    virtual void FreeNotUsedCGFs() = 0;
    virtual void MakeUnitCube() = 0;

    virtual bool CheckOcclusion_TestAABB(const AABB& rAABB, float fEntDistance) = 0;
    virtual bool CheckOcclusion_TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY) = 0;

    virtual void PushIntoCullQueue(const SCheckOcclusionJobData& rCheckOcclusionData) = 0;
    virtual void PopFromCullQueue(SCheckOcclusionJobData* pCheckOcclusionData) = 0;

    virtual void PushIntoCullOutputQueue(const SCheckOcclusionOutput& rCheckOcclusionOutput) = 0;
    virtual bool PopFromCullOutputQueue(SCheckOcclusionOutput* pCheckOcclusionOutput) = 0;

    virtual void BeginCulling() = 0;
    virtual void RemoveCullJobProducer() = 0;
    virtual void AddCullJobProducer() = 0;

#ifndef _RELEASE
    virtual void CoverageBufferDebugDraw() = 0;
#endif

    virtual bool LoadOcclusionMesh(const char* pFileName) = 0;

    virtual void ClearStatObjGarbage() = 0;
    virtual void CheckForGarbage(IStatObj* pObject) = 0;
    virtual void UnregisterForGarbage(IStatObj* pObject) = 0;

    virtual int GetObjectLOD(const IRenderNode* pObj, float fDistance) = 0;
    virtual bool RayStatObjIntersection(IStatObj* pStatObj, const Matrix34& objMat, _smart_ptr<IMaterial> pMat, Vec3 vStart, Vec3 vEnd, Vec3& vClosestHitPoint, float& fClosestHitDistance, bool bFastTest) = 0;
    virtual bool RayRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal, bool bFastTest, _smart_ptr<IMaterial> pMat) = 0;
    virtual bool SphereRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const float fRadius, _smart_ptr<IMaterial> pMat) = 0;
    virtual void FillTerrainTexInfo(IOctreeNode* pOcNode, float fEntDistance, struct SSectorTextureSet*& pTerrainTexInfo, const AABB& objBox) = 0;

    virtual uint8 GetDissolveRef(float fDist, float fMaxViewDist) = 0;
    virtual float GetLodDistDissolveRef(SLodDistDissolveTransitionState* pState, float curDist, int nNewLod, const SRenderingPassInfo& passInfo) = 0;

    virtual void CleanStreamingData() = 0;
    virtual IRenderMesh* GetRenderMeshBox() = 0;

    virtual void PrepareCullbufferAsync(const CCamera& rCamera) = 0;
    virtual void BeginOcclusionCulling(const SRenderingPassInfo& passInfo) = 0;
    virtual void EndOcclusionCulling() = 0;
    virtual void RenderBufferedRenderMeshes(const SRenderingPassInfo& passInfo) = 0;

    virtual PodArray<PodArray<StatInstGroup> >& GetListStaticTypes() = 0;
    virtual int IncrementNextPrecachePointId() = 0;
};

//////////////////////////////////////////////////////////////////////////
class CObjManager
    : public Cry3DEngineBase
    , public IObjManager
    , private StatInstGroupEventBus::Handler
{
public:
    enum
    {
        MaxPrecachePoints = 4,
    };

public:
    CObjManager();
    virtual ~CObjManager();

    void PreloadLevelObjects();
    void UnloadObjects(bool bDeleteAll);
    void UnloadVegetationModels(bool bDeleteAll);

    void CheckTextureReadyFlag();

    IStatObj* AllocateStatObj();
    void FreeStatObj(IStatObj* pObj);
    virtual _smart_ptr<IStatObj> GetDefaultCGF() { return m_pDefaultCGF; }

    virtual SRainParams& GetRainParams() override { return m_rainParams; }
    virtual SSnowParams& GetSnowParams() override { return m_snowParams;  }

    template <class T>
    static int GetItemId(std::vector<T*>* pArray, T* pItem, bool bAssertIfNotFound = true)
    {
        for (uint32 i = 0, end = pArray->size(); i < end; ++i)
        {
            if ((*pArray)[i] == pItem)
            {
                return i;
            }
        }
        return -1;
    }

    template <class T>
    static T* GetItemPtr(std::vector<T*>* pArray, int nId)
    {
        if (nId < 0)
        {
            return NULL;
        }

        assert(nId < (int)pArray->size());

        if (nId < (int)pArray->size())
        {
            return (*pArray)[nId];
        }
        else
        {
            return NULL;
        }
    }


    template <class T>
    static int GetItemId(std::vector<_smart_ptr<T> >* pArray, _smart_ptr<T> pItem, bool bAssertIfNotFound = true)
    {
        for (uint32 i = 0, end = pArray->size(); i < end; ++i)
        {
            if ((*pArray)[i] == pItem)
            {
                return i;
            }
        }
        return -1;
    }

    template <class T>
    static _smart_ptr<T> GetItemPtr(std::vector<_smart_ptr<T> >* pArray, int nId)
    {
        if (nId < 0)
        {
            return NULL;
        }

        assert(nId < (int)pArray->size());

        if (nId < (int)pArray->size())
        {
            return (*pArray)[nId];
        }
        else
        {
            return NULL;
        }
    }

    //! Loads a static object from a CGF file.  Does not increment the static object's reference counter.  The reference returned is not guaranteed to be valid unless run on the same thread running the garbage collection.  Best used for priming the cache
    virtual IStatObj* LoadStatObjUnsafeManualRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL) override;

    //! Loads a static object from a CGF file.  Increments the static object's reference counter.  This method is threadsafe.  Not suitable for preloading
    _smart_ptr<IStatObj> LoadStatObjAutoRef(const char* szFileName, const char* szGeomName = NULL, IStatObj::SSubObject** ppSubObject = NULL, bool bUseStreaming = true, unsigned long nLoadingFlags = 0, const void* m_pData = 0, int m_nDataSize = 0, const char* szBlockName = NULL);

private:

    template<typename T>
    T LoadStatObjInternal(const char* filename, const char* _szGeomName, IStatObj::SSubObject** ppSubObject, bool bUseStreaming, unsigned long nLoadingFlags, const void* pData, int nDataSize, const char* szBlockName);

    template <size_t SIZE_IN_CHARS>
    void NormalizeLevelName(const char* filename, char(&normalizedFilename)[SIZE_IN_CHARS]);

    void LoadDefaultCGF(const char* filename, unsigned long nLoadingFlags);
    virtual IStatObj* LoadNewCGF(IStatObj* pObject, int flagCloth, bool bUseStreaming, bool bForceBreakable, unsigned long nLoadingFlags, const char* normalizedFilename, const void* pData, int nDataSize, const char* originalFilename, const char* geomName, IStatObj::SSubObject** ppSubObject) override;

    virtual IStatObj* LoadFromCacheNoRef(IStatObj* pObject, bool bUseStreaming, unsigned long nLoadingFlags, const char* geomName, IStatObj::SSubObject** ppSubObject) override;

    PodArray<PodArray<StatInstGroup> > m_lstStaticTypes;

public:

    void GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount);

    // Deletes object.
    // Only should be called by Release function of IStatObj.
    virtual bool InternalDeleteObject(IStatObj* pObject) override;

    virtual PodArray<PodArray<StatInstGroup> >& GetListStaticTypes() override { return m_lstStaticTypes; }

    void MakeShadowCastersList(CVisArea* pReceiverArea, const AABB& aabbReceiver,
        int dwAllowedTypes, int32 nRenderNodeFlags, Vec3 vLightPos, CDLight* pLight, ShadowMapFrustum* pFr, PodArray<struct SPlaneObject>* pShadowHull, const SRenderingPassInfo& passInfo);

    int MakeStaticShadowCastersList(IRenderNode* pIgnoreNode, ShadowMapFrustum* pFrustum, int renderNodeExcludeFlags, int nMaxNodes, const SRenderingPassInfo& passInfo);

    void MakeDepthCubemapRenderItemList(CVisArea* pReceiverArea, const AABB& cubemapAABB, int renderNodeFlags, PodArray<struct IShadowCaster*>* objectsList, const SRenderingPassInfo& passInfo);

    // decal pre-caching
    DecalsToPrecreate m_decalsToPrecreate;

    void PrecacheStatObjMaterial(_smart_ptr<IMaterial> pMaterial, const float fEntDistance, IStatObj* pStatObj, bool bFullUpdate, bool bDrawNear);
    void PrecacheCharacter(IRenderNode* pObj, const float fImportance, ICharacterInstance* pCharacter, _smart_ptr<IMaterial> pSlotMat,
        const Matrix34& matParent, const float fEntDistance, const float fScale, int nMaxDepth, bool bFullUpdate, bool bDrawNear, int nLod);
    virtual DecalsToPrecreate& GetDecalsToPrecreate() override { return m_decalsToPrecreate; }

    void PrecacheStatObj(IStatObj* pStatObj, int nLod, const Matrix34A& statObjMatrix, _smart_ptr<IMaterial> pMaterial, float fImportance, float fEntDistance, bool bFullUpdate, bool bHighPriority);

    virtual PodArray<SStreamAbleObject>& GetArrStreamableObjects() override { return m_arrStreamableObjects; }
    virtual PodArray<SObjManPrecacheCamera>& GetStreamPreCacheCameras() override { return m_vStreamPreCacheCameras; }

    virtual Vec3 GetSunColor() override { return m_vSunColor; }
    virtual void SetSunColor(const Vec3& color) override { m_vSunColor = color; }

    Vec3 GetSunAnimColor() override { return m_sunAnimColor; }
    void SetSunAnimColor(const Vec3& color) override { m_sunAnimColor = color; }

    float GetSunAnimSpeed() override { return m_sunAnimSpeed; }
    void SetSunAnimSpeed(float sunAnimSpeed) override { m_sunAnimSpeed = sunAnimSpeed; }

    AZ::u8 GetSunAnimPhase() override { return m_sunAnimPhase; }
    void SetSunAnimPhase(AZ::u8 sunAnimPhase) override { m_sunAnimPhase = sunAnimPhase; }

    AZ::u8 GetSunAnimIndex() override { return m_sunAnimIndex; }
    void SetSunAnimIndex(AZ::u8 sunAnimIndex) override { m_sunAnimIndex = sunAnimIndex; }

    virtual float GetSSAOAmount() override { return m_fSSAOAmount;  }
    virtual void SetSSAOAmount(float amount) override { m_fSSAOAmount = amount; }

    virtual float GetSSAOContrast() override { return m_fSSAOContrast; }
    virtual void SetSSAOContrast(float amount) override { m_fSSAOContrast = amount; }

    virtual bool IsCameraPrecacheOverridden() override { return m_bCameraPrecacheOverridden; }
    virtual void SetCameraPrecacheOverridden(bool state) override { m_bCameraPrecacheOverridden = state; }

    virtual ObjectsMap& GetNameToObjectMap() override { return m_nameToObjectMap; }
    virtual LoadedObjects& GetLoadedObjects() override { return m_lstLoadedObjects; }

    //////////////////////////////////////////////////////////////////////////

    ObjectsMap m_nameToObjectMap;
    LoadedObjects m_lstLoadedObjects;

    /// Thread-safety for async requests to CreateInstance.
    // Always take this lock before m_garbageMutex if taking both
    AZStd::recursive_mutex m_loadMutex;

#ifdef WIN64
#pragma warning( push )                                 //AMD Port
#pragma warning( disable : 4267 )
#endif

public:
    int GetLoadedObjectCount() { return m_lstLoadedObjects.size(); }

#ifdef WIN64
#pragma warning( pop )                                  //AMD Port
#endif

    uint16 CheckCachedNearestCubeProbe(IRenderNode* pEnt)
    {
        if (pEnt->m_pRNTmpData)
        {
            CRNTmpData::SRNUserData& pUserDataRN = pEnt->m_pRNTmpData->userData;

            const uint16 nCacheClearThreshold = 32;
            ++pUserDataRN.nCubeMapIdCacheClearCounter;
            pUserDataRN.nCubeMapIdCacheClearCounter &= (nCacheClearThreshold - 1);

            if (pUserDataRN.nCubeMapId && pUserDataRN.nCubeMapIdCacheClearCounter)
            {
                return pUserDataRN.nCubeMapId;
            }
        }

        // cache miss
        return 0;
    }

    int16 GetNearestCubeProbe(IVisArea* pVisArea, const AABB& objBox, bool bSpecular = true);

    void RenderObject(IRenderNode* o,
        const AABB& objBox,
        float fEntDistance,
        EERType eERType,
        const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);

    void RenderVegetation(class CVegetation* pEnt,
            const AABB& objBox, float fEntDistance,
            SSectorTextureSet* pTerrainTexInfo, bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);
    void RenderBrush(class CBrush* pEnt,
            const SSectorTextureSet* pTerrainTexInfo,
            const AABB& objBox, float fEntDistance,
            CVisArea* pVisArea, bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);
    void RenderDecalAndRoad(IRenderNode* pEnt,
        const AABB& objBox, float fEntDistance,
        bool nCheckOcclusion, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter);

    void RenderObjectDebugInfo(IRenderNode* pEnt, float fEntDistance, const SRenderingPassInfo& passInfo);
    void RenderAllObjectDebugInfo();
    void RenderObjectDebugInfo_Impl(IRenderNode* pEnt, float fEntDistance);
    void RemoveFromRenderAllObjectDebugInfo(IRenderNode* pEnt);

    float GetXYRadius(int nType, int nSID = GetDefSID());
    bool GetStaticObjectBBox(int nType, Vec3& vBoxMin, Vec3& vBoxMax, int nSID = GetDefSID());

    IStatObj* GetStaticObjectByTypeID(int nTypeID, int nSID = GetDefSID());
    IStatObj* FindStaticObjectByFilename(const char* filename);

    //float GetBendingRandomFactor() override;
    int GetUpdateStreamingPrioriryRoundIdFast() override { return m_nUpdateStreamingPrioriryRoundIdFast; }
    int GetUpdateStreamingPrioriryRoundId() override { return m_nUpdateStreamingPrioriryRoundId; };
    virtual void IncrementUpdateStreamingPrioriryRoundIdFast(int amount) override { m_nUpdateStreamingPrioriryRoundIdFast += amount; }
    virtual void IncrementUpdateStreamingPrioriryRoundId(int amount) override { m_nUpdateStreamingPrioriryRoundId += amount; }

    virtual void SetLockCGFResources(bool value) override { m_bLockCGFResources = value; }
    virtual bool IsLockCGFResources() override { return m_bLockCGFResources != 0; }


    bool IsBoxOccluded(const AABB& objBox,
        float fDistance,
        OcclusionTestClient* const __restrict pOcclTestVars,
        bool bIndoorOccludersOnly,
        EOcclusionObjectType eOcclusionObjectType,
        const SRenderingPassInfo& passInfo);

    void AddDecalToRenderer(float fDistance,
        _smart_ptr<IMaterial> pMat,
        const uint8 sortPrio,
        Vec3 right,
        Vec3 up,
        const UCol& ucResCol,
        const uint8 uBlendType,
        const Vec3& vAmbientColor,
        Vec3 vPos,
        const int nAfterWater,
        const SRenderingPassInfo& passInfo,
        CVegetation* pVegetation,
        const SRendItemSorter& rendItemSorter);

    //////////////////////////////////////////////////////////////////////////

    void RegisterForStreaming(IStreamable* pObj);
    void UnregisterForStreaming(IStreamable* pObj);
    void UpdateRenderNodeStreamingPriority(IRenderNode* pObj, float fEntDistance, float fImportanceFactor, bool bFullUpdate, const SRenderingPassInfo& passInfo, bool bHighPriority = false);

    void GetMemoryUsage(class ICrySizer* pSizer) const;
    void GetBandwidthStats(float* fBandwidthRequested);

    void ReregisterEntitiesInArea(Vec3 vBoxMin, Vec3 vBoxMax);
    void UpdateObjectsStreamingPriority(bool bSyncLoad, const SRenderingPassInfo& passInfo);
    void ProcessObjectsStreaming(const SRenderingPassInfo& passInfo);

    // implementation parts of ProcessObjectsStreaming
    void ProcessObjectsStreaming_Impl(bool bSyncLoad, const SRenderingPassInfo& passInfo);
    void ProcessObjectsStreaming_Sort(bool bSyncLoad, const SRenderingPassInfo& passInfo);
    void ProcessObjectsStreaming_Release();
    void ProcessObjectsStreaming_InitLoad(bool bSyncLoad);
    void ProcessObjectsStreaming_Finish();

#ifdef OBJMAN_STREAM_STATS
    void ProcessObjectsStreaming_Stats(const SRenderingPassInfo& passInfo);
#endif

    // time counters

    virtual bool IsAfterWater(const Vec3& vPos, const SRenderingPassInfo& passInfo) override;

    void GetObjectsStreamingStatus(I3DEngine::SObjectsStreamingStatus& outStatus);

    void FreeNotUsedCGFs();

    void MakeUnitCube();

    //////////////////////////////////////////////////////////////////////////
    // CheckOcclusion functionality
    bool CheckOcclusion_TestAABB(const AABB& rAABB, float fEntDistance);
    bool CheckOcclusion_TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY);

    void PushIntoCullQueue(const SCheckOcclusionJobData& rCheckOcclusionData);
    void PopFromCullQueue(SCheckOcclusionJobData* pCheckOcclusionData);

    void PushIntoCullOutputQueue(const SCheckOcclusionOutput& rCheckOcclusionOutput);
    bool PopFromCullOutputQueue(SCheckOcclusionOutput* pCheckOcclusionOutput);

    void BeginCulling();
    void RemoveCullJobProducer();
    void AddCullJobProducer();
    virtual NAsyncCull::CCullThread& GetCullThread() override { return m_CullThread;  };

#ifndef _RELEASE
    void CoverageBufferDebugDraw();
#endif

    bool LoadOcclusionMesh(const char* pFileName);

    //////////////////////////////////////////////////////////////////////////
    // Garbage collection for parent stat objects.
    // Returns number of deleted objects
    void ClearStatObjGarbage();
    void CheckForGarbage(IStatObj* pObject);
    void UnregisterForGarbage(IStatObj* pObject);

    int GetObjectLOD(const IRenderNode* pObj, float fDistance);
    bool RayStatObjIntersection(IStatObj* pStatObj, const Matrix34& objMat, _smart_ptr<IMaterial> pMat,
        Vec3 vStart, Vec3 vEnd, Vec3& vClosestHitPoint, float& fClosestHitDistance, bool bFastTest);
    bool RayRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal, bool bFastTest, _smart_ptr<IMaterial> pMat);
    bool SphereRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const float fRadius, _smart_ptr<IMaterial> pMat);
    void FillTerrainTexInfo(IOctreeNode* pOcNode, float fEntDistance, struct SSectorTextureSet*& pTerrainTexInfo, const AABB& objBox);
    PodArray<CVisArea*> m_tmpAreas0, m_tmpAreas1;

    uint8 GetDissolveRef(float fDist, float fMaxViewDist);
    float GetLodDistDissolveRef(SLodDistDissolveTransitionState* pState, float curDist, int nNewLod, const SRenderingPassInfo& passInfo);

    virtual PodArray<COctreeNode*>& GetArrStreamingNodeStack() override { return m_arrStreamingNodeStack; }
    virtual PodArray<SObjManPrecachePoint>& GetStreamPreCachePointDefs() override { return m_vStreamPreCachePointDefs; }

    void CleanStreamingData();
    IRenderMesh* GetRenderMeshBox();

    void PrepareCullbufferAsync(const CCamera& rCamera);
    void BeginOcclusionCulling(const SRenderingPassInfo& passInfo);
    void EndOcclusionCulling();
    void RenderBufferedRenderMeshes(const SRenderingPassInfo& passInfo);

    virtual float GetGSMMaxDistance() const override { return m_fGSMMaxDistance; }
    virtual void SetGSMMaxDistance(float value) override { m_fGSMMaxDistance = value; }

    virtual int IncrementNextPrecachePointId() override { return m_nNextPrecachePointId++; }
private:
    void PrecacheCharacterCollect(IRenderNode* pObj, const float fImportance, ICharacterInstance* pCharacter, _smart_ptr<IMaterial> pSlotMat,
        const Matrix34& matParent, const float fEntDistance, const float fScale, int nMaxDepth, bool bFullUpdate, bool bDrawNear, int nLod,
        const int nRoundId, std::vector<std::pair<_smart_ptr<IMaterial>, float> >& collectedMaterials);

    std::vector<std::pair<_smart_ptr<IMaterial>, float> > m_collectedMaterials;

public:
    //////////////////////////////////////////////////////////////////////////
    // Public Member variables (need to be cleaned).
    //////////////////////////////////////////////////////////////////////////

    static int m_nUpdateStreamingPrioriryRoundId;
    static int m_nUpdateStreamingPrioriryRoundIdFast;
    static int s_nLastStreamingMemoryUsage;                 //For streaming tools in editor

    Vec3    m_vSunColor;    //Similar to CDLight's m_BaseColor
    Vec3    m_sunAnimColor; //Similar to CDLight's m_Color
    float   m_sunAnimSpeed;
    AZ::u8  m_sunAnimPhase;
    AZ::u8  m_sunAnimIndex;

    float                   m_fILMul;
    float                   m_fSSAOAmount;
    float                   m_fSSAOContrast;
    SRainParams     m_rainParams;
    SSnowParams     m_snowParams;

    int           m_bLockCGFResources;

    float m_fGSMMaxDistance;

public:
    //////////////////////////////////////////////////////////////////////////
    // Private Member variables.
    //////////////////////////////////////////////////////////////////////////
    PodArray<IStreamable*>  m_arrStreamableToRelease;
    PodArray<IStreamable*>  m_arrStreamableToLoad;
    PodArray<IStreamable*>  m_arrStreamableToDelete;
    bool m_bNeedProcessObjectsStreaming_Finish;

#ifdef SUPP_HWOBJ_OCCL
    IShader* m_pShaderOcclusionQuery;
#endif

    //  bool LoadStaticObjectsFromXML(XmlNodeRef xmlVegetation);
    _smart_ptr<IStatObj>        m_pDefaultCGF;
    _smart_ptr<IRenderMesh> m_pRMBox;

    //////////////////////////////////////////////////////////////////////////
    std::vector<_smart_ptr<IStatObj> > m_lockedObjects;

    //////////////////////////////////////////////////////////////////////////

    bool m_bGarbageCollectionEnabled;

    PodArray<SStreamAbleObject> m_arrStreamableObjects;
    PodArray<COctreeNode*> m_arrStreamingNodeStack;
    PodArray<SObjManPrecachePoint> m_vStreamPreCachePointDefs;
    PodArray<SObjManPrecacheCamera> m_vStreamPreCacheCameras;
    int m_nNextPrecachePointId;
    bool m_bCameraPrecacheOverridden;

    PodArray<CTerrainNode*> m_lstTmpCastingNodes;

#ifdef POOL_STATOBJ_ALLOCS
    stl::PoolAllocator<sizeof(CStatObj), stl::PSyncMultiThread, alignof(CStatObj)>* m_statObjPool;
#endif

    CThreadSafeRendererContainer<SObjManRenderDebugInfo> m_arrRenderDebugInfo;

    NAsyncCull::CCullThread m_CullThread;
    CryMT::SingleProducerSingleConsumerQueue<SCheckOcclusionJobData> m_CheckOcclusionQueue;
    CryMT::N_ProducerSingleConsumerQueue<SCheckOcclusionOutput> m_CheckOcclusionOutputQueue;

private:

    // Always take this lock after m_loadLock if taking both
    AZStd::recursive_mutex m_garbageMutex;
    AZStd::vector<IStatObj*> m_checkForGarbage;

private:
    // StatInstGroupEventBus
    AZStd::unordered_set<StatInstGroupId> m_usedIds;
    StatInstGroupId GenerateStatInstGroupId() override;
    void ReleaseStatInstGroupId(StatInstGroupId statInstGroupId) override;
    void ReleaseStatInstGroupIdSet(const AZStd::unordered_set<StatInstGroupId>& statInstGroupIdSet) override;
    void ReserveStatInstGroupIdRange(StatInstGroupId from, StatInstGroupId to) override;
};


#endif // CRYINCLUDE_CRY3DENGINE_OBJMAN_H
