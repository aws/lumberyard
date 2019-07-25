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

#ifndef CRYINCLUDE_CRY3DENGINE_MERGEDMESHRENDERNODE_H
#define CRYINCLUDE_CRY3DENGINE_MERGEDMESHRENDERNODE_H
#pragma once

#include <limits>
#include "ObjMan.h"
#include "Cry3DEngineTraits.h"
#include "MergedMeshJobExecutor.h"

// Global define to enable and disable some debugging features (defined &
// described below) to help finding issues in merged meshes at runtime.
#define MMRM_DEBUG 1

// Enable this define to include code that performs debug visualization of the
// state of merged meshes
#define MMRM_RENDER_DEBUG 1

// Enable this define to perform memory footprint monitoring
#define MMRM_SIZE_INFO 1

// The maximum number of potential colliders is set below. If more potential
// colliders are found than this limit below, only this number of colliders
// will be respected
#define MMRM_MAX_COLLIDERS (48)

// The maximum number of potential projectiles is set below. If more potential
// projectiles are found than this limit below, only this number of projectiles
// will be respected
#define MMRM_MAX_PROJECTILES (12)

// The maximum number of contacts that a deformable is allowed to generate.
#define MMRM_MAX_CONTACTS (32)

// The number of wind force samples along each principle axis.
#define MMRM_WIND_DIM 4

// The dimensions of the density grid on each patch (16x16x16 means one sample every metre)
#define MMRM_DENSITY_DIM 16

// Enable code that checks for boundaries when accessing and writing back the
// interleaved baked buffers.
#define MMRM_USE_BOUNDS_CHECK 0

// Enable mmrm local debug printing statements
#define MMRM_DEBUG_PRINT 0

// Enable mmrm - local assertions
#define MMRM_DEBUG_ASSERT 0

// Set this to one to enable full fp32 simulation
#define MMRM_SIMULATION_USES_FP32 1

// Enforce bending via relative angles : Ensure that the spine local-space
// angle relative to it's parent does not differ too far from the resting
// pose. If so a simple lerp is performed to ensure (not correct, but efficient
// & sufficient).
#define MMRM_SPINE_SLERP_BENDING 1

// Enforce bending - ensure that the height of the current spine vertex
// relative to the line segment defined by it's immediate neighbour vertices
// matches the initial height recorded in it's rest-pose
#define MMRM_SPINE_HEIGHT_BENDING 0

// Bending enforcement options are mutually exclusive
#if MMRM_SPINE_HEIGHT_BENDING && MMRM_SPINE_SLERP_BENDING
# error Please select either MMRM_SPINE_HEIGHT_BENDING or MMRM_SPINE_SLERP_BENDING
#endif

// Visualize the wind sample grid (NOTE: requires USE_JOB_SYSTEM to be false!)
#define MMRM_VISUALIZE_WINDFIELD 0

// Visualize the wind samples from the wind grid (NOTE: requires USE_JOB_SYSTEM
// to be false!)
#define MMRM_VISUALIZE_WINDSAMPLES 0

// Visualize the forces acting on the simulated particles (NOTE: requires
// USE_JOB_SYSTEM to be false!)
#define MMRM_VISUALIZE_FORCES 0

// Enables/disables the use of vectorized instructions for large parts of the
// simulation / skinning
#define MMRM_USE_VECTORIZED_SSE_INSTRUCTIONS 1

// Enables/disables the use of the frameprofiler for mergedmesh geometry stages
#define MMRM_ENABLE_PROFILER 1

// Unrolls the geometry baking loops, can result in very large functions
// exceeding the instruction cache footprint. Best to be turned off on AMD
// Jaguar platforms, as the tight loop exceeds the 32kb L1I cache size
#define MMRM_UNROLL_GEOMETRY_BAKING_LOOPS AZ_LEGACY_3DENGINE_TRAIT_UNROLL_GEOMETRY_BACKING_LOOPS

// Number of instances that will submitted per job
#define MMRM_MAX_SAMPLES_PER_BATCH (1024u)

// Disable the use of vectorized on PC because a plethora of them are actually
// not ported yet
#if defined(APPLE) || defined(LINUX)
# undef MMRM_USE_VECTORIZED_SSE_INSTRUCTIONS
# define MMRM_USE_VECTORIZED_SSE_INSTRUCTIONS 0
#endif

