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
//  Contains:
//  high-level loading of characters

#ifndef CRYINCLUDE_CRYANIMATION_CHARACTERMANAGER_H
#define CRYINCLUDE_CRYANIMATION_CHARACTERMANAGER_H
#pragma once

#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <ICryAnimation.h>
#include "ParamLoader.h"
#include "AttachmentVCloth.h"
#include "ManualResetEvent.h"
#include "UniqueManualEvent.h"

class CSkin;                        //default skinning
class CAttachmentSKIN;  //skin-instance
class CAttachmentVCLOTH;    //cloth-instance
class CDefaultSkeleton; //default skeleton
class CCharInstance;        //skel-instance
class CAttachmentManager;       //skel-instance
class CClothManager;

class CFacialAnimation;
struct IAnimationSet;

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
class CPoseModifierSetup;
DECLARE_SMART_POINTERS(CPoseModifierSetup);
#endif

extern float g_YLine;

struct DebugInstances
{
    _smart_ptr<ICharacterInstance> m_pInst;
    QuatTS m_GridLocation;
    f32 m_fExtrapolation;
    ColorF m_AmbientColor;
};


struct CDefaultSkeletonReferences
{
    CDefaultSkeleton* m_pDefaultSkeleton;
    DynArray<CCharInstance*> m_RefByInstances;
    CDefaultSkeletonReferences()
    {
        m_pDefaultSkeleton = 0;
        m_RefByInstances.reserve(0x10);
    }
    void GetMemoryUsage(ICrySizer* pSizer) const {}
};
struct CDefaultSkinningReferences
{
    CSkin* m_pDefaultSkinning;
    DynArray<CAttachmentSKIN*> m_RefByInstances;
    DynArray<CAttachmentVCLOTH*> m_RefByInstancesVCloth;
    CDefaultSkinningReferences()
    {
        m_pDefaultSkinning = 0;
        m_RefByInstances.reserve(0x10);
    }
    void GetMemoryUsage(ICrySizer* pSizer) const {}
};

//! Allows returning a CDefaultSkinningReferences pointer in a thread-safe way by holding onto a lock
struct ScopedDefaultSkinningReferences
{
public:

    ScopedDefaultSkinningReferences(CDefaultSkinningReferences* defaultSkinningReferences, AZStd::unique_lock<AZStd::recursive_mutex>&& lock)
        : m_defaultSkinningReferences(defaultSkinningReferences),
        m_lock(std::move(lock))
    {}

    CDefaultSkinningReferences* operator->() const
    {
        return m_defaultSkinningReferences;
    }

    bool operator ==(AZStd::nullptr_t nullPtr) const
    {
        return m_defaultSkinningReferences == nullptr;
    }

    bool operator !=(AZStd::nullptr_t nullPtr) const
    {
        return m_defaultSkinningReferences != nullptr;
    }

    explicit operator bool() const
    {
        return m_defaultSkinningReferences != nullptr;
    }

private:
    CDefaultSkinningReferences* m_defaultSkinningReferences;
    AZStd::unique_lock<AZStd::recursive_mutex> m_lock;
};


