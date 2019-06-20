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

#include "Vegetation_precompiled.h"
#include "AreaSystemComponent.h"

#include <Vegetation/Ebuses/AreaNotificationBus.h>
#include <Vegetation/Ebuses/AreaRequestBus.h>
#include <Vegetation/Ebuses/InstanceSystemRequestBus.h>
#include <Vegetation/Ebuses/AreaInfoBus.h>
#include <Vegetation/Ebuses/DebugNotificationBus.h>
#include <Vegetation/Ebuses/DebugSystemDataBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Utility/SurfaceDataUtility.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/utils.h>

#include <I3DEngine.h>
#include <ISystem.h>

namespace Vegetation
{
    namespace AreaSystemUtil
    {
        template <typename T>
        void hash_combine_64(uint64_t& seed, T const& v)
        {
            AZStd::hash<T> hasher;
            seed ^= hasher(v) + 0x9e3779b97f4a7c13LL + (seed << 12) + (seed >> 4);
        }

        static bool UpdateVersion(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 4)
            {
                classElement.RemoveElementByName(AZ_CRC("ThreadSleepTimeMs", 0x9e86f79d));
            }
            return true;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // ViewRect

    bool AreaSystemComponent::ViewRect::IsInside(int inX, int inY) const
    {
        return inX >= m_x && inX < m_x + m_width && inY >= m_y && inY < m_y + m_height;
    }

    AreaSystemComponent::ViewRect AreaSystemComponent::ViewRect::Overlap(const ViewRect& b) const
    {
        ViewRect o;
        o.m_x = m_x > b.m_x ? m_x : b.m_x;
        o.m_y = m_y > b.m_y ? m_y : b.m_y;
        o.m_width = m_x + m_width > b.m_x + b.m_width ? b.m_x + b.m_width : m_x + m_width;
        o.m_height = m_y + m_height > b.m_y + b.m_height ? b.m_y + b.m_height : m_y + m_height;
        o.m_width -= o.m_x;
        o.m_height -= o.m_y;
        return o;
    }

    bool AreaSystemComponent::ViewRect::operator==(const ViewRect& b)
    {
        return m_x == b.m_x && m_y == b.m_y && m_width == b.m_width && m_height == b.m_height;
    }

    bool AreaSystemComponent::ViewRect::operator!=(const ViewRect& b)
    {
        return m_x != b.m_x || m_y != b.m_y || m_width != b.m_width || m_height != b.m_height;
    }

    //////////////////////////////////////////////////////////////////////////
    // AreaSystemConfig

    // These limitations are somewhat arbitrary.  It's possible to select combinations of larger values than these that will work successfully.
    // However, these values are also large enough that going beyond them is extremely likely to cause problems.
    const int AreaSystemConfig::s_maxViewRectangleSize = 128;
    const int AreaSystemConfig::s_maxSectorDensity = 64;
    const int AreaSystemConfig::s_maxSectorSizeInMeters = 1024;
    const int64_t AreaSystemConfig::s_maxVegetationInstances = 2 * 1024 * 1024;
    const int AreaSystemConfig::s_maxInstancesPerMeter = 16;