// disable vectorized instructions in debug builds
#if defined(_DEBUG)
    #undef MMRM_USE_VECTORIZED_SSE_INSTRUCTIONS
    #define MMRM_USE_VECTORIZED_SSE_INSTRUCTIONS 0
#endif

// The compile-time default poolsize in bytes
#define MMRM_DEFAULT_POOLSIZE_STR "2750"

// Increase the damping for this amount of time when undergoing severe contacts
#define MMRM_PLASTICITY_TIME (10000)

// The fixed timestep for the simulator
#define MMRM_FIXED_STEP (0.035f)

// The enable debug visualization of the clusters
#define MMRM_CLUSTER_VISUALIZATION 1

#if !MMRM_DEBUG || defined(_RELEASE)
# undef MEMORY_ENABLE_DEBUG_HEAP
# undef MMRM_RENDER_DEBUG
# undef MMRM_PROFILE
# undef MMRM_SIZE_INFO
# undef MMRM_CLUSTER_VISUALIZATION
# define MEMORY_ENABLE_DEBUG_HEAP 0
# define MMRM_RENDER_DEBUG 0
# define MMRM_PROFILE 0
# define MMRM_SIZE_INFO 0
# define MMRM_CLUSTER_VISUALIZATION 0
#endif

#if MMRM_DEBUG_PRINT
# define mmrm_printf(...) CryLog(__VA_ARGS__)
#endif
#ifndef mmrm_printf
# define mmrm_printf(...) (void)0
#endif

#if MMRM_DEBUG_ASSERT
# define mmrm_assert(x) do { if ((x) == false) {__debugbreak(); } \
} while (false)
#endif
#ifndef mmrm_assert
# define mmrm_assert(x) (void)0
#endif

////////////////////////////////////////////////////////////////////////////////
// Forward declarations
struct SMMRMGroupHeader;
struct SMMRM;
struct SSampleDensity;
struct SProcVegSample;
struct SMMRMProfilingInfo;
struct SMergedMeshPriorityCmp;
struct SMMRMProjectile;
struct SMergedMeshSectorChunk;
struct SMergedMeshInstanceCompressed;