struct CharacterAttachment
{
    CharacterAttachment()
        : m_relRotation(false)
        , m_relPosition(false)
        , m_RelativeDefault(IDENTITY)
        , m_AbsoluteDefault(IDENTITY)
        , m_Type(0xDeadBeef)
        , m_AttFlags(0)
        , m_ProxyParams(0, 0, 0, 0)
        , m_ProxyPurpose(0)
    {
        memset(&m_AttPhysInfo, 0, sizeof(m_AttPhysInfo));
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_strAttachmentName);
        pSizer->AddObject(m_strJointName);
        pSizer->AddObject(m_strBindingPath);
        pSizer->AddObject(m_pMaterial);
        pSizer->AddObject(m_parrMaterial);
        pSizer->AddObject(m_pStaticObject);
    }

    string m_strAttachmentName;
    uint32 m_Type;
    uint32 m_AttFlags;
    string m_strJointName;
    bool m_relRotation;
    bool m_relPosition;
    QuatT  m_RelativeDefault;
    QuatT  m_AbsoluteDefault;
    Vec4   m_ProxyParams;
    uint32 m_ProxyPurpose;

    SimulationParams ap;

    string m_strRowJointName;
    RowSimulationParams rowap;

    SVClothParams clothParams;

    string m_strBindingPath;
    string m_strSimBindingPath;
    _smart_ptr<IMaterial> m_pMaterial; //this is the shared material for all LODs
    _smart_ptr<IMaterial> m_parrMaterial[g_nMaxGeomLodLevels]; //some LODs can have an individual material
    _smart_ptr<IStatObj> m_pStaticObject;

    CryBonePhysics m_AttPhysInfo[2];
};

struct CharacterDefinition
{
    virtual ~CharacterDefinition(){}
    CharacterDefinition()
    {
        m_nRefCounter = 0;
        m_nKeepInMemory = 0;
        m_nKeepModelsInMemory = -1;
    }

    virtual void AddRef()
    {
        ++m_nRefCounter;
    }

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        pSizer->AddObject(m_strBaseModelFilePath);
        pSizer->AddObject(m_pBaseModelMaterial);
        pSizer->AddObject(m_arrAttachments);
    }

    AZStd::string m_strFilePath;
    string m_strBaseModelFilePath;
    _smart_ptr<IMaterial> m_pBaseModelMaterial;
    int m_nKeepModelsInMemory;  // Reference count.
    AZStd::atomic<int> m_nRefCounter;  // Reference count.
    int m_nKeepInMemory;
    DynArray<CharacterAttachment> m_arrAttachments;

#ifdef ENABLE_RUNTIME_POSE_MODIFIERS
    CPoseModifierSetupPtr m_pPoseModifierSetup;
#endif
};

