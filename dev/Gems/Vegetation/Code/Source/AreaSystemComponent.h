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

#pragma once

#include <AzCore/Component/Component.h>
#include <Vegetation/Ebuses/AreaSystemRequestBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/SystemConfigurationBus.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/parallel/thread.h>
#include <GradientSignal/Ebuses/SectorDataRequestBus.h>
#include <SurfaceData/SurfaceDataSystemNotificationBus.h>
#include <CrySystemBus.h>
#include <StatObjBus.h>
#include <ISystem.h>

namespace Vegetation
{
    struct DebugData;

    enum class SnapMode : AZ::u8
    {
        Corner = 0,
        Center
    };

    /**
    * The configuration for managing areas mostly the dimensions of the sectors
    */
    class AreaSystemConfig
        : public AZ::ComponentConfig
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaSystemConfig, AZ::SystemAllocator, 0);
        AZ_RTTI(AreaSystemConfig, "{14CCBE43-52DD-4F56-92A8-2BB011A0F7A2}", AZ::ComponentConfig);
        static void Reflect(AZ::ReflectContext* context);

        bool operator == (const AreaSystemConfig& other) const
        {
            return m_viewRectangleSize == other.m_viewRectangleSize
                && m_sectorDensity == other.m_sectorDensity
                && m_sectorSizeInMeters == other.m_sectorSizeInMeters
                && m_threadProcessingIntervalMs == other.m_threadProcessingIntervalMs
                && m_sectorSearchPadding == other.m_sectorSearchPadding
                && m_sectorPointSnapMode == other.m_sectorPointSnapMode;
        }

        int m_viewRectangleSize = 13;
        int m_sectorDensity = 20;
        int m_sectorSizeInMeters = 16;
        int m_threadProcessingIntervalMs = 500;
        int m_sectorSearchPadding = 0;
        SnapMode m_sectorPointSnapMode = SnapMode::Corner;
    private:
        static const int s_maxViewRectangleSize;
        static const int s_maxSectorDensity;
        static const int s_maxSectorSizeInMeters;

        static const int s_maxInstancesPerMeter;
        static const int64_t s_maxVegetationInstances;

        AZ::Outcome<void, AZStd::string> ValidateViewArea(void* newValue, const AZ::Uuid& valueType);
        AZ::Outcome<void, AZStd::string> ValidateSectorDensity(void* newValue, const AZ::Uuid& valueType);
        AZ::Outcome<void, AZStd::string> ValidateSectorSize(void* newValue, const AZ::Uuid& valueType);
    };

    /**
    * Manages an sectors and claims while the camera scrolls through the 3D world
    */
    class AreaSystemComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , private AreaSystemRequestBus::Handler
        , private GradientSignal::SectorDataRequestBus::Handler
        , private SystemConfigurationRequestBus::Handler
        , private InstanceStatObjEventBus::Handler
        , private CrySystemEventBus::Handler
        , private ISystemEventListener
        , private SurfaceData::SurfaceDataSystemNotificationBus::Handler
    {
    public:
        friend class EditorAreaSystemComponent;
        AZ_COMPONENT(AreaSystemComponent, "{7CE8E791-6BC6-4C88-8727-A476DE00F9A1}");
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services);
        static void Reflect(AZ::ReflectContext* context);

        AreaSystemComponent(const AreaSystemConfig& configuration);
        AreaSystemComponent() = default;
        ~AreaSystemComponent() = default;

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        bool ReadInConfig(const AZ::ComponentConfig* baseConfig) override;
        bool WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const override;

        //////////////////////////////////////////////////////////////////////////
        // AreaSystemRequestBus
        void RegisterArea(AZ::EntityId areaId) override;
        void UnregisterArea(AZ::EntityId areaId) override;
        void RefreshArea(AZ::EntityId areaId) override;
        void RefreshAllAreas() override;
        void ClearAllAreas() override;
        void MuteArea(AZ::EntityId areaId) override;
        void UnmuteArea(AZ::EntityId areaId) override;
        void EnumerateInstancesInAabb(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const override;

        //////////////////////////////////////////////////////////////////////////
        // GradientSignal::SectorDataRequestBus
        void GetPointsPerMeter(float& value) const override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        void ApplyPendingConfigChanges();

        //////////////////////////////////////////////////////////////////////////
        // SystemConfigurationRequestBus
        void UpdateSystemConfig(const AZ::ComponentConfig* config) override;
        void GetSystemConfig(AZ::ComponentConfig* config) const override;

        //////////////////////////////////////////////////////////////////////////
        // SurfaceData::SurfaceDataSystemNotificationBus
        void OnSurfaceChanged(const AZ::EntityId& entityId, const AZ::Aabb& bounds) override;

        //////////////////////////////////////////////////////////////////////////
        // InstanceStatObjEventBus
        void ReleaseData() override;

        ////////////////////////////////////////////////////////////////////////////
        // CrySystemEvents
        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override;
        void OnCrySystemShutdown(ISystem& system) override;
        void OnCryEditorBeginLevelExport() override;
        void OnCryEditorEndLevelExport(bool /*success*/) override;

        //////////////////////////////////////////////////////////////////////////
        // ISystemEventListener
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

    private:
        void ReleaseWithoutCleanup();
        void UpdateVegetationAreas();

        void ProcessVegetationThreadTasks();
        void UpdateActiveVegetationAreas();
        void CalculateViewRect();

        void QueueVegetationTask(AZStd::function<void()> func);

        AreaSystemConfig m_configuration;

        using ClaimContainer = AZStd::unordered_map<ClaimHandle, InstanceData>;

        using SectorId = AZStd::pair<int, int>;

        struct SectorInfo
        {
            AZ_CLASS_ALLOCATOR(SectorInfo, AZ::SystemAllocator, 0);

            SectorId m_id = {};
            AZ::Aabb m_bounds = {};
            int m_worldX = {};
            int m_worldY = {};
            //! Keeps track of points that have been claimed.  This is not cleared at the start of an update pass
            ClaimContainer m_claimedWorldPoints;
            ClaimContext m_baseContext;
        };

        struct VegetationAreaInfo
        {
            AZ_CLASS_ALLOCATOR(VegetationAreaInfo, AZ::SystemAllocator, 0);

            AZ::EntityId m_id;
            AZ::Aabb m_bounds = {};
            float m_priority = {};
        };
        using VegetationAreaMap = AZStd::unordered_map<AZ::EntityId, VegetationAreaInfo>;
        using VegetationAreaSet = AZStd::unordered_set<AZ::EntityId>;
        using VegetationAreaVector = AZStd::vector<VegetationAreaInfo>;

        //! Get sector id by world coordinates.
        SectorId GetSectorId(const AZ::Vector3& worldPos) const;

        //! Creates a new sector
        SectorInfo* CreateSector(const SectorId& sectorId);
        void UpdateSectorBounds(SectorInfo& sectorInfo);
        void UpdateSectorPoints(SectorInfo& sectorInfo);
        void UpdateSectorCallbacks(SectorInfo& sectorInfo);

        //! Get sector by 2d veg map coordinates.
        const SectorInfo* GetSector(const SectorId& sectorId) const;
        SectorInfo* GetSector(const SectorId& sectorId);

        void ReleaseUnregisteredClaims(SectorInfo& sectorInfo);
        void ReleaseUnusedClaims(SectorInfo& sectorInfo);
        void FillSector(SectorInfo& sectorInfo);
        void EmptySector(SectorInfo& sectorInfo);
        void DeleteSector(const SectorId& sectorId);
        void ClearSectors();

        AZ::Aabb GetBubbleBounds() const;

        // claiming logic
        void CreateClaim(SectorInfo& sectorInfo, const ClaimHandle handle, const InstanceData& instanceData);
        ClaimHandle CreateClaimHandle(const SectorInfo& sectorInfo, uint32_t index) const;

        mutable AZStd::recursive_mutex m_vegetationThreadMutex;
        AZStd::atomic_bool m_vegetationThreadRunning{ false };
        float m_vegetationThreadTaskTimer = 0.0f;
        mutable AZStd::recursive_mutex m_vegetationThreadTaskMutex;
        using VegetationThreadTaskList = AZStd::list<AZStd::function<void()>>;
        VegetationThreadTaskList m_vegetationThreadTasks;

        //! 2D Array rolling window of sectors that store vegetation objects.
        using SectorRollingWindow = AZStd::unordered_map<SectorId, SectorInfo>;
        mutable AZStd::recursive_mutex m_sectorRollingWindowMutex;
        SectorRollingWindow m_sectorRollingWindow;

        //! set of dirty area bounds to refresh
        struct HashAabb
        {
            typedef AZ::Aabb argument_type;
            typedef AZStd::size_t result_type;
            inline result_type operator()(argument_type value) const
            {
                size_t result = 0;
                AZStd::hash_combine(result, value.GetMin().GetX());
                AZStd::hash_combine(result, value.GetMin().GetY());
                AZStd::hash_combine(result, value.GetMin().GetZ());
                AZStd::hash_combine(result, value.GetMax().GetX());
                AZStd::hash_combine(result, value.GetMax().GetY());
                AZStd::hash_combine(result, value.GetMax().GetZ());
                return result;
            }
        };

        using DirtyAreaBoundsSet = AZStd::unordered_set<AZ::Aabb, HashAabb>;
        DirtyAreaBoundsSet m_dirtyAreaBoundsSet;
        DirtyAreaBoundsSet m_dirtySectorBoundsSet;

        VegetationAreaMap m_globalVegetationAreaMap;
        VegetationAreaSet m_ignoredVegetationAreaSet;

        //! List of areas that have been unregistered and need to have their claims released
        VegetationAreaMap m_unregisteredVegetationAreaSet;

        //! world to sector scaling ratio.
        float m_worldToSector = 0.0f;

        // helper struct to manage the "scrolling view rectangle"
        struct ViewRect
        {
            int m_x = 0;
            int m_y = 0;
            int m_width = 0;
            int m_height = 0;

            ViewRect() = default;

            ViewRect(int inX, int inY, int inW, int inH)
                : m_x(inX)
                , m_y(inY)
                , m_width(inW)
                , m_height(inH)
            {
            }

            bool IsInside(int inX, int inY) const;
            ViewRect Overlap(const ViewRect& b) const;
            bool operator !=(const ViewRect& b);
            bool operator ==(const ViewRect& b);
        };

        ViewRect m_prevViewRect = {};
        ViewRect m_currViewRect = {};

        //containers kept around for reserved space
        bool m_activeAreasDirty = true;
        VegetationAreaVector m_activeAreas;
        VegetationAreaVector m_activeAreasInBubble;
        ClaimContext m_activeContext;
        EntityIdStack m_activeStackIds;

        //this stores the previous state of the current sector while filling
        ClaimContainer m_claimedWorldPointsBeforeFill;

        //! Cached pointer to the debug data
        DebugData* m_debugData = nullptr;

        ISystem* m_system = nullptr;

        bool m_configDirty = false;
        AreaSystemConfig m_pendingConfigUpdate;
    };
}