////////////////////////////////////////////////////////////////////////////////
// RenderNode for merged meshes
//
// Merged meshes work by storing multiple linear lists of instances (currently
// position, scale and rotation) each associated with a specially preprocessed
// cgf. If the merged mesh rendernode is visible, a culling job per instance
// type is spawned that marks the instances that are visible and also
// accumulates the required buffer sizes for the rendermesh that will be used
// to render the instances in least number of drawcalls possible (taking size
// limitations (0xffff max vertex count), material switches and other
// limitations into account). After culling, the resulting rendermesh(es) will
// be created and the buffer's queried for their data location and finally an
// update job is spawned for each visible instance type that will bake the
// actual per-instance mesh into the large buffer at the pre-determined offsets
// calculated during culling.
//
// If the instance type's associated cgf contains touchbending (foliage spines)
// , an additional simple simulation pass is triggered that will simulate the
// bones with a simple one-iteration PDB based approach and skin the resulting
// instance. Proper wind-sampling is taken into account.
//
// To alleviate the storage requirements of the instance list (and the
// simulation state should touch bending be enabled on an instance type), the
// actual storage can be streamed out and in on demand on runtime.
//
// ToDo items:
//
//  - The rendermesh is re-created every frame and not pooled currently, so
// vram can (and will) run out of bounds if care is not taken to reduce the
// load. This will be addressed in a near-future commit.
//
//  - The cgf is preprocessed at runtime, this can be moved to level export time
//
class CMergedMeshRenderNode
    : public IRenderNode
    , public IStreamCallback
    , public Cry3DEngineBase
{
    friend class COctreeNode;
    friend class CMergedMeshesManager;
    friend struct SMergedMeshInstanceSorter;
    friend class CTerrain;
    // FrameID of the last frame this rendernode was drawn and updated
    uint32 m_LastDrawFrame = 0;
    uint32 m_LastUpdateFrame = 0;

    // The state the rendernode can reside in
    enum State
    {
        INITIALIZED = 0,        // initialized (created)
        DIRTY,                          // changes to the instance list or any attached cgf has been made
        PREPARING,                  // the attached cgfs are being preprocessed asynchronously
        PREPARED,                       // default state if instance data is not present (opposite of streamed in)
        STREAMING,                  // instance data is being streamed in
        STREAMED_IN,                // instance data is present and the rendernode can be rendered
        RENDERNODE_STATE_ERROR
    };
    inline const char* ToString(State state) const
    {
        switch (state)
        {
        case INITIALIZED:
            return "INITIALIZED";
        case DIRTY:
            return "DIRTY";
        case PREPARING:
            return "PREPARING";
        case PREPARED:
            return "PREPARED";
        case STREAMING:
            return "STREAMING";
        case STREAMED_IN:
            return "STREAMED_IN";
        case RENDERNODE_STATE_ERROR:
            return "ERROR";
        }
        return "UNKNOWN";
    }
    volatile State m_State = INITIALIZED;
    enum RENDERMESH_UPDATE_TYPE
    {
        RUT_STATIC = 0,
        RUT_DYNAMIC = 1,

        RUT_NUM_UPDATE_TYPES
    };

    // The number of instances this rendernode maintains
    size_t m_Instances = 0;
    mutable size_t m_SpineCount = std::numeric_limits<size_t>::max();
    mutable size_t m_DeformCount = std::numeric_limits<size_t>::max();

    // The distance to camera
    float m_DistanceSQ = 0.0f;

    // The internal extents used as to determine the unique spatial hash of this rendernode
    AABB m_internalAABB = AABB::RESET;

    // The extents of the instance geometry attached to this rendernode
    AABB m_visibleAABB = AABB::RESET;

    // Position of this rendernode - corresponds to the center of the internal aabb
    Vec3 m_pos;

    // Initial position of this rendernode before applying world shift
    Vec3 m_initPos;

    // The position of the camera the last time we updated the merged mesh
    Vec3 m_cameraPosAtLastUpdate;

    // A z rotation to apply to this rendernode
    float m_zRotation = 0.0f;

    // The (external) lod
    int m_nLod[RUT_NUM_UPDATE_TYPES] = { -1, -1 };

    // Is the mesh already put into the active list?
    unsigned int m_nActive : 1;

    // Is the mesh already put into the visible list?
    unsigned int m_nVisible : 1;

    // Does the mesh need a static mesh update
    unsigned int m_needsStaticMeshUpdate : 1;

    // Does the mesh need a dynamic mesh update
    unsigned int m_needsDynamicMeshUpdate : 1;

    // Is the mesh needs a static mesh update
    unsigned int m_needsPostRenderStatic : 1;

    // Is the mesh needs a static mesh update
    unsigned int m_needsPostRenderDynamic : 1;

    // If the mesh owns its groups
    unsigned int m_ownsGroups : 1;

    // If the node owns an instance that is controlled by the Dynamic Vegetation system
    unsigned int m_hasDynamicInstances : 1;

    // The render state
    enum RenderMode
    {
        DYNAMIC = 0, // dynamic - prebaked & optionally simulated unique meshes
        INSTANCED,   // instanced - preprocessed
        NOT_VISIBLE, // simply not visible
    };
    RenderMode m_RenderMode = NOT_VISIBLE;
    inline const char* ToString(RenderMode renderMode) const
    {
        switch (renderMode)
        {
        case INSTANCED:
            return "INSTANCED";
        case DYNAMIC:
            return "DYNAMIC";
        case NOT_VISIBLE:
            return "NOT_VISIBLE";
        }
        return "UNKNOWN";
    }

    // VRAM Memory footprint of the corresponding buffers in bytes
    uint32 m_SizeInVRam = 0;

    // The instance lists.
    SMMRMGroupHeader* m_groups = nullptr;
    uint32 m_nGroups = 0;

    // Stored render params because this rendernode is rendered in a two-stage process.
    SRendParams m_rendParams;

    // The wind force sample lattice. Gets bilinearly interpolated during the simulation.
    Vec3* m_wind = nullptr;

    // The list of surface types used by the instances in this node
    ISurfaceType* m_surface_types[MMRM_MAX_SURFACE_TYPES];

    // The density samples
    SSampleDensity* m_density = nullptr;

    // The list of colliders, approximated as spheres. Probably more elaborate
    // primitives will be supported in the future
    primitives::sphere* m_Colliders = nullptr;

    // Number of active colliders
    int m_nColliders = 0;

    // The list of potential projectiles, approximated as spheres and directions.
    SMMRMProjectile* m_Projectiles = nullptr;

    // Number of active projectiles
    int m_nProjectiles = 0;

    // The jobstate for all cull jobs 
    MergedMeshJobExecutor m_cullJobExecutor;

    // The jobstate for all mesh update jobs
    MergedMeshJobExecutor m_updateJobExecutor;

    // The jobstate for initializing spines
    MergedMeshJobExecutor m_spineInitializationJobExecutor;

    // The list of rendermeshes that belong to this object
    std::vector<SMMRM> m_renderMeshes[2];

    // Only valid if we are currently streaming in data
    IReadStream_AutoPtr m_pReadStream;

    // Has the spine data already been initialized?
    bool m_SpinesInitialized = false;

    // Does this render node have any spine / deformation data?
    bool m_SpinesExist = false;

    // Debug visualization
    void DebugRender();

    // Checks if a member of a group requests to cast shadows
    bool GroupsCastShadows(RENDERMESH_UPDATE_TYPE type);

    // Submits the merged rendermesh to the renderer
    void RenderRenderMesh(
        RENDERMESH_UPDATE_TYPE type
        , const float fDistance
        , const SRenderingPassInfo& passInfo
        , const SRendItemSorter& rendItemSorter);

    // Creates the rendermesh based on the results of the culling jobs
    // Note: The culling jobs have to have all completed *before* calling this method
    void CreateRenderMesh(RENDERMESH_UPDATE_TYPE, const SRenderingPassInfo &passInfo);

    // Deletes the rendermesh - note if any update jobs are still in flight, this
    // call will block
    // The zap parameter actually frees the memory of the std vectors in flight
    bool DeleteRenderMesh(RENDERMESH_UPDATE_TYPE, bool block = true, bool zap = false);

    // Print the state to the screen
    void PrintState(float&);

    // Calculate the sample density grid.
    void CalculateDensity();

    // Query the sample density grid. Returns the number of surface types filled into the surface types list
    size_t QueryDensity(const Vec3 &pos, ISurfaceType * (&surfaceTypes)[MMRM_MAX_SURFACE_TYPES], float (&density)[MMRM_MAX_SURFACE_TYPES]);

    // Resets the node to the initial dirty state
    void Reset();

    // Initialize the spine / deformation data for a single group.
    void InitializeSpineAndDeformationData(SMMRMGroupHeader* header);

public:
    CMergedMeshRenderNode();
    virtual ~CMergedMeshRenderNode();

    // Set the configured position and offset and create the sample maps
    bool Setup(const AABB& aabb, const AABB& visAbb, const PodArray<SProcVegSample>*);

    // Add a instance type to the merged mesh render node and preallocate memory
    // for the actual instances
    // Returns false if the group cannot be created for some reason.
    bool AddGroup(uint32 statInstGroupId, uint32 numInstances);

    // Adds an instance to the map
    IRenderNode* AddInstance(const SProcVegSample&, bool bRegister = true);
    // Removes an instance from the map
    size_t RemoveInstance(size_t, size_t);

    // Clear the instance map data - zaps everything
    void Clear();

    size_t SizeInVRam() const { return m_SizeInVRam; }

    const AABB GetInternalBBox() const { return m_internalAABB; }

    bool IsDynamic() const { return m_RenderMode == DYNAMIC; }

    // True if any of the groups inside this merged mesh uses terrain color.
    bool UsesTerrainColor() const;

    // Access to the instance types of this
    uint32 NumGroups() const;
    const SMMRMGroupHeader* Group(size_t index) const;

    // Returns the (main memory) size in bytes of the allocated internal structures to represent
    // all the instances.
    uint32 MetaSize() const;

    // Returns the number of visible instances
    uint32 VisibleInstances(uint32 frameId) const;

    // Returns the number of spine instances
    uint32 SpineCount() const;

    // Returns the number of deformed instances
    uint32 DeformCount() const;

    // Activate Spines in memory
    void ActivateSpines();

    // Remove the spines from memory
    void RemoveSpines();

    // Is visible?
    bool Visible() const;

    // Distance to Camera
    float DistanceSq() const { return m_DistanceSQ; }

    // Active/Inactive flags
    int IsActive() const { return m_nActive; }
    void SetActive(int active) { m_nActive = active; }

    int IsInVisibleSet() const { return m_nVisible; }
    void SetInVisibleSet(int visible) { m_nVisible = visible; }

    //
    void OverrideViewDistRatio(float value);
    void OverrideLodRatio(float value);

    // Important Note: This method should not be called from any client code,
    // it is called implicitly from Setup. Sadly, it has to be public
    // because DECLARE_CLASS_JOB macros only work on public methods.
    bool PrepareRenderMesh(RENDERMESH_UPDATE_TYPE, int nLod = 0);

    // Compile the instance data into a streamable chunk
    bool Compile(byte* pData, int& nSize, string* pName, std::vector<struct IStatInstGroup*>* pVegGroupTable);

    // Fills the array full of samples
    void FillSamples(DynArray<Vec2>&);

    // Stream in data from the file system for this render node
    // Note the extents of the internal aabb are used to determine the filename
    bool StreamIn();

    // Flush out any allocated data
    bool StreamOut();

    bool SyncAllJobs();

    bool StreamedIn() const { return m_State == STREAMED_IN; }

    // A MergedMeshRenderNode can be streamed in and out only in the runtime, and only for static instances.
    // In-editor, nodes for static instances are created through editor data files, not runtime data files, so
    // the runtime data files might not exist to stream from, or might be out-of-date.  For dynamic instances,
    // the expectation is that "streaming" is controlled via spawning / despawning of dynamic vegetation areas,
    // not by controlling it at the merged mesh sector level.
    bool SupportsStreaming() const { return (!gEnv->IsEditor() && !gEnv->IsDedicated() && (m_hasDynamicInstances == false)); }

    // Update streamable components
    bool UpdateStreamableComponents(float fImportance, float fEntDistance, bool bFullUpdate);

    Vec3 GetSamplePos(size_t, size_t) const;
    AABB GetSampleAABB(size_t, size_t) const;

    bool PostRender(const SRenderingPassInfo& passInfo);

    // util job entry function used by AsyncStreaming Callback
    void InitializeSamples(float fExtents, const uint8* pBuffer);
    // util job entry function used by Activate Spines
    void InitializeSpines();

    void QueryColliders();
    void QueryProjectiles();
    void SampleWind();


    //////////////////////////////////////////////////////////////////////
    // Inherited from IStreamCallback
    //////////////////////////////////////////////////////////////////////
    void StreamAsyncOnComplete (IReadStream* pStream, unsigned nError);
    void StreamOnComplete (IReadStream* pStream, unsigned nError);

    //////////////////////////////////////////////////////////////////////
    // Inherited from IRenderNode
    //////////////////////////////////////////////////////////////////////
    const char* GetName() const { return "Runtime MergedMesh";  }
    const char* GetEntityClassName() const { return "Runtime MergedMesh"; };
    Vec3 GetPos(bool bWorldOnly = true) const;
    const AABB GetBBox() const { return m_visibleAABB; };
    void SetBBox(const AABB& WSBBox) { };
    void FillBBox(AABB& aabb);
    void OffsetPosition(const Vec3& delta);
    void Render(const struct SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo);

    struct IPhysicalEntity* GetPhysics() const { return NULL; };
    void SetPhysics(IPhysicalEntity* pPhys) { };
    void SetMaterial(_smart_ptr<IMaterial> pMat) override {}
    _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL);
    _smart_ptr<IMaterial> GetMaterialOverride(){return NULL; }
    EERType GetRenderNodeType();
    float GetMaxViewDist();
    void GetMemoryUsage(ICrySizer* pSizer) const {}

    StatInstGroup& GetStatObjGroup(const int index) const
    {
        return GetObjManager()->GetListStaticTypes()[0][index];
    }
    IStatObj* GetStatObj(const int index) const
    {
        return GetObjManager()->GetListStaticTypes()[0][index].GetStatObj();
    }
};