//////////////////////////////////////////////////////////////////////
/// This class contains a list of character bodies and list of character instances.
/// On attempt to create same character second time only new instance will be created.
/// Only this class can create actual characters.
class CharacterManager
    : public ICharacterManager
    , public AzFramework::LegacyAssetEventBus::MultiHandler
{
public:
    static bool s_bPaused;
    static uint32 s_renderFrameIdLocal;
    static uint32 GetRendererThreadId();
    static uint32 GetRendererMainThreadId();


    friend class CAnimationManager;
    friend class CAttachmentManager;
    friend class CAttachmentSKIN;
    friend class CAttachmentVCLOTH;
    friend class CClothPiece;
    friend class CCharInstance;
    friend class CDefaultSkeleton;
    friend class CSkin;
    friend class CParamLoader;
#if BLENDSPACE_VISUALIZATION
    friend class CSkeletonAnim;
    friend struct SParametricSamplerInternal;
#endif
    static const char* c_AnimationEditorFileLocation;

    CharacterManager ();
    ~CharacterManager ();
    virtual void Release(); // Deletes itself

    virtual void PostInit();



    virtual void SyncAllAnimations();
    virtual void AddAnimationToSyncQueue(ICharacterInstance* pCharInstance);
    virtual void RemoveAnimationToSyncQueue(ICharacterInstance* pCharInstance);
    void ClearPoseModifiersFromSynchQueue();
    void StopAnimationsOnAllInstances();

    void UpdateRendererFrame();

    // should be called every frame
    void Update(bool bPause);
    void UpdateStreaming(int nFullUpdateRoundId, int nFastUpdateRoundId);
    void DatabaseUnloading();

    CAnimationSet* GetAnimationSetUsedInCharEdit();

    void DummyUpdate(); // can be called instead of Update() for UI purposes (such as in preview viewports, etc).




    //////////////////////////////////////////////////////////////////////////
    IFacialAnimation* GetIFacialAnimation();
    const IFacialAnimation* GetIFacialAnimation() const;

    IChrParamsParser* GetIChrParamParser(){ return &m_ParamLoader; };

    IAnimEvents* GetIAnimEvents() { return &g_AnimationManager; };
    const IAnimEvents* GetIAnimEvents() const { return &g_AnimationManager; };

    CAnimationManager& GetAnimationManager() { return m_AnimationManager; };
    const CAnimationManager& GetAnimationManager() const { return m_AnimationManager; };

    CParamLoader& GetParamLoader() { return m_ParamLoader; };

    CFacialAnimation* GetFacialAnimation() {return m_pFacialAnimation; }
    const CFacialAnimation* GetFacialAnimation() const {return m_pFacialAnimation; }

    //a list with model-names that use "ForceSkeletonUpdates"
    std::vector<string> m_arrSkeletonUpdates;
    std::vector<uint32> m_arrAnimPlaying;
    std::vector<uint32> m_arrForceSkeletonUpdates;
    std::vector<uint32> m_arrVisible;
    uint32 m_nUpdateCounter;
    uint32 m_AllowStartOfAnimation;
    uint32 g_SkeletonUpdates;
    uint32 g_AnimationUpdates;

    IAnimationStreamingListener* m_pStreamingListener;





    //methods to handle animation assets
    void SetAnimMemoryTracker(const SAnimMemoryTracker& amt)
    {
        g_AnimationManager.m_AnimMemoryTracker.m_nMemTracker        = amt.m_nMemTracker;
        g_AnimationManager.m_AnimMemoryTracker.m_nAnimsCurrent  = amt.m_nAnimsCurrent;
        g_AnimationManager.m_AnimMemoryTracker.m_nAnimsMax          = amt.m_nAnimsMax;
        g_AnimationManager.m_AnimMemoryTracker.m_nAnimsAdd          = amt.m_nAnimsAdd;
        g_AnimationManager.m_AnimMemoryTracker.m_nAnimsCounter  = amt.m_nAnimsCounter;
    }
    SAnimMemoryTracker GetAnimMemoryTracker() const
    {
        return g_AnimationManager.m_AnimMemoryTracker;
    }

    bool DBA_PreLoad(const char* filepath, ICharacterManager::EStreamingDBAPriority priority);
    bool DBA_LockStatus(const char* filepath, uint32 status, ICharacterManager::EStreamingDBAPriority priority);
    bool DBA_Unload(const char* filepath);
    bool DBA_Unload_All();

    virtual bool CAF_AddRef(uint32 filePathCRC);
    virtual bool CAF_IsLoaded(uint32 filePathCRC) const;
    virtual bool CAF_Release(uint32 filePathCRC);
    virtual bool CAF_LoadSynchronously(uint32 filePathCRC);
    virtual bool LMG_LoadSynchronously(uint32 filePathCRC, const IAnimationSet* pAnimationSet);

    virtual bool CAF_AddRefByGlobalId(int globalID);
    virtual bool CAF_ReleaseByGlobalId(int globalID);

    EReloadCAFResult ReloadCAF(const char* szFilePathCAF)
    {
        return g_AnimationManager.ReloadCAF(szFilePathCAF);
    };
    int ReloadLMG(const char* szFilePathLMG)
    {
        return g_AnimationManager.ReloadLMG(szFilePathLMG);
    };



    CDefaultSkeleton* GetModelByAimPoseID(uint32 nGlobalIDAimPose);
    const char* GetDBAFilePathByGlobalID(int32 globalID) const;
    virtual void SetStreamingListener(IAnimationStreamingListener* pListener) { m_pStreamingListener = pListener; };

    // light profiler functions
    virtual void AddFrameTicks(uint64 nTicks) { m_nFrameTicks += nTicks; }
    virtual void AddFrameSyncTicks(uint64 nTicks) { m_nFrameSyncTicks += nTicks; }
    virtual void ResetFrameTicks() { m_nFrameTicks = 0; m_nFrameSyncTicks = 0; }
    virtual uint64 NumFrameTicks() const { return m_nFrameTicks; }
    virtual uint64 NumFrameSyncTicks() const { return m_nFrameSyncTicks; }
    virtual uint32 NumCharacters() const { return m_nActiveCharactersLastFrame; }

    void UpdateDatabaseUnloadTimeStamp();
    uint32 GetDatabaseUnloadTimeDelta() const;

    ICharacterInstance* LoadCharacterDefinition(const AZStd::string& pathname, uint32 nLoadingFlags = 0);

    UniqueManualEvent SafeGetOrLoadCDF(const AZStd::string& pathname, CharacterDefinition*& characterDefinition);

    CharacterDefinition* LoadCDF(const char* pathname);
    CharacterDefinition* LoadCDFFromXML(XmlNodeRef root, const char* pathname);
    void ReleaseCDF(const char* pathname);
    uint32 GetCDFId(const AZStd::string& pathname);
    CharacterDefinition* GetOrLoadCDF(const AZStd::string& pathname, bool addRef = false);
    bool StreamKeepCDFResident(const char* szFilePath, int nLod, int nRefAdj, bool bUrgent);

    //! Return value will hold a lock on m_cacheSkinMutex until it goes out of scope
    ScopedDefaultSkinningReferences GetDefaultSkinningReferences(CSkin* pDefaultSkinning);

    // override from AssetCatalogEventBus::Handler
    void OnFileChanged(AZStd::string assetPath) override;
    void OnFileRemoved(AZStd::string assetPath) override;

    void MarkForGC(CDefaultSkeleton* pSkeleton);
    void MarkForGC(CSkin* pSkin);
    void UnmarkForGC(CDefaultSkeleton* pSkeleton);
    void UnmarkForGC(CSkin* pSkin);
    void RunGarbageCollection();
private:

    template<typename T, void(CharacterManager::* TUnregisterFunc)(T*)>
    void RunGarbageSet(AZStd::unordered_set<T*>& pendingGarbageList, AZStd::recursive_mutex& cacheMutex);

    uint32 m_StartGAH_Iterator;
    void LoadAnimationImageFile(const char* filenameCAF, const char* filenameAIM);
    bool LoadAnimationImageFileCAF(const char* filenameCAF, IChunkFile* pChunkFile);
    bool LoadAnimationImageFileAIM(const char* filenameAIM, IChunkFile* pChunkFile);
    uint32 IsInitializedByIMG() {   return m_InitializedByIMG;  };
    uint32 m_InitializedByIMG;


    void DumpAssetStatistics();
    f32 GetAverageFrameTime(f32 sec, f32 FrameTime, f32 TimeScale, f32 LastAverageFrameTime);




    //methods to manage instances
    virtual ICharacterInstance* CreateInstance(const char* szFilePath, uint32 nLoadingFlags = 0);
    virtual void CreateInstanceAsync(const LoadInstanceAsyncResult& callback, const char* szFileName, uint32 nLoadingFlags = 0);
    ICharacterInstance* CreateCGAInstance(const char* szFilePath, uint32 nLoadingFlags = 0);
    ICharacterInstance* CreateSKELInstance(const char* szFilePath, uint32 nLoadingFlags);
    void RegisterInstanceSkel(CDefaultSkeleton* pDefaultSkeleton, CCharInstance* pInstance);
    void UnregisterInstanceSkel(CDefaultSkeleton* pDefaultSkeleton, CCharInstance* pInstance);
    void RegisterInstanceSkin(CSkin* pDefaultSkinning, CAttachmentSKIN* pInstance);
    void UnregisterInstanceSkin(CSkin* pDefaultSkinning, CAttachmentSKIN* pInstance);
    void RegisterInstanceVCloth(CSkin* pDefaultSkinning, CAttachmentVCLOTH* pInstance);
    void UnregisterInstanceVCloth(CSkin* pDefaultSkinning, CAttachmentVCLOTH* pInstance);
    virtual uint32 GetNumInstancesPerModel(const IDefaultSkeleton& rIDefaultSkeleton) const;
    virtual ICharacterInstance* GetICharInstanceFromModel(const IDefaultSkeleton& rIDefaultSkeleton, uint32 num) const;
    void GetCharacterInstancesSize(class ICrySizer* pSizer) const;

    //methods to manage skels and skins
    virtual IDefaultSkeleton* LoadModelSKELUnsafeManualRef(const char* szFilePath, uint32 nLoadingFlags);
    virtual _smart_ptr<ISkin> LoadModelSKINAutoRef(const char* szFilePath, uint32 nLoadingFlags);
    virtual ISkin* LoadModelSKINUnsafeManualRef(const char* szFilePath, uint32 nLoadingFlags);
    CDefaultSkeleton* FetchModelSKELUnsafeManualRef(const char* szFilePath, uint32 nLoadingFlags);
    _smart_ptr<CDefaultSkeleton> FetchModelSKELAutoRef(const char* szFilePath, uint32 nLoadingFlags);
    _smart_ptr<CDefaultSkeleton> FetchModelSKELForGCAAutoRef(const char* szFilePath, uint32 nLoadingFlags);
    CDefaultSkeleton* FetchModelSKELForGCAUnsafeManualRef(const char* szFilePath, uint32 nLoadingFlags);
    CSkin* FetchModelSKINUnsafeManualRef(const char* szFilePath, uint32 nLoadingFlags);
    _smart_ptr<CSkin> FetchModelSKINAutoRef(const char* szFilePath, uint32 nLoadingFlags);
    void PreloadModelsCDF();
    void PreloadModelsCHR();
    void PreloadModelsCGA();
    void RegisterModelSKEL(CDefaultSkeleton* pModelSKEL, uint32 nLoadingFlags);
    void RegisterModelSKIN(CSkin* pModelSKIN, uint32 nLoadingFlags);
    void UnregisterModelSKEL(CDefaultSkeleton* pModelSKEL);
    void UnregisterModelSKIN(CSkin* pModelSKIN);
    void SkelExtension(CCharInstance* pCharInstance, const char* pFilepathSKEL, const CharacterDefinition& characterDef, const uint32 nLoadingFlags);
    uint32 CompatibilityTest(CDefaultSkeleton* pDefaultSkeleton, CSkin* pCSkin);
    CDefaultSkeleton* CheckIfModelSKELLoadedUnsafeManualRef(const string& strFileName, uint32 nLoadingFlags);
    _smart_ptr<CDefaultSkeleton> CheckIfModelSKELLoadedAutoRef(const string& strFileName, uint32 nLoadingFlags);
    CDefaultSkeleton* CreateExtendedSkel(CCharInstance* pCharInstance, CDefaultSkeleton* pDefaultSkeleton, uint64 nExtendedCRC64, const DynArray<const char*>& arrNotMatchingSkins, const uint32 nLoadingFlags);
#ifdef EDITOR_PCDEBUGCODE
    virtual void ClearAllKeepInMemFlags();
#endif

    CSkin* CheckIfModelSKINLoadedUnsafeManualRef(const string& strFileName, uint32 nLoadingFlags);
    _smart_ptr<CSkin> CheckIfModelSKINLoadedAutoRef(const string& strFileName, uint32 nLoadingFlags);
    CDefaultSkeleton* CheckIfModelExtSKELCreated (const uint64 nCRC64, uint32 nLoadingFlags);
    void UpdateStreaming_SKEL(std::vector<CDefaultSkeletonReferences>& skels, uint32 nRenderFrameId, const uint32* nRoundIds);
    void UpdateStreaming_SKIN(std::vector<CDefaultSkinningReferences>& skins, uint32 nRenderFrameId, const uint32* nRoundIds);

    virtual bool LoadAndLockResources(const char* szFilePath, uint32 nLoadingFlags);
    virtual void StreamKeepCharacterResourcesResident(const char* szFilePath, int nLod, bool bKeep, bool bUrgent = false);
    virtual bool StreamHasCharacterResources(const char* szFilePath, int nLod);
    virtual void GetLoadedModels(IDefaultSkeleton** pIDefaultSkeleton, uint32& nCount) const;
    virtual void ReloadAllModels();
    virtual void ReloadAllCHRPARAMS();
    virtual void PreloadLevelModels();

    void GetModelCacheSize() const;
    void DebugModelCache(uint32 printtxt, std::vector<CDefaultSkeletonReferences>& parrModelCache, std::vector<CDefaultSkinningReferences>& parrModelCacheSKIN);
    virtual void ClearResources(bool bForceCleanup);    //! Cleans up all resources - currently deletes all bodies and characters (even if there are references on them)
    void CleanupModelCache(bool bForceCleanup);         // deletes all registered bodies; the character instances got deleted even if they're still referenced
    void GetStatistics(Statistics& rStats) const;   // returns statistics about this instance of character animation manager don't call this too frequently
    void GetMemoryUsage(ICrySizer* pSizer) const;  //puts the size of the whole subsystem into this sizer object, classified, according to the flags set in the sizer
    void TrackMemoryOfModels();
    std::vector<CDefaultSkeletonReferences> m_arrModelCacheSKEL;
    std::vector<CDefaultSkinningReferences> m_arrModelCacheSKIN;

    uint32 GetCDFIdUnsafe(const AZStd::string& pathname);

#if BLENDSPACE_VISUALIZATION
    void CreateDebugInstances(const char* szCharacterFileName);
    void DeleteDebugInstances();
    void RenderDebugInstances(const SRenderingPassInfo& passInfo);
    void RenderBlendSpace(const SRenderingPassInfo& passInfo, ICharacterInstance* pCharacterInstance, float fCharacterScale, unsigned int flags);
    bool HasAnyDebugInstancesCreated() const;
    bool HasDebugInstancesCreated(const char* szCharacterFileName) const;
    DynArray<DebugInstances> m_arrCharacterBase;
#endif
#ifdef EDITOR_PCDEBUGCODE
    virtual void GetMotionParameterDetails(SMotionParameterDetails& outDetails, EMotionParamID paramId) const;
    virtual bool InjectCDF(const char* pathname, const char* content, size_t contentLength);
    virtual void ClearCDFCache() { m_arrCacheForCDF.clear(); m_pendingCDFLoads.clear(); } //deactivate the cache in Editor-Mode, or we can't load the same CDF after we changed & saved it

    virtual void InjectBSPACE(const char* pathname, const char* content, size_t contentLength);
    virtual void ClearBSPACECache();
    std::vector<CDefaultSkeletonReferences> m_arrModelCacheSKEL_CharEdit;
    std::vector<CDefaultSkinningReferences> m_arrModelCacheSKIN_CharEdit;
#endif

    void ReloadQueuedAssets();
    bool IsAnimationCurrentlyBeingUsedByAnimationSet(int globalAnimationID);
    void LoadAnimationToValidSets(const AZStd::string& assetPath);
    void RemoveAnimationFromValidSets(const AZStd::string& assetPath);

    bool LiveReloadSkin(const AZStd::string& assetPath, std::vector<CDefaultSkinningReferences>& modelCacheSKIN, uint32 loadingFlags);
    bool CreateAndBindNewSkin(const AZStd::string& assetPath, DynArray<CAttachmentSKIN*>& attachmentInstances, uint32 loadingFlags, bool keepInMemory);
    bool RemoveSkin(const AZStd::string& assetPath, DynArray<CAttachmentSKIN*>& attachmentInstances);

    bool LiveReloadSkeletonSet(const AZStd::string& assetPath, std::vector<CDefaultSkeletonReferences>& modelCacheSkeleton, uint32 loadingFlags);

    bool UpdateSkeletonCharacterInstances(_smart_ptr<CDefaultSkeleton> baseDefaultSkeleton, std::vector<CDefaultSkeletonReferences>& skeletonList, const uint32 nLoadingFlags);
    void ReloadCharacterInstanceSkeleton(CCharInstance* pCharInstance, _smart_ptr<CDefaultSkeleton> pDefaultSkeleton, const uint32 nLoadingFlags);

    bool LiveReloadSkeletonSetCHRPARAMS(const AZStd::string& assetPath, std::vector<CDefaultSkeletonReferences>& skeletonList, uint32 loadingFlags);
    void ReloadSkeletonCHRPARAMS(const AZStd::string& assetPath, CDefaultSkeletonReferences* skeletonReferences, const uint32 nLoadingFlags);

    AZStd::set<AZStd::string> m_queuedReloadSet;
    static const float s_reloadTimeout;
    float m_reloadRequestTimeStamp;

    CAnimationManager m_AnimationManager;

    CFacialAnimation*       m_pFacialAnimation;

    CParamLoader m_ParamLoader;

    std::vector<f32> m_arrFrameTimes;

    AZStd::vector<AZStd::unique_ptr<CharacterDefinition>> m_arrCacheForCDF;

    struct PendingLoad
    {
        PendingLoad() {}

        PendingLoad(AZStd::unique_ptr<ManualResetEvent> manualResetEvent)
            : m_refCount(0)
            , m_resetEvent(std::move(manualResetEvent))
        { }
        PendingLoad(const PendingLoad&) = delete;
        PendingLoad(PendingLoad&& rhs)
            : m_refCount(rhs.m_refCount)
            , m_resetEvent(AZStd::move(rhs.m_resetEvent))
        {
            rhs.m_refCount = 0;
        }

        int m_refCount;
        AZStd::unique_ptr<ManualResetEvent> m_resetEvent;
    };

    AZStd::unordered_map<AZStd::string, PendingLoad> m_pendingCDFLoads;

    std::vector<ICharacterInstance*> m_AnimationSyncQueue;  // queue to remember all animations which were started and need a sync

    void ProcessAsyncLoadRequests();

    AZStd::mutex m_asyncLoadQueueLock; // Lock for m_asyncLoadRequests
    std::queue<InstanceAsyncLoadRequest> m_asyncLoadRequests;

    uint64 m_nFrameTicks;                                   // number of ticks spend in animations function during the last frame
    uint64 m_nFrameSyncTicks;                           // number of ticks spend in animations sync function during the last frame
    uint32 m_nActiveCharactersLastFrame;    // number of characters for which ForwardKinematics was called

    uint32 m_nStreamUpdateRoundId[MAX_STREAM_PREDICTION_ZONES];

    uint32 m_lastDatabaseUnloadTimeStamp;

    // geometry data cache for VCloth
    SClothGeometry* LoadVClothGeometry(const CAttachmentVCLOTH& pRendAtt, _smart_ptr<IRenderMesh> pRenderMeshes[]);
    typedef std::map<uint64, SClothGeometry> TClothGeomCache;
    TClothGeomCache m_clothGeometries;

    AZStd::unordered_set<CDefaultSkeleton*> m_defaultSkelGarbageSet;
    AZStd::unordered_set<CSkin*> m_skinGarbageSet;

    mutable AZStd::mutex m_cdfCacheMutex;

    //! Must be taken before m_garbageMutex if taking both
    mutable AZStd::recursive_mutex m_cacheSkinMutex;

    //! Must be taken before m_garbageMutex if taking both
    mutable AZStd::recursive_mutex m_cacheSkelMutex;

    //! Must be taken after m_cacheSkelMutex/m_cacheSkinMutex if taking both
    mutable AZStd::mutex m_garbageMutex;
};


#endif // CRYINCLUDE_CRYANIMATION_CHARACTERMANAGER_H