    void AreaSystemConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaSystemConfig, AZ::ComponentConfig>()
                ->Version(4, &AreaSystemUtil::UpdateVersion)
                ->Field("ViewRectangleSize", &AreaSystemConfig::m_viewRectangleSize)
                ->Field("SectorDensity", &AreaSystemConfig::m_sectorDensity)
                ->Field("SectorSizeInMeters", &AreaSystemConfig::m_sectorSizeInMeters)
                ->Field("ThreadProcessingIntervalMs", &AreaSystemConfig::m_threadProcessingIntervalMs)
                ->Field("SectorSearchPadding", &AreaSystemConfig::m_sectorSearchPadding)
                ->Field("SectorPointSnapMode", &AreaSystemConfig::m_sectorPointSnapMode)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<AreaSystemConfig>(
                    "Vegetation Area System Config", "Handles the placement and removal of vegetation instance based on the vegetation area component rules")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_viewRectangleSize, "View Area Grid Size", "The number of sectors (per-side) of a managed grid in a scrolling view centered around the camera.")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AreaSystemConfig::ValidateViewArea)
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, s_maxViewRectangleSize)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_sectorDensity, "Sector Point Density", "The number of equally-spaced vegetation instance grid placement points (per-side) within a sector")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AreaSystemConfig::ValidateSectorDensity)
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, s_maxSectorDensity)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_sectorSizeInMeters, "Sector Size In Meters", "The size in meters (per-side) of each sector.")
                    ->Attribute(AZ::Edit::Attributes::ChangeValidate, &AreaSystemConfig::ValidateSectorSize)
                    ->Attribute(AZ::Edit::Attributes::Min, 1)
                    ->Attribute(AZ::Edit::Attributes::Max, s_maxSectorSizeInMeters)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_threadProcessingIntervalMs, "Thread Processing Interval", "The delay (in milliseconds) between processing queued thread tasks.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 5000)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &AreaSystemConfig::m_sectorSearchPadding, "Sector Search Padding", "Increases the search radius for surrounding sectors when enumerating instances.")
                    ->Attribute(AZ::Edit::Attributes::Min, 0)
                    ->Attribute(AZ::Edit::Attributes::Max, 2)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AreaSystemConfig::m_sectorPointSnapMode, "Sector Point Snap Mode", "Controls whether vegetation placement points are located at the corner or the center of the cell.")
                    ->EnumAttribute(SnapMode::Corner, "Corner")
                    ->EnumAttribute(SnapMode::Center, "Center")
                    ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AreaSystemConfig>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Constructor()
                ->Property("viewRectangleSize", BehaviorValueProperty(&AreaSystemConfig::m_viewRectangleSize))
                ->Property("sectorDensity", BehaviorValueProperty(&AreaSystemConfig::m_sectorDensity))
                ->Property("sectorSizeInMeters", BehaviorValueProperty(&AreaSystemConfig::m_sectorSizeInMeters))
                ->Property("threadProcessingIntervalMs", BehaviorValueProperty(&AreaSystemConfig::m_threadProcessingIntervalMs))
                ->Property("sectorPointSnapMode",
                    [](AreaSystemConfig* config) { return static_cast<AZ::u8>(config->m_sectorPointSnapMode); },
                    [](AreaSystemConfig* config, const AZ::u8& i) { config->m_sectorPointSnapMode = static_cast<SnapMode>(i); })
                ;
        }
    }

    AZ::Outcome<void, AZStd::string> AreaSystemConfig::ValidateViewArea(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<int>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Unexpectedly received a non-int type for the View Area Grid Size!"));
        }

        int viewRectangleSize = *static_cast<int*>(newValue);
        const int instancesPerSector = m_sectorDensity * m_sectorDensity;
        const int totalSectors = viewRectangleSize * viewRectangleSize;

        int64_t totalInstances = instancesPerSector * totalSectors;

        if (totalInstances > s_maxVegetationInstances)
        {
            return AZ::Failure(AZStd::string::format("The combination of View Area Grid Size and Sector Point Density will create %lld instances.  Only a max of %lld instances is allowed.", 
                                             totalInstances, s_maxVegetationInstances));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> AreaSystemConfig::ValidateSectorDensity(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<int>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Unexpectedly received a non-int type for the Sector Point Density!"));
        }

        int sectorDensity = *static_cast<int*>(newValue);
        const int instancesPerSector = sectorDensity * sectorDensity;
        const int totalSectors = m_viewRectangleSize * m_viewRectangleSize;

        int64_t totalInstances = instancesPerSector * totalSectors;

        if (totalInstances >= s_maxVegetationInstances)
        {
            return AZ::Failure(AZStd::string::format("The combination of View Area Grid Size and Sector Point Density will create %lld instances.  Only a max of %lld instances is allowed.",
                totalInstances, s_maxVegetationInstances));
        }

        const float instancesPerMeter = static_cast<float>(sectorDensity) / static_cast<float>(m_sectorSizeInMeters);
        if (instancesPerMeter > s_maxInstancesPerMeter)
        {
            return AZ::Failure(AZStd::string::format("The combination of Sector Point Density and Sector Size in Meters will create %.1f instances per meter.  Only a max of %d instances per meter is allowed.",
                                                     instancesPerMeter, s_maxInstancesPerMeter));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> AreaSystemConfig::ValidateSectorSize(void* newValue, const AZ::Uuid& valueType)
    {
        if (azrtti_typeid<int>() != valueType)
        {
            AZ_Assert(false, "Unexpected value type");
            return AZ::Failure(AZStd::string("Unexpectedly received a non-int type for the Sector Size In Meters!"));
        }

        int sectorSizeInMeters = *static_cast<int*>(newValue);

        const float instancesPerMeter = static_cast<float>(m_sectorDensity) / static_cast<float>(sectorSizeInMeters);
        if (instancesPerMeter > s_maxInstancesPerMeter)
        {
            return AZ::Failure(AZStd::string::format("The combination of Sector Point Density and Sector Size in Meters will create %.1f instances per meter.  Only a max of %d instances per meter is allowed.",
                                                     instancesPerMeter, s_maxInstancesPerMeter));
        }

        return AZ::Success();
    }


    //////////////////////////////////////////////////////////////////////////
    // AreaSystemComponent

    void AreaSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaSystemService", 0x36da2b62));
    }

    void AreaSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationAreaSystemService", 0x36da2b62));
    }

    void AreaSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
    {
        services.push_back(AZ_CRC("VegetationDebugSystemService", 0x8cac3d67));
        services.push_back(AZ_CRC("VegetationInstanceSystemService", 0x823a6007));
        services.push_back(AZ_CRC("SurfaceDataSystemService", 0x1d44d25f));
    }

    void AreaSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AreaSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("Configuration", &AreaSystemComponent::m_configuration)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<AreaSystemComponent>("Vegetation Area System", "Manages registration and processing of vegetation area entities")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Vegetation")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/console/lumberyard/vegetation/vegetation-system-area")
                    ->DataElement(0, &AreaSystemComponent::m_configuration, "Configuration", "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    AreaSystemComponent::AreaSystemComponent(const AreaSystemConfig& configuration)
        : m_configuration(configuration)
    {
    }

    void AreaSystemComponent::Init()
    {
    }

    void AreaSystemComponent::Activate()
    {
        //wait for the vegetation thread work to complete
        AZStd::lock_guard<decltype(m_vegetationThreadMutex)> lockTasks(m_vegetationThreadMutex);
        m_vegetationThreadRunning = false;

        m_system = GetISystem();
        AZ::TickBus::Handler::BusConnect();
        AreaSystemRequestBus::Handler::BusConnect();
        GradientSignal::SectorDataRequestBus::Handler::BusConnect();
        SystemConfigurationRequestBus::Handler::BusConnect();
        InstanceStatObjEventBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusConnect();

        m_worldToSector = 1.0f / m_configuration.m_sectorSizeInMeters;
        m_activeAreasDirty = true;

        VEG_PROFILE_METHOD(DebugSystemDataBus::BroadcastResult(m_debugData, &DebugSystemDataBus::Events::GetDebugData));
    }

    void AreaSystemComponent::Deactivate()
    {
        //wait for the vegetation thread work to complete
        AZStd::lock_guard<decltype(m_vegetationThreadMutex)> lockTasks(m_vegetationThreadMutex);
        m_vegetationThreadRunning = false;

        AZ::TickBus::Handler::BusDisconnect();
        AreaSystemRequestBus::Handler::BusDisconnect();
        GradientSignal::SectorDataRequestBus::Handler::BusDisconnect();
        SystemConfigurationRequestBus::Handler::BusDisconnect();
        InstanceStatObjEventBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
        SurfaceData::SurfaceDataSystemNotificationBus::Handler::BusDisconnect();

        ClearSectors();
        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::DestroyAllInstances);
        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::Cleanup);

        if (m_system)
        {
            m_system->GetISystemEventDispatcher()->RemoveListener(this);
            m_system = nullptr;
        }
    }

    bool AreaSystemComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const AreaSystemConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool AreaSystemComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto config = azrtti_cast<AreaSystemConfig*>(outBaseConfig))
        {
            if (m_configDirty)
            {
                *config = m_pendingConfigUpdate;
            }
            else
            {
                *config = m_configuration;
            }
            return true;
        }
        return false;
    }

    void AreaSystemComponent::RegisterArea(AZ::EntityId areaId)
    {
        QueueVegetationTask([this, areaId]() {
            auto& area = m_globalVegetationAreaMap[areaId];
            area.m_id = areaId;
            area.m_priority = {};
            area.m_bounds = AZ::Aabb::CreateNull();

            AreaInfoBus::EventResult(area.m_priority, area.m_id, &AreaInfoBus::Events::GetPriority);
            AreaInfoBus::EventResult(area.m_bounds, area.m_id, &AreaInfoBus::Events::GetEncompassingAabb);
            AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaRegistered);
            m_dirtyAreaBoundsSet.insert(area.m_bounds);
            m_activeAreasDirty = true;
        });
    }

    void AreaSystemComponent::UnregisterArea(AZ::EntityId areaId)
    {
        QueueVegetationTask([this, areaId]() {
            auto itArea = m_globalVegetationAreaMap.find(areaId);
            if (itArea != m_globalVegetationAreaMap.end())
            {
                const auto& area = itArea->second;
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaUnregistered);
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);

                m_unregisteredVegetationAreaSet[area.m_id] = area;
                m_dirtyAreaBoundsSet.insert(area.m_bounds);
                m_globalVegetationAreaMap.erase(itArea);
                m_activeAreasDirty = true;
            }
        });
    }

    void AreaSystemComponent::RefreshArea(AZ::EntityId areaId)
    {
        QueueVegetationTask([this, areaId]() {
            auto itArea = m_globalVegetationAreaMap.find(areaId);
            if (itArea != m_globalVegetationAreaMap.end())
            {
                auto& area = itArea->second;
                m_dirtyAreaBoundsSet.insert(area.m_bounds);
                AreaInfoBus::EventResult(area.m_priority, area.m_id, &AreaInfoBus::Events::GetPriority);
                AreaInfoBus::EventResult(area.m_bounds, area.m_id, &AreaInfoBus::Events::GetEncompassingAabb);
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaRefreshed);
                m_dirtyAreaBoundsSet.insert(area.m_bounds);
                m_activeAreasDirty = true;
            }
        });
    }

    void AreaSystemComponent::RefreshAllAreas()
    {
        QueueVegetationTask([this]() {
            for (auto& entry : m_globalVegetationAreaMap)
            {
                auto& area = entry.second;
                AreaInfoBus::EventResult(area.m_priority, area.m_id, &AreaInfoBus::Events::GetPriority);
                AreaInfoBus::EventResult(area.m_bounds, area.m_id, &AreaInfoBus::Events::GetEncompassingAabb);
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaRefreshed);
            }
            m_dirtyAreaBoundsSet.insert(AZ::Aabb::CreateNull());
            m_dirtySectorBoundsSet.insert(AZ::Aabb::CreateNull());
        });
    }

    void AreaSystemComponent::ClearAllAreas()
    {
        QueueVegetationTask([this]() {
            ClearSectors();
        });
    }

    void AreaSystemComponent::MuteArea(AZ::EntityId areaId)
    {
        QueueVegetationTask([this, areaId]() {
            m_ignoredVegetationAreaSet.insert(areaId);
            m_activeAreasDirty = true;
        });
    }

    void AreaSystemComponent::UnmuteArea(AZ::EntityId areaId)
    {
        QueueVegetationTask([this, areaId]() {
            m_ignoredVegetationAreaSet.erase(areaId);
            m_activeAreasDirty = true;
        });
    }

    void AreaSystemComponent::EnumerateInstancesInAabb(const AZ::Aabb& bounds, AreaSystemEnumerateCallback callback) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        if (!bounds.IsValid())
        {
            return;
        }

        const SectorId minSector = GetSectorId(bounds.GetMin());
        const int minX = minSector.first - m_configuration.m_sectorSearchPadding;
        const int minY = minSector.second - m_configuration.m_sectorSearchPadding;
        const SectorId maxSector = GetSectorId(bounds.GetMax());
        const int maxX = maxSector.first + m_configuration.m_sectorSearchPadding;
        const int maxY = maxSector.second + m_configuration.m_sectorSearchPadding;

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        for (int currX = minX; currX <= maxX; ++currX)
        {
            for (int currY = minY; currY <= maxY; ++currY)
            {
                const SectorInfo* sectorInfo = GetSector(SectorId(currX, currY));
                if (sectorInfo) // manual sector id's can be outside the active area
                {
                    for (const auto& claimPair : sectorInfo->m_claimedWorldPoints)
                    {
                        const auto& instanceData = claimPair.second;
                        if (callback(instanceData) != AreaSystemEnumerateCallbackResult::KeepEnumerating)
                        {
                            return;
                        }
                    }
                }
            }
        }
    }

    void AreaSystemComponent::GetPointsPerMeter(float& value) const
    {
        if (m_configuration.m_sectorDensity <= 0 || m_configuration.m_sectorSizeInMeters <= 0.0f)
        {
            value = 1.0f;
        }
        else
        {
            value = static_cast<float>(m_configuration.m_sectorDensity) / m_configuration.m_sectorSizeInMeters;
        }
    }

    void AreaSystemComponent::QueueVegetationTask(AZStd::function<void()> func)
    {
        AZStd::lock_guard<decltype(m_vegetationThreadTaskMutex)> lock(m_vegetationThreadTaskMutex);
        m_vegetationThreadTasks.push_back(func);

        if (m_debugData)
        {
            m_debugData->m_areaTaskQueueCount.store(m_vegetationThreadTasks.size(), AZStd::memory_order_relaxed);
        }
    }

    void AreaSystemComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        if (m_configuration.m_sectorSizeInMeters < 0)
        {
            m_configuration.m_sectorSizeInMeters = 1;
        }
        m_worldToSector = 1.0f / m_configuration.m_sectorSizeInMeters;
        m_vegetationThreadTaskTimer -= deltaTime;

        if (!m_vegetationThreadRunning)
        {
            ApplyPendingConfigChanges();

            CalculateViewRect();

            bool processTasks = false;
            if (m_vegetationThreadTaskTimer <= 0.0f)
            {
                m_vegetationThreadTaskTimer = m_configuration.m_threadProcessingIntervalMs * 0.001f;

                // run the vegetation thread if we are done processing the current batch and a refresh is needed.
                AZStd::lock_guard<decltype(m_vegetationThreadTaskMutex)> lock(m_vegetationThreadTaskMutex);
                processTasks = !m_vegetationThreadTasks.empty();
            }

            if (m_prevViewRect != m_currViewRect || processTasks)
            {
                //create a job to process vegetation areas, tasks, sectors in the background
                m_vegetationThreadRunning = true;
                auto job = AZ::CreateJobFunction([this]() {
                    UpdateVegetationAreas();
                    m_vegetationThreadRunning = false;
                }, true);
                job->Start();
            }
        }
    }

    void AreaSystemComponent::ApplyPendingConfigChanges()
    {
        if (m_configDirty)
        {
            ReleaseWithoutCleanup();

            if (m_configuration.m_threadProcessingIntervalMs != m_pendingConfigUpdate.m_threadProcessingIntervalMs)
            {
                m_vegetationThreadTaskTimer = 0.0f;
            }

            ReadInConfig(&m_pendingConfigUpdate);
            m_worldToSector = 1.0f / m_configuration.m_sectorSizeInMeters;
            RefreshAllAreas();

            GradientSignal::SectorDataNotificationBus::Broadcast(&GradientSignal::SectorDataNotificationBus::Events::OnSectorDataConfigurationUpdated);

            m_configDirty = false;
        }
    }

    void AreaSystemComponent::ProcessVegetationThreadTasks()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        VegetationThreadTaskList tasks;
        {
            AZStd::lock_guard<decltype(m_vegetationThreadTaskMutex)> lock(m_vegetationThreadTaskMutex);
            AZStd::swap(tasks, m_vegetationThreadTasks);

            if (m_debugData)
            {
                m_debugData->m_areaTaskQueueCount.store(m_vegetationThreadTasks.size(), AZStd::memory_order_relaxed);
                m_debugData->m_areaTaskActiveCount.store(tasks.size(), AZStd::memory_order_relaxed);
            }
        }

        for (const auto& task : tasks)
        {
            task();

            if (m_debugData)
            {
                m_debugData->m_areaTaskActiveCount.fetch_sub(1, AZStd::memory_order_relaxed);
            }
        }
    }

    void AreaSystemComponent::UpdateActiveVegetationAreas()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        //build a priority sorted list of all active areas
        if (m_activeAreasDirty)
        {
            m_activeAreasDirty = false;
            m_activeAreas.clear();
            m_activeAreas.reserve(m_globalVegetationAreaMap.size());
            for (const auto& areaPair : m_globalVegetationAreaMap)
            {
                const auto& area = areaPair.second;

                //if this is an area being ignored due to a parent area blender, skip it
                if (m_ignoredVegetationAreaSet.find(area.m_id) != m_ignoredVegetationAreaSet.end())
                {
                    continue;
                }

                //do any per area setup or checks since the state of areas and entities with the system has changed
                bool prepared = false;
                m_activeStackIds.clear();
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaConnect);
                AreaRequestBus::EventResult(prepared, area.m_id, &AreaRequestBus::Events::PrepareToClaim, m_activeStackIds);
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);
                if (!prepared)
                {
                    //if PrepareToClaim failed, reject this area
                    continue;
                }

                m_activeAreas.push_back(area);
            }

            AZStd::sort(m_activeAreas.begin(), m_activeAreas.end(), [](const auto& lhs, const auto& rhs)
            {
                return lhs.m_priority > rhs.m_priority;
            });
        }

        //further reduce set of active areas to only include ones that intersect the bubble
        AZ::Aabb bubbleBounds = GetBubbleBounds();
        m_activeAreasInBubble = m_activeAreas;
        m_activeAreasInBubble.erase(
            AZStd::remove_if(
                m_activeAreasInBubble.begin(),
                m_activeAreasInBubble.end(),
                [bubbleBounds](const auto& area) {return area.m_bounds.IsValid() && !area.m_bounds.Overlaps(bubbleBounds);}),
            m_activeAreasInBubble.end());
    }

    void AreaSystemComponent::UpdateSystemConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const AreaSystemConfig*>(baseConfig))
        {
            if ((!m_configDirty && m_configuration == *config) || (m_configDirty && m_pendingConfigUpdate == *config))
            {
                return;
            }

            m_configDirty = true;
            m_pendingConfigUpdate = *config;
        }
    }

    void AreaSystemComponent::GetSystemConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        WriteOutConfig(outBaseConfig);
    }

    void AreaSystemComponent::OnSurfaceChanged(const AZ::EntityId& /*entityId*/, const AZ::Aabb& bounds)
    {
        QueueVegetationTask([this, bounds]() {
            m_dirtyAreaBoundsSet.insert(bounds);
            m_dirtySectorBoundsSet.insert(bounds);
        });
    }

    void AreaSystemComponent::UpdateVegetationAreas()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);
        AZStd::lock_guard<decltype(m_vegetationThreadMutex)> lockTasks(m_vegetationThreadMutex);

        // only process the sectors if the allocation has happened
        if (m_worldToSector <= 0.0f)
        {
            return;
        }

        ProcessVegetationThreadTasks();
        UpdateActiveVegetationAreas();

        AZStd::unordered_set<SectorInfo*> sectorsToFill;
        AZStd::unordered_set<SectorInfo*> sectorsToDelete;
        AZStd::unordered_set<SectorInfo*> sectorsToRebuildSurfaceCache;

        //determine all sectors outside of current view and mark them for deletion
        {
            AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
            for (auto& sectorPair : m_sectorRollingWindow)
            {
                auto& sectorInfo = sectorPair.second;
                if (!m_currViewRect.IsInside(sectorInfo.m_worldX, sectorInfo.m_worldY))
                {
                    sectorsToDelete.insert(&sectorInfo);
                }
            }
        }

        //destroy all sectors marked for deletion
        for (auto sectorInfo : sectorsToDelete)
        {
            DeleteSector(sectorInfo->m_id);
        }

        //determine if any remaining sectors need to be updated
        {
            AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
            for (auto& sectorPair : m_sectorRollingWindow)
            {
                //for all areas that have been marked dirty, repopulate intersecting sectors
                for (const auto& aabb : m_dirtyAreaBoundsSet)
                {
                    auto& sectorInfo = sectorPair.second;
                    if (!aabb.IsValid() || aabb.Overlaps(sectorInfo.m_bounds))
                    {
                        sectorsToFill.insert(&sectorInfo);
                        break;
                    }
                }

                //rebuild dirty sector data rather than destroy all sectors
                for (const auto& aabb : m_dirtySectorBoundsSet)
                {
                    auto& sectorInfo = sectorPair.second;
                    if (!aabb.IsValid() || aabb.Overlaps(sectorInfo.m_bounds))
                    {
                        sectorsToRebuildSurfaceCache.insert(&sectorInfo);
                        sectorsToFill.insert(&sectorInfo);
                        break;
                    }
                }
            }
        }
        m_dirtyAreaBoundsSet.clear();
        m_dirtySectorBoundsSet.clear();

        for (auto sectorInfo : sectorsToRebuildSurfaceCache)
        {
            UpdateSectorPoints(*sectorInfo);
        }

        //(re)create any sectors that are in the current view
        for (int y = m_currViewRect.m_y; y < m_currViewRect.m_y + m_currViewRect.m_height; ++y)
        {
            for (int x = m_currViewRect.m_x; x < m_currViewRect.m_x + m_currViewRect.m_width; ++x)
            {
                SectorId sectorId(x, y);
                if (!GetSector(sectorId))
                {
                    sectorsToFill.insert(CreateSector(sectorId));
                }
            }
        }

        //sort sectors to fill based on distance from the center of the bubble
        AZ::Aabb bubbleBounds = GetBubbleBounds();

        AZStd::vector<SectorInfo*> sectorsToFillSorted(sectorsToFill.begin(), sectorsToFill.end());
        if (bubbleBounds.IsValid())
        {
            AZStd::sort(sectorsToFillSorted.begin(), sectorsToFillSorted.end(), [bubbleBounds](const auto& lhs, const auto& rhs) {
                return
                    lhs->m_bounds.GetCenter().GetDistanceSq(bubbleBounds.GetCenter()) <
                    rhs->m_bounds.GetCenter().GetDistanceSq(bubbleBounds.GetCenter());
            });
        }

        for (auto sectorInfo : sectorsToFillSorted)
        {
            FillSector(*sectorInfo);
        }

        m_unregisteredVegetationAreaSet.clear();
    }

    void AreaSystemComponent::CalculateViewRect()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        auto engine = m_system ? m_system->GetI3DEngine() : nullptr;
        if (engine)
        {
            Vec3 pos = engine->GetRenderingCamera().GetPosition();
            const int viewSize = m_configuration.m_viewRectangleSize;
            int halfViewSize = viewSize >> 1;
            pos.x -= halfViewSize * m_configuration.m_sectorSizeInMeters;
            pos.y -= halfViewSize * m_configuration.m_sectorSizeInMeters;

            m_prevViewRect = m_currViewRect;
            m_currViewRect.m_x = (int)(pos.x * m_worldToSector);
            m_currViewRect.m_y = (int)(pos.y * m_worldToSector);
            m_currViewRect.m_width = viewSize;
            m_currViewRect.m_height = viewSize;
        }
    }

    AreaSystemComponent::SectorId AreaSystemComponent::GetSectorId(const AZ::Vector3& worldPos) const
    {
        int wx = (int)(worldPos.GetX() * m_worldToSector);
        int wy = (int)(worldPos.GetY() * m_worldToSector);
        return SectorId(wx, wy);
    }

    AreaSystemComponent::SectorInfo* AreaSystemComponent::CreateSector(const SectorId& sectorId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        SectorInfo sectorInfo;
        sectorInfo.m_id = sectorId;
        sectorInfo.m_worldX = sectorId.first;
        sectorInfo.m_worldY = sectorId.second;
        UpdateSectorBounds(sectorInfo);
        UpdateSectorPoints(sectorInfo);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        SectorInfo& sectorInfoRef = m_sectorRollingWindow[sectorInfo.m_id] = sectorInfo;
        UpdateSectorCallbacks(sectorInfoRef);
        return &sectorInfoRef;
    }

    void AreaSystemComponent::UpdateSectorBounds(SectorInfo& sectorInfo)
    {
        sectorInfo.m_bounds = AZ::Aabb::CreateFromMinMax(
            AZ::Vector3(
                static_cast<float>(sectorInfo.m_worldX * m_configuration.m_sectorSizeInMeters),
                static_cast<float>(sectorInfo.m_worldY * m_configuration.m_sectorSizeInMeters),
                -AZ_FLT_MAX),
            AZ::Vector3(
                static_cast<float>((sectorInfo.m_worldX + 1) * m_configuration.m_sectorSizeInMeters),
                static_cast<float>((sectorInfo.m_worldY + 1) * m_configuration.m_sectorSizeInMeters),
                AZ_FLT_MAX));
    }

    void AreaSystemComponent::UpdateSectorPoints(SectorInfo& sectorInfo)
    {
        const float vegStep = m_configuration.m_sectorSizeInMeters / static_cast<float>(m_configuration.m_sectorDensity);

        //build a free list of all points in the sector for areas to consume
        sectorInfo.m_baseContext.m_masks.clear();
        sectorInfo.m_baseContext.m_availablePoints.clear();
        sectorInfo.m_baseContext.m_availablePoints.reserve(m_configuration.m_sectorDensity * m_configuration.m_sectorDensity);

        // Determine within our texel area where we want to create our vegetation positions:
        // 0 = lower left corner, 0.5 = center
        const float texelOffset = (m_configuration.m_sectorPointSnapMode == SnapMode::Center) ? 0.5f : 0.0f;

        uint claimIndex = 0;
        ClaimPoint claimPoint;
        SurfaceData::SurfacePointList availablePoints;
        for (int u = 0; u < m_configuration.m_sectorDensity; ++u)
        {
            for (int v = 0; v < m_configuration.m_sectorDensity; ++v)
            {
                const float fu = (static_cast<float>(u) + texelOffset) * vegStep;
                const float fv = (static_cast<float>(v) + texelOffset) * vegStep;
                const float posX = static_cast<float>(sectorInfo.m_worldX * m_configuration.m_sectorSizeInMeters) + fu;
                const float posY = static_cast<float>(sectorInfo.m_worldY * m_configuration.m_sectorSizeInMeters) + fv;
                const AZ::Vector3 claimPosition = AZ::Vector3(posX, posY, AZ_FLT_MAX);

                availablePoints.clear();
                SurfaceData::SurfaceDataSystemRequestBus::Broadcast(&SurfaceData::SurfaceDataSystemRequestBus::Events::GetSurfacePoints,
                    claimPosition,
                    SurfaceData::SurfaceTagVector(),
                    availablePoints);

                for (auto& surfacePoint : availablePoints)
                {
                    claimPoint.m_handle = CreateClaimHandle(sectorInfo, ++claimIndex);
                    claimPoint.m_position = surfacePoint.m_position;
                    claimPoint.m_normal = surfacePoint.m_normal;
                    claimPoint.m_masks = surfacePoint.m_masks;
                    sectorInfo.m_baseContext.m_availablePoints.push_back(claimPoint);
                    SurfaceData::AddMaxValueForMasks(sectorInfo.m_baseContext.m_masks, surfacePoint.m_masks);
                }
            }
        }
    }

    void AreaSystemComponent::UpdateSectorCallbacks(SectorInfo& sectorInfo)
    {
        //setup callback to test if matching point is already claimed
        sectorInfo.m_baseContext.m_existedCallback = [this, &sectorInfo](const ClaimPoint& point, const InstanceData& instanceData)
        {
            const ClaimHandle handle = point.m_handle;
            auto claimItr = m_claimedWorldPointsBeforeFill.find(handle);
            bool exists = (claimItr != m_claimedWorldPointsBeforeFill.end()) && InstanceData::IsSameInstanceData(instanceData, claimItr->second);

            if (exists)
            {
                CreateClaim(sectorInfo, handle, instanceData);
                VEG_PROFILE_METHOD(DebugNotificationBus::QueueBroadcast(&DebugNotificationBus::Events::CreateInstance, instanceData.m_instanceId, instanceData.m_position, instanceData.m_id));
            }

            return exists;
        };

        //setup callback to create claims for new instances
        sectorInfo.m_baseContext.m_createdCallback = [this, &sectorInfo](const ClaimPoint& point, const InstanceData& instanceData)
        {
            const ClaimHandle handle = point.m_handle;
            auto claimItr = m_claimedWorldPointsBeforeFill.find(handle);
            bool exists = claimItr != m_claimedWorldPointsBeforeFill.end();

            if (exists)
            {
                const auto& claimedInstanceData = claimItr->second;
                if (claimedInstanceData.m_id != instanceData.m_id)
                {
                    //must force bus connect if areas are different
                    AreaNotificationBus::Event(claimedInstanceData.m_id, &AreaNotificationBus::Events::OnAreaConnect);
                    AreaRequestBus::Event(claimedInstanceData.m_id, &AreaRequestBus::Events::UnclaimPosition, handle);
                    AreaNotificationBus::Event(claimedInstanceData.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);
                }
                else
                {
                    //already connected during fill sector
                    AreaRequestBus::Event(claimedInstanceData.m_id, &AreaRequestBus::Events::UnclaimPosition, handle);
                }
            }

            CreateClaim(sectorInfo, handle, instanceData);
        };
    }

    void AreaSystemComponent::DeleteSector(const SectorId& sectorId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        auto itSector = m_sectorRollingWindow.find(sectorId);
        if (itSector != m_sectorRollingWindow.end())
        {
            SectorInfo& sectorInfo(itSector->second);
            EmptySector(sectorInfo);
            m_sectorRollingWindow.erase(itSector);
        }
    }

    const AreaSystemComponent::SectorInfo* AreaSystemComponent::GetSector(const SectorId& sectorId) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        auto itSector = m_sectorRollingWindow.find(sectorId);
        return itSector != m_sectorRollingWindow.end() ? &itSector->second : nullptr;
    }

    AreaSystemComponent::SectorInfo* AreaSystemComponent::GetSector(const SectorId& sectorId)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        auto itSector = m_sectorRollingWindow.find(sectorId);
        return itSector != m_sectorRollingWindow.end() ? &itSector->second : nullptr;
    }

    void AreaSystemComponent::ReleaseUnregisteredClaims(SectorInfo& sectorInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        for (const auto& areaPair : m_unregisteredVegetationAreaSet)
        {
            if (areaPair.second.m_bounds.Overlaps(sectorInfo.m_bounds))
            {
                for (auto claimItr = sectorInfo.m_claimedWorldPoints.begin(); claimItr != sectorInfo.m_claimedWorldPoints.end(); )
                {
                    const auto& claimPair = *claimItr;
                    if (claimPair.second.m_id == areaPair.first)
                    {
                        claimItr = sectorInfo.m_claimedWorldPoints.erase(claimItr);
                        continue;
                    }

                    ++claimItr;
                }
            }
        }
    }

    void AreaSystemComponent::ReleaseUnusedClaims(SectorInfo& sectorInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set<ClaimHandle>> claimsToRelease;

        // Group up all the previously-claimed-but-no-longer-claimed points based on area id
        for (const auto& claimPair : m_claimedWorldPointsBeforeFill)
        {
            const auto& handle = claimPair.first;
            const auto& instanceData = claimPair.second;
            const auto& areaId = instanceData.m_id;
            if (sectorInfo.m_claimedWorldPoints.find(handle) == sectorInfo.m_claimedWorldPoints.end())
            {
                claimsToRelease[areaId].insert(handle);
            }
        }
        m_claimedWorldPointsBeforeFill.clear();

        // Iterate over the claims by area id and release them
        for (const auto& claimPair : claimsToRelease)
        {
            const auto& areaId = claimPair.first;
            const auto& handles = claimPair.second;
            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaConnect);

            for (const auto& handle : handles)
            {
                AreaRequestBus::Event(areaId, &AreaRequestBus::Events::UnclaimPosition, handle);
            }

            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaDisconnect);
        }
    }

    void AreaSystemComponent::FillSector(SectorInfo& sectorInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);
        VEG_PROFILE_METHOD(DebugNotificationBus::QueueBroadcast(&DebugNotificationBus::Events::FillSectorStart, sectorInfo.m_worldX, sectorInfo.m_worldY, AZStd::chrono::system_clock::now()));

        ReleaseUnregisteredClaims(sectorInfo);

        //m_availablePoints is a free list initialized with the complete set of points in the sector.
        m_activeContext = sectorInfo.m_baseContext;

        // Clear out the list of claimed world points before we begin
        m_claimedWorldPointsBeforeFill = sectorInfo.m_claimedWorldPoints;
        sectorInfo.m_claimedWorldPoints.clear();

        //for all active areas attempt to spawn vegetation on sector grid positions
        for (const auto& area : m_activeAreasInBubble)
        {
            //if one or more areas claimed all the points in m_availablePoints, there's no reason to continue.
            if (m_activeContext.m_availablePoints.empty())
            {
                break;
            }

            //only consider areas that intersect this sector
            if (!area.m_bounds.IsValid() || area.m_bounds.Overlaps(sectorInfo.m_bounds))
            {
                VEG_PROFILE_METHOD(DebugNotificationBus::QueueBroadcast(&DebugNotificationBus::Events::FillAreaStart, area.m_id, AZStd::chrono::system_clock::now()));

                m_activeStackIds.clear();

                //each area is responsible for removing whatever points it claims from m_availablePoints, so subsequent areas will have fewer points to try to claim.
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaConnect);
                AreaRequestBus::Event(area.m_id, &AreaRequestBus::Events::ClaimPositions, m_activeStackIds, m_activeContext);
                AreaNotificationBus::Event(area.m_id, &AreaNotificationBus::Events::OnAreaDisconnect);

                VEG_PROFILE_METHOD(DebugNotificationBus::QueueBroadcast(&DebugNotificationBus::Events::FillAreaEnd, area.m_id, AZStd::chrono::system_clock::now(), m_activeContext.m_availablePoints.size()));
            }
        }
        size_t remainingPointCount = m_activeContext.m_availablePoints.size();

        ReleaseUnusedClaims(sectorInfo);

        m_activeContext = ClaimContext();

        VEG_PROFILE_METHOD(DebugNotificationBus::QueueBroadcast(&DebugNotificationBus::Events::FillSectorEnd, sectorInfo.m_worldX, sectorInfo.m_worldY, AZStd::chrono::system_clock::now(), remainingPointCount));
    }

    void AreaSystemComponent::EmptySector(SectorInfo& sectorInfo)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::unordered_map<AZ::EntityId, AZStd::unordered_set<ClaimHandle>> claimsToRelease;

        // group up all the points based on area id
        for (const auto& claimPair : sectorInfo.m_claimedWorldPoints)
        {
            const auto& handle = claimPair.first;
            const auto& instanceData = claimPair.second;
            const auto& areaId = instanceData.m_id;
            claimsToRelease[areaId].insert(handle);
        }
        sectorInfo.m_claimedWorldPoints.clear();

        // iterate over the claims by area id and release them
        for (const auto& claimPair : claimsToRelease)
        {
            const auto& areaId = claimPair.first;
            const auto& handles = claimPair.second;
            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaConnect);

            for (const auto& handle : handles)
            {
                AreaRequestBus::Event(areaId, &AreaRequestBus::Events::UnclaimPosition, handle);
            }

            AreaNotificationBus::Event(areaId, &AreaNotificationBus::Events::OnAreaDisconnect);
        }
    }

    void AreaSystemComponent::ClearSectors()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        AZStd::lock_guard<decltype(m_sectorRollingWindowMutex)> lock(m_sectorRollingWindowMutex);
        for (auto& sectorPair : m_sectorRollingWindow)
        {
            EmptySector(sectorPair.second);
        }
        m_sectorRollingWindow.clear();

    }

    AZ::Aabb AreaSystemComponent::GetBubbleBounds() const
    {
        return AZ::Aabb::CreateFromMinMax(
            AZ::Vector3(
                static_cast<float>(m_currViewRect.m_x * m_configuration.m_sectorSizeInMeters),
                static_cast<float>(m_currViewRect.m_y * m_configuration.m_sectorSizeInMeters),
                -AZ_FLT_MAX),
            AZ::Vector3(
                static_cast<float>((m_currViewRect.m_x + m_currViewRect.m_width) * m_configuration.m_sectorSizeInMeters),
                static_cast<float>((m_currViewRect.m_y + m_currViewRect.m_height) * m_configuration.m_sectorSizeInMeters),
                AZ_FLT_MAX));
    }

    void AreaSystemComponent::CreateClaim(SectorInfo& sectorInfo, const ClaimHandle handle, const InstanceData& instanceData)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);
        sectorInfo.m_claimedWorldPoints[handle] = instanceData;
    }

    ClaimHandle AreaSystemComponent::CreateClaimHandle(const SectorInfo& sectorInfo, uint32_t index) const
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        ClaimHandle handle = 0;
        AreaSystemUtil::hash_combine_64(handle, sectorInfo.m_id.first);
        AreaSystemUtil::hash_combine_64(handle, sectorInfo.m_id.second);
        AreaSystemUtil::hash_combine_64(handle, index);
        return handle;
    }

    void AreaSystemComponent::ReleaseWithoutCleanup()
    {
        //wait for vegetation thread
        AZStd::lock_guard<decltype(m_vegetationThreadMutex)> lockTasks(m_vegetationThreadMutex);

        //process pending tasks instead of clearing them if this is just an export
        UpdateVegetationAreas();
        ClearSectors();
        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::DestroyAllInstances);
    }

    void AreaSystemComponent::ReleaseData()
    {
        //wait for vegetation thread
        AZStd::lock_guard<decltype(m_vegetationThreadMutex)> lockTasks(m_vegetationThreadMutex);

        //process pending tasks instead of clearing them if this is just an export
        UpdateVegetationAreas();
        ClearSectors();
        InstanceSystemRequestBus::Broadcast(&InstanceSystemRequestBus::Events::Cleanup);
    }

    void AreaSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams)
    {
        m_system = &system;
        m_system->GetISystemEventDispatcher()->RegisterListener(this);
    }

    void AreaSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        if (m_system)
        {
            m_system->GetISystemEventDispatcher()->RemoveListener(this);
            m_system = nullptr;
        }
    }

    void AreaSystemComponent::OnCryEditorBeginLevelExport()
    {
        // We need to free all our instances before exporting a level to ensure that none of the dynamic vegetation data
        // gets exported into the static vegetation data files.

        // Clear all our spawned vegetation data so that they don't get written out with the vegetation sectors.  
        ReleaseData();
    }

    void AreaSystemComponent::OnCryEditorEndLevelExport(bool /*success*/)
    {
        // We don't need to do anything here.  When the vegetation game components reactivate themselves after the level export completes,
        // (see EditorVegetationComponentBase.h) they will trigger a refresh of the vegetation areas which will produce all our instances again.  
    }


    void AreaSystemComponent::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Entity);

        switch (event)
        {
        case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
        case ESYSTEM_EVENT_LEVEL_LOAD_START:
        case ESYSTEM_EVENT_LEVEL_UNLOAD:
        {
            ReleaseData();
            break;
        }
        default:
            break;
        }
    }
}