////////////////////////////////////////////////////////////////////////////////
// RenderNode Proxy node sandbox
//
// Instances of this node are just a refcounting wrapper exposed to the editor
// during vegetation editing. Construction and destruction of instances of this
// class correspond to adding and removing instances to a real
// MergedMeshRendernode
//
class CMergedMeshInstanceProxy
    : public IRenderNode
    , public Cry3DEngineBase
{
    friend class CMergedMeshRenderNode;

    CMergedMeshRenderNode* m_host;
    size_t m_headerIndex;
    size_t m_sampleIndex;

    CMergedMeshInstanceProxy(CMergedMeshRenderNode*, size_t, size_t);

public:
    CMergedMeshInstanceProxy();
    virtual ~CMergedMeshInstanceProxy();

    //////////////////////////////////////////////////////////////////////
    // Inherited from IRenderNode
    //////////////////////////////////////////////////////////////////////
    const char* GetName() const { return "Runtime MergedMesh Proxy Instance";  }
    const char* GetEntityClassName() const { return "Runtime MergedMesh Proxy Instance"; };
    Vec3 GetPos(bool bWorldOnly = true) const { return m_host->GetSamplePos(m_headerIndex, m_sampleIndex); };
    const AABB GetBBox() const { return m_host->GetSampleAABB(m_headerIndex, m_sampleIndex); };
    void SetBBox(const AABB& WSBBox) {__debugbreak(); };
    void OffsetPosition(const Vec3& delta) {}
    void Render(const struct SRendParams& EntDrawParams, const SRenderingPassInfo& passInfo) {__debugbreak(); }

    struct IPhysicalEntity* GetPhysics() const { __debugbreak(); return NULL; };
    void SetPhysics(IPhysicalEntity* pPhys) {__debugbreak(); };
    void SetMaterial(_smart_ptr<IMaterial> pMat) override {__debugbreak(); }
    _smart_ptr<IMaterial> GetMaterial(Vec3* pHitPos = NULL){ __debugbreak(); return NULL; }
    _smart_ptr<IMaterial> GetMaterialOverride(){ __debugbreak(); return NULL; }
    EERType GetRenderNodeType() { __debugbreak(); return eERType_MergedMesh; }
    float GetMaxViewDist() { __debugbreak(); return FLT_MAX; }
    void GetMemoryUsage(ICrySizer* pSizer) const {__debugbreak(); }
};


