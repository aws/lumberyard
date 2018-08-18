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

#ifndef CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_NAVIGATIONSYSTEM_H
#define CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_NAVIGATIONSYSTEM_H
#pragma once

#include <INavigationSystem.h>

#include "../MNM/MNM.h"
#include "../MNM/Tile.h"
#include "../MNM/MeshGrid.h"
#include "../MNM/TileGenerator.h"

#include "WorldMonitor.h"
#include "OffMeshNavigationManager.h"
#include "VolumesManager.h"
#include "IslandConnectionsManager.h"

#include <CryListenerSet.h>
#include <AzCore/Jobs/LegacyJobExecutor.h>

#if defined(WIN32) || defined(WIN64)
#define NAVIGATION_SYSTEM_EDITOR_BACKGROUND_UPDATE 1
#else
#define NAVIGATION_SYSTEM_EDITOR_BACKGROUND_UPDATE 0
#endif

#if defined(WIN32) || defined(WIN64)
#define NAVIGATION_SYSTEM_PC_ONLY 1
#else
#define NAVIGATION_SYSTEM_PC_ONLY 0
#endif

#if DEBUG_MNM_ENABLED || NAVIGATION_SYSTEM_EDITOR_BACKGROUND_UPDATE
class NavigationSystem;
struct NavigationMesh;
#endif

#if DEBUG_MNM_ENABLED
class NavigationSystem;
struct NavigationMesh;

class NavigationSystemDebugDraw
{
    struct NavigationSystemWorkingProgress
    {
        NavigationSystemWorkingProgress()
            : m_initialQueueSize(0)
            , m_currentQueueSize(0)
            , m_timeUpdating(0.0f)
        {
        }

        void Update(const float frameTime, const size_t queueSize);
        void Draw();

    private:

        void BeginDraw();
        void EndDraw();
        void DrawQuad(const Vec2& origin, const Vec2& size, const ColorB& color);

        float   m_timeUpdating;
        size_t  m_initialQueueSize;
        size_t  m_currentQueueSize;
        SAuxGeomRenderFlags m_oldRenderFlags;
    };

    struct DebugDrawSettings
    {
        DebugDrawSettings()
            : meshID(0)
        {
        }

        inline bool Valid() const { return (meshID != NavigationMeshID(0)); }

        NavigationMeshID meshID;

        size_t selectedX;
        size_t selectedY;
        size_t selectedZ;

        bool forceGeneration;
    };

public:

    NavigationSystemDebugDraw()
        : m_agentTypeID(0)
    {
    }

    inline void SetAgentType(const NavigationAgentTypeID agentType)
    {
        m_agentTypeID = agentType;
    }

    inline NavigationAgentTypeID GetAgentType() const
    {
        return m_agentTypeID;
    }

    void DebugDraw(NavigationSystem& navigationSystem);
    void UpdateWorkingProgress(const float frameTime, const size_t queueSize);

private:

    MNM::TileID DebugDrawTileGeneration(NavigationSystem& navigationSystem, const DebugDrawSettings& settings);
    void DebugDrawRayCast(NavigationSystem& navigationSystem, const DebugDrawSettings& settings);
    void DebugDrawPathFinder(NavigationSystem& navigationSystem, const DebugDrawSettings& settings);
    void DebugDrawClosestPoint(NavigationSystem& navigationSystem, const DebugDrawSettings& settings);
    void DebugDrawGroundPoint(NavigationSystem& navigationSystem, const DebugDrawSettings& settings);
    void DebugDrawIslandConnection(NavigationSystem& navigationSystem, const DebugDrawSettings& settings);

    void DebugDrawNavigationMeshesForSelectedAgent(NavigationSystem& navigationSystem, MNM::TileID excludeTileID);
    void DebugDrawNavigationSystemState(NavigationSystem& navigationSystem);
    void DebugDrawMemoryStats(NavigationSystem& navigationSystem);

    DebugDrawSettings GetDebugDrawSettings(NavigationSystem& navigationSystem);

    inline Vec3 TriangleCenter(const Vec3& a, const Vec3& b, const Vec3& c)
    {
        return (a + b + c) / 3.f;
    }

    NavigationAgentTypeID m_agentTypeID;
    NavigationSystemWorkingProgress m_progress;
};
#else
class NavigationSystemDebugDraw
{
public:
    inline void SetAgentType(const NavigationAgentTypeID agentType) { };
    inline NavigationAgentTypeID GetAgentType() const { return NavigationAgentTypeID(0); };
    inline void DebugDraw(const NavigationSystem& navigationSystem) { };
    inline void UpdateWorkingProgress(const float frameTime, const size_t queueSize) { };
};
#endif

#if NAVIGATION_SYSTEM_EDITOR_BACKGROUND_UPDATE
class NavigationSystemBackgroundUpdate
    : public ISystemEventListener
{
    class Thread
        : public CrySimpleThread<>
    {
        typedef CrySimpleThread<> Base;

    public:
        Thread(NavigationSystem& navigationSystem)
            : m_navigationSystem(navigationSystem)
            , m_requestedStop(false)
        {
        }

        virtual void Run();
        virtual void Cancel();

    private:
        NavigationSystem& m_navigationSystem;

        volatile bool     m_requestedStop;
    };

public:
    NavigationSystemBackgroundUpdate(NavigationSystem& navigationSystem)
        : m_pBackgroundThread(NULL)
        , m_navigationSystem(navigationSystem)
        , m_enabled(gEnv->IsEditor())
        , m_paused(false)
    {
        RegisterAsSystemListener();
    }

    ~NavigationSystemBackgroundUpdate()
    {
        RemoveAsSystemListener();
        Stop();
    }

    bool IsRunning() const
    {
        return (m_pBackgroundThread != NULL);
    }

    void Pause(const bool pause)
    {
        if (pause)
        {
            Stop();     // Stop and synch if necessary
        }

        m_paused = pause;
    }

    // ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventListener

private:

    bool Start();
    bool Stop();

    void RegisterAsSystemListener();
    void RemoveAsSystemListener();

    bool IsEnabled() const { return m_enabled; }

    Thread* m_pBackgroundThread;
    NavigationSystem& m_navigationSystem;

    bool              m_enabled;
    bool                            m_paused;
};
#else
class NavigationSystemBackgroundUpdate
{
public:
    NavigationSystemBackgroundUpdate(NavigationSystem& navigationSystem)
    {
    }

    bool IsRunning() const { return false; }
    void Pause(const bool readingOrSavingMesh) { }
};
#endif

typedef MNM::BoundingVolume NavigationBoundingVolume;

struct NavigationMesh
{
    NavigationMesh(NavigationAgentTypeID _agentTypeID)
        : agentTypeID(_agentTypeID)
        , version(0)
    {
    };

#if DEBUG_MNM_ENABLED
    struct ProfileMemoryStats
    {
        ProfileMemoryStats(const MNM::MeshGrid::ProfilerType& _gridProfiler)
            : gridProfiler(_gridProfiler)
            , totalNavigationMeshMemory(0)
        {
        }

        const MNM::MeshGrid::ProfilerType& gridProfiler;
        size_t totalNavigationMeshMemory;
    };

    ProfileMemoryStats GetMemoryStats(ICrySizer* pSizer) const;
#endif

    NavigationAgentTypeID agentTypeID;
    size_t version;

    MNM::MeshGrid grid;
    NavigationVolumeID boundary;
    typedef std::vector<NavigationVolumeID> ExclusionVolumes;
    ExclusionVolumes exclusions;
    string name; // TODO: this is currently duplicated for all agent types
};


struct AgentType
{
    // Copying the AgentType in a multi threaded environment
    // is not safe due to our string class being implemented as a
    // not thread safe Copy-on-Write
    // Using a custom assignment operator and a custom copy constructor
    // gives us thread safety without the usage of code locks

    AgentType() {}

    AgentType(const AgentType& other)
    {
        MakeDeepCopy(other);
    }

    struct Settings
    {
        Vec3 voxelSize;

        uint16 radiusVoxelCount;
        uint16 climbableVoxelCount;
        float climbableInclineGradient;
        float climbableStepRatio;
        uint16 heightVoxelCount;
        uint16 maxWaterDepthVoxelCount;
    };

    struct MeshInfo
    {
        MeshInfo(const NavigationMeshID& _id, uint32 _name)
            : id(_id)
            , name(_name)
        {
        }

        NavigationMeshID id;
        uint32 name;
    };

    AgentType& operator=(const AgentType& other)
    {
        MakeDeepCopy(other);

        return *this;
    }