////////////////////////////////////////////////////////////////////////////////
// MergedMesh Manager
//
// Contains a wrapping spatial grid of mergedmesh rendernodes
struct SProjectile
{
    Vec3  current_pos;
    Vec3  initial_pos;
    Vec3  direction;
    float lifetime;
    float size;
    IPhysicalEntity* entity;

    SProjectile()
        : current_pos()
        , initial_pos()
        , direction()
        , lifetime()
        , size()
        , entity()
    {}

    ~SProjectile() { memset(this, 0, sizeof(*this)); }
    bool operator< (const SProjectile& other) const { return entity < other.entity; }
};

class CMergedMeshesManager
    : public Cry3DEngineBase
    , public IMergedMeshesManager
{
    friend class CMergedMeshRenderNode;
    friend class CDeformableNode;
    friend inline void QueryProjectiles(SMMRMProjectile*&, int&, const AABB&);

    enum
    {
        HashDimXY = 32, HashDimZ = 2, MaxProjectiles = 1024, MaxUnloadFrames = 128
    };

    typedef std::vector<CMergedMeshRenderNode*> NodeListT;
    typedef std::vector<CMergedMeshRenderNode*> NodeArrayT;
    typedef std::vector<SProjectile> ProjectileArrayT;

    // For tracking fast moving projectiles
    static int OnPhysPostStep(const EventPhys*);

    NodeArrayT m_Nodes[HashDimXY][HashDimXY][HashDimZ];
    NodeArrayT m_ActiveNodes;
    NodeArrayT m_VisibleNodes;
    NodeArrayT m_SegNodes;
    NodeArrayT m_PostRenderNodes;
    ProjectileArrayT m_Projectiles;
    volatile int m_ProjectileLock;

    MergedMeshJobExecutor m_nodeCullJobExecutor;

    MergedMeshJobExecutor m_nodeUpdateJobExecutor;

    MergedMeshJobExecutor m_nodeSpineInitJobExecutor;

    // The jobstate for all mesh update jobs
    MergedMeshJobExecutor m_updateCompletionMergedMeshesManager;

    size_t  m_CurrentSizeInVramDynamic;
    size_t  m_CurrentSizeInVramInstanced;
    size_t  m_CurrentSizeInMainMem;
    size_t  m_GeomSizeInMainMem;
    size_t  m_InstanceCount;
    size_t  m_VisibleInstances;
    size_t  m_InstanceSize;
    size_t  m_SpineSize;
    size_t  m_nActiveNodes;
    bool  m_PoolOverFlow;
    bool  m_MeshListPresent;

    AABB m_CachedBBs[4];
    int  m_CachedLRU[4];
    CMergedMeshRenderNode* m_CachedNodes[4];

#if MMRM_CLUSTER_VISUALIZATION
    DynArray<SMeshAreaCluster> m_clusters;
#endif // MMRM_CLUSTER_VISUALIZATION

    CMergedMeshRenderNode* FindNode(const Vec3& pos);

    void AddProjectile(const SProjectile&);

public:
    CMergedMeshesManager();
    ~CMergedMeshesManager();

    void Init();
    void Shutdown();

    // Called by the
    void UpdateViewDistRatio(float);
    void UpdateLodRatio(float);

    // Compile sectors
    bool CompileSectors(DynArray<SInstanceSector>& pSectors, std::vector<struct IStatInstGroup*>* pVegGroupTable);

    // Compile cluster areas
    bool CompileAreas(DynArray<SMeshAreaCluster>& clusters, int flags);

    //  Query the sample density grid. Returns the number of surface types filled into the surface types list
    size_t QueryDensity(const Vec3 &pos, ISurfaceType * (&surfaceTypes)[MMRM_MAX_SURFACE_TYPES], float (&density)[MMRM_MAX_SURFACE_TYPES]);

    // Fill in the density values
    void CalculateDensity();

    bool GetUsedMeshes(DynArray<string>& pMeshNames);

    void PreloadMeshes();
    //! Returns true if the preparation step succeeds or if there are no meshes to prepare, false otherwise
    bool SyncPreparationStep();

    IRenderNode* AddInstance(const SProcVegSample&, CMergedMeshRenderNode** ppNode = nullptr, bool bRegister = true);
    IRenderNode* AddDynamicInstance(const IMergedMeshesManager::SInstanceSample&, IRenderNode** ppNode, bool bRegister) override;

    CMergedMeshRenderNode* GetNode(const Vec3& vPos);
    void RemoveMergedMesh(CMergedMeshRenderNode*);

    void PostRenderMeshes(const SRenderingPassInfo& passInfo);
    void RegisterForPostRender(CMergedMeshRenderNode*);

    // Called once a frame
    void Update(const SRenderingPassInfo& passInfo);

    void SortActiveInstances(const SRenderingPassInfo& passInfo);
    void SortActiveInstances_Async(const SRenderingPassInfo passInfo); //NOTE: called internally
    void ResetActiveNodes();

    // Enable dynamic generation of merged meshes in game mode.
    static void SetDynamicGenerationEnabled(bool enable);

    size_t CurrentSizeInVram() const { return CurrentSizeInVramDynamic() + CurrentSizeInVramInstanced(); }
    size_t CurrentSizeInVramDynamic() const { return m_CurrentSizeInVramDynamic; }
    size_t CurrentSizeInVramInstanced() const { return m_CurrentSizeInVramInstanced; }
    size_t CurrentSizeInMainMem() const { return m_CurrentSizeInMainMem; }
    size_t GeomSizeInMainMem() const { return m_GeomSizeInMainMem; }
    size_t InstanceCount() const { return m_InstanceCount; }
    size_t VisibleInstances() const { return m_VisibleInstances; }
    size_t InstanceSize() const { return m_InstanceSize; }
    size_t SpineSize() const { return m_SpineSize; }
    size_t ActiveNodes() const { return m_nActiveNodes; }
    bool PoolOverFlow() const { return m_PoolOverFlow; }

    void PrepareSegmentData(const AABB& aabb);
    int GetSegmentNodeCount() { return m_SegNodes.size(); }
    int GetCompiledDataSize(uint32 index);
    bool GetCompiledData(uint32 index, byte* pData, int nSize, string* pName, std::vector<struct IStatInstGroup*>** ppStatInstGroupTable);
};


#endif // CRYINCLUDE_CRY3DENGINE_MERGEDMESHRENDERNODE_H