    void MakeDeepCopy(const AgentType& other)
    {
        settings = other.settings;

        meshes = other.meshes;

        exclusions = other.exclusions;

        callbacks = other.callbacks;

        meshEntityCallback = other.meshEntityCallback;

        smartObjectUserClasses.reserve(other.smartObjectUserClasses.size());
        SmartObjectUserClasses::const_iterator end = other.smartObjectUserClasses.end();
        for (SmartObjectUserClasses::const_iterator it = other.smartObjectUserClasses.begin(); it != end; ++it)
        {
            smartObjectUserClasses.push_back(it->c_str());
        }

        name = other.name.c_str();
    }

    Settings settings;

    typedef std::vector<MeshInfo> Meshes;
    Meshes meshes;

    typedef std::vector<NavigationVolumeID> ExclusionVolumes;
    ExclusionVolumes exclusions;

    typedef std::vector<NavigationMeshChangeCallback> Callbacks;
    Callbacks callbacks;

    NavigationMeshEntityCallback meshEntityCallback;

    typedef std::vector<string> SmartObjectUserClasses;
    SmartObjectUserClasses smartObjectUserClasses;

    string name;
};

class NavigationSystem
    : public INavigationSystem
{
    friend class NavigationSystemDebugDraw;
    friend class NavigationSystemBackgroundUpdate;

public:
    NavigationSystem(const char* configName);
    ~NavigationSystem();

    virtual NavigationAgentTypeID CreateAgentType(const char* name, const CreateAgentTypeParams& params);
    virtual NavigationAgentTypeID GetAgentTypeID(const char* name) const;
    virtual NavigationAgentTypeID GetAgentTypeID(size_t index) const;
    virtual const char* GetAgentTypeName(NavigationAgentTypeID agentTypeID) const;
    virtual size_t GetAgentTypeCount() const;
    bool GetAgentTypeProperties(const NavigationAgentTypeID agentTypeID, AgentType& agentTypeProperties) const;

    virtual NavigationMeshID CreateMesh(const char* name, NavigationAgentTypeID agentTypeID, const CreateMeshParams& params);
    virtual NavigationMeshID CreateMesh(const char* name, NavigationAgentTypeID agentTypeID, const CreateMeshParams& params, NavigationMeshID requestedId);
    virtual void DestroyMesh(NavigationMeshID meshID);

    virtual void SetMeshEntityCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshEntityCallback& callback);
    virtual void AddMeshChangeCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshChangeCallback& callback);
    virtual void RemoveMeshChangeCallback(NavigationAgentTypeID agentTypeID, const NavigationMeshChangeCallback& callback);

    virtual NavigationVolumeID CreateVolume(Vec3* vertices, size_t vertexCount, float height);
    virtual NavigationVolumeID CreateVolume(Vec3* vertices, size_t vertexCount, float height, NavigationVolumeID requestedID);
    virtual void DestroyVolume(NavigationVolumeID volumeID);
    virtual void SetVolume(NavigationVolumeID volumeID, Vec3* vertices, size_t vertexCount, float height);
    virtual bool ValidateVolume(NavigationVolumeID volumeID);
    virtual NavigationVolumeID GetVolumeID(NavigationMeshID meshID);

    virtual void SetMeshBoundaryVolume(NavigationMeshID meshID, NavigationVolumeID volumeID);
    virtual void SetExclusionVolume(const NavigationAgentTypeID* agentTypeIDs, size_t agentTypeIDCount,
        NavigationVolumeID volumeID);

    virtual NavigationMeshID GetMeshID(const char* name, NavigationAgentTypeID agentTypeID) const;
    virtual const char* GetMeshName(NavigationMeshID meshID) const;
    virtual void SetMeshName(NavigationMeshID meshID, const char* name);

    virtual WorkingState GetState() const;
    virtual WorkingState Update(bool blocking);
    virtual void PauseNavigationUpdate();
    virtual void RestartNavigationUpdate();

    virtual size_t QueueMeshUpdate(NavigationMeshID meshID, const AABB& aabb);
    virtual void ProcessQueuedMeshUpdates();

    virtual void Clear();
    virtual void ClearAndNotify();
    virtual bool ReloadConfig();
    virtual void DebugDraw();
    virtual void Reset();

    void GetMemoryStatistics(ICrySizer* pSizer);

    virtual void SetDebugDisplayAgentType(NavigationAgentTypeID agentTypeID);
    virtual NavigationAgentTypeID GetDebugDisplayAgentType() const;

    void QueueDifferenceUpdate(NavigationMeshID meshID, const NavigationBoundingVolume& oldVolume,
        const NavigationBoundingVolume& newVolume);

    virtual void WorldChanged(const AABB& aabb);

    const NavigationMesh& GetMesh(const NavigationMeshID& meshID) const;
    NavigationMesh& GetMesh(const NavigationMeshID& meshID);
    NavigationMeshID GetEnclosingMeshID(NavigationAgentTypeID agentTypeID, const Vec3& location) const;
    bool IsLocationInMesh(NavigationMeshID meshID, const Vec3& location) const;
    MNM::TriangleID GetClosestMeshLocation(NavigationMeshID meshID, const Vec3& location, float vrange, float hrange,
        Vec3* meshLocation, float* distSq) const;
    bool GetGroundLocationInMesh(NavigationMeshID meshID, const Vec3& location,
        float vDownwardRange, float hRange, Vec3* meshLocation) const;

    virtual std::tuple<bool, NavigationMeshID, Vec3> RaycastWorld(const Vec3& segP0, const Vec3& segP1) const;

    virtual bool GetClosestPointInNavigationMesh(const NavigationAgentTypeID agentID, const Vec3& location, float vrange, float hrange, Vec3* meshLocation, float minIslandArea = 0.f) const;

    virtual bool IsLocationValidInNavigationMesh(const NavigationAgentTypeID agentID, const Vec3& location) const;
    virtual bool IsPointReachableFromPosition(const NavigationAgentTypeID agentID, const IEntity* pEntityToTestOffGridLinks, const Vec3& startLocation, const Vec3& endLocation) const;
    virtual bool IsLocationContainedWithinTriangleInNavigationMesh(const NavigationAgentTypeID agentID, const Vec3& location, float downRange, float upRange) const override;

    virtual size_t GetTriangleCenterLocationsInMesh(const NavigationMeshID meshID, const Vec3& location, const AABB& searchAABB, Vec3* centerLocations, size_t maxCenterLocationCount, float minIslandArea = 0.f) const override;

    virtual size_t GetTriangleBorders(const NavigationMeshID meshID, const AABB& aabb, Vec3* pBorders, size_t maxBorderCount, float minIslandArea = 0.f) const override;
    virtual size_t GetTriangleInfo(const NavigationMeshID meshID, const AABB& aabb, Vec3* centerLocations, uint32* islandids, size_t max_count, float minIslandArea = 0.f) const override;
    virtual MNM::GlobalIslandID GetGlobalIslandIdAtPosition(const NavigationAgentTypeID agentID, const Vec3& location);

    virtual bool ReadFromFile(const char* fileName, bool bAfterExporting);
    virtual bool SaveToFile(const char* fileName) const;

    virtual void RegisterListener(INavigationSystemListener* pListener, const char* name = NULL) {m_listenersList.Add(pListener, name); }
    virtual void UnRegisterListener(INavigationSystemListener* pListener) {m_listenersList.Remove(pListener); }

    virtual void RegisterUser(INavigationSystemUser* pUser, const char* name = NULL) { m_users.Add(pUser, name); }
    virtual void UnRegisterUser(INavigationSystemUser* pUser) { m_users.Remove(pUser); }

    virtual void RegisterArea(const char* shapeName);
    virtual void UnRegisterArea(const char* shapeName);
    virtual bool IsAreaPresent(const char* shapeName);
    virtual NavigationVolumeID GetAreaId(const char* shapeName) const;
    virtual void SetAreaId(const char* shapeName, NavigationVolumeID id);
    virtual void UpdateAreaNameForId(const NavigationVolumeID id, const char* newShapeName);

    virtual void StartWorldMonitoring();
    virtual void StopWorldMonitoring();

    virtual bool IsInUse() const;

    ///> Accessibility
    virtual void CalculateAccessibility();
    virtual void ComputeAccessibility(const Vec3& seedPos, NavigationAgentTypeID agentTypeId = NavigationAgentTypeID(0), float range = 0.f, EAccessbilityDir dir = AccessibilityAway);

    void ComputeIslands();
    void AddIslandConnectionsBetweenTriangles(const NavigationMeshID& meshID, const MNM::TriangleID startingTriangleID, const MNM::TriangleID endingTriangleID);
    void RemoveIslandsConnectionBetweenTriangles(const NavigationMeshID& meshID, const MNM::TriangleID startingTriangleID, const MNM::TriangleID endingTriangleID = 0);
    void RemoveAllIslandConnectionsForObject(const NavigationMeshID& meshID, const uint32 objectId);

    virtual MNM::TileID GetTileIdWhereLocationIsAtForMesh(NavigationMeshID meshID, const Vec3& location);
    virtual void GetTileBoundsForMesh(NavigationMeshID meshID, MNM::TileID tileID, AABB& bounds) const;
    virtual MNM::TriangleID GetTriangleIDWhereLocationIsAtForMesh(const NavigationAgentTypeID agentID, const Vec3& location);

    virtual const IOffMeshNavigationManager& GetIOffMeshNavigationManager() const { return m_offMeshNavigationManager; }
    virtual IOffMeshNavigationManager& GetIOffMeshNavigationManager() { return m_offMeshNavigationManager; }

    bool AgentTypeSupportSmartObjectUserClass(NavigationAgentTypeID agentTypeID, const char* smartObjectUserClass) const;
    uint16 GetAgentRadiusInVoxelUnits(NavigationAgentTypeID agentTypeID) const;
    uint16 GetAgentHeightInVoxelUnits(NavigationAgentTypeID agentTypeID) const;
    bool TryGetAgentRadiusData(const char* agentType, Vec3& voxelSize, uint16& radiusInVoxels) const override;

    inline const WorldMonitor* GetWorldMonitor() const
    {
        return &m_worldMonitor;
    }

    inline WorldMonitor* GetWorldMonitor()
    {
        return &m_worldMonitor;
    }

    inline const OffMeshNavigationManager* GetOffMeshNavigationManager() const
    {
        return &m_offMeshNavigationManager;
    }

    inline OffMeshNavigationManager* GetOffMeshNavigationManager()
    {
        return &m_offMeshNavigationManager;
    }

    inline const IslandConnectionsManager* GetIslandConnectionsManager() const
    {
        return &m_islandConnectionsManager;
    }

    inline IslandConnectionsManager* GetIslandConnectionsManager()
    {
        return &m_islandConnectionsManager;
    }

    struct TileTask
    {
        inline bool operator==(const TileTask& other) const
        {
            return (meshID == other.meshID) && (x == other.x) && (y == other.y) && (z == other.z);
        }

        inline bool operator<(const TileTask& other) const
        {
            if (meshID != other.meshID)
            {
                return meshID < other.meshID;
            }

            if (x != other.x)
            {
                return x < other.x;
            }

            if (y != other.y)
            {
                return y < other.y;
            }

            if (z != other.z)
            {
                return z < other.z;
            }

            return false;
        }

        NavigationMeshID meshID;

        uint16 x;
        uint16 y;
        uint16 z;
    };

    struct TileTaskHasher
    {
        typedef size_t      result_type;
        inline size_t operator()(const NavigationSystem::TileTask& tileTask) const
        {
            return static_cast<size_t>((uint32)tileTask.meshID ^ tileTask.x ^ tileTask.y ^ tileTask.z);
        }
    };

    struct TileTaskResult
    {
        TileTaskResult()
            : state(Running)
            , hashValue(0)
        {
        };

        TileTaskResult(TileTaskResult&& other)
            : tile(std::move(other.tile))
            , hashValue(other.hashValue)
            , meshID(std::move(other.meshID))
            , x(other.x)
            , y(other.y)
            , z(other.z)
            , volumeCopy(other.volumeCopy)
            , state(other.state)
            , next(other.next)
        {
            other.jobExecutor.WaitForCompletion();
        }

        TileTaskResult(const TileTaskResult&) = delete;
        TileTaskResult& operator=(const TileTaskResult&) = delete;
        TileTaskResult& operator=(TileTaskResult&&) = delete;

        enum State
        {
            Running = 0,
            Completed,
            NoChanges,
            Failed,
        };

        MNM::Tile tile;
        uint32 hashValue;

        NavigationMeshID meshID;

        uint16 x;
        uint16 y;
        uint16 z;
        uint16 volumeCopy;

        volatile uint16 state; // communicated over thread boundaries
        uint16 next; // next free

        AZ::LegacyJobExecutor jobExecutor;
    } _ALIGN(16);

private:

#if NAVIGATION_SYSTEM_PC_ONLY
    void UpdateMeshes(const float frameTime, const bool blocking, const bool multiThreaded, const bool bBackground);
    void SetupGenerator(NavigationMeshID meshID, const MNM::MeshGrid::Params& paramsGrid,
        uint16 x, uint16 y, uint16 z, MNM::TileGenerator::Params& params,
        const MNM::BoundingVolume* boundary, const MNM::BoundingVolume* exclusions,
        size_t exclusionCount);
    bool SpawnJob(TileTaskResult& result, NavigationMeshID meshID, const MNM::MeshGrid::Params& paramsGrid,
        uint16 x, uint16 y, uint16 z, bool mt);
    void CommitTile(TileTaskResult& result);

    /// store invalid exlusion volumes to help prevent log spamming
    AZStd::unordered_set<uint32_t> m_invalidExclusionVolumes; 
#endif

    void ResetAllNavigationSystemUsers();

    void WaitForAllNavigationSystemUsersCompleteTheirReadingAsynchronousTasks();
    void UpdateNavigationSystemUsersForSynchronousWritingOperations();
    void UpdateNavigationSystemUsersForSynchronousOrAsynchronousReadingOperations();
    void UpdateInternalNavigationSystemData(const bool blocking);
    void UpdateInternalSubsystems();

    void ComputeWorldAABB();
    void SetupTasks();
    void StopAllTasks();

    void UpdateAllListener(const ENavigationEvent event);
    bool TryGetAgentSettings(const char* agentType, const AgentType::Settings*& agentSettings) const;

#if MNM_USE_EXPORT_INFORMATION
    void ClearAllAccessibility(uint8 resetValue);
    void ComputeAccessibility(IAIObject* pIAIObject, NavigationAgentTypeID agentTypeId = NavigationAgentTypeID(0));
#endif

    typedef AZStd::unordered_set<TileTask, TileTaskHasher> TileTaskQueue;
    TileTaskQueue m_tileQueue;
    AZStd::recursive_mutex m_tileQueueMutex;

    typedef std::vector<uint16> RunningTasks;
    RunningTasks m_runningTasks;
    size_t m_maxRunningTaskCount;
    float m_cacheHitRate;
    float m_throughput;

    using TileTaskResults = AZStd::vector<TileTaskResult>;
    TileTaskResults m_results;
    uint16 m_free;
    WorkingState m_state;

    typedef id_map<uint32, NavigationMesh> Meshes;
    Meshes m_meshes;

    typedef id_map<uint32, MNM::BoundingVolume> Volumes;
    Volumes m_volumes;

    typedef std::vector<AgentType> AgentTypes;
    AgentTypes  m_agentTypes;
    uint32      m_configurationVersion;

    NavigationSystemDebugDraw         m_debugDraw;

    NavigationSystemBackgroundUpdate*  m_pEditorBackgroundUpdate;

    AABB m_worldAABB;
    WorldMonitor m_worldMonitor;

    OffMeshNavigationManager m_offMeshNavigationManager;
    IslandConnectionsManager m_islandConnectionsManager;

    struct VolumeDefCopy
    {
        VolumeDefCopy()
            : version(~0ul)
            , refCount(0)
            , meshID(0)
        {
        }

        size_t version;
        size_t refCount;

        NavigationMeshID meshID;

        MNM::BoundingVolume boundary;
        std::vector<MNM::BoundingVolume> exclusions;
    };

    std::vector<VolumeDefCopy> m_volumeDefCopy;

    string m_configName;

    typedef CListenerSet<INavigationSystemListener*> NavigationListeners;
    NavigationListeners m_listenersList;

    typedef CListenerSet<INavigationSystemUser*> NavigationSystemUsers;
    NavigationSystemUsers m_users;

    CVolumesManager m_volumesManager;
    bool m_isNavigationUpdatePaused;
};

namespace NavigationSystemUtils
{
    inline bool IsDynamicObjectPartOfTheMNMGenerationProcess(IPhysicalEntity* pPhysicalEntity)
    {
        if (pPhysicalEntity)
        {
            pe_status_dynamics dyn;
            if (pPhysicalEntity->GetStatus(&dyn) && (dyn.mass <= 1e-6f))
            {
                return true;
            }
        }

        return false;
    }
}


#endif // CRYINCLUDE_CRYAISYSTEM_NAVIGATION_NAVIGATIONSYSTEM_NAVIGATIONSYSTEM_H
