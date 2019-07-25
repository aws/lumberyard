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
#ifndef AZ_UNITY_BUILD

#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Crc.h>

#include <AzCore/Driller/Driller.h>
#include <AzCore/IO/StreamerDriller.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/UserSettings/UserSettings.h>

#include <AzCore/IO/Streamer.h>

#include <AzCore/std/parallel/thread.h>

namespace AZ
{
    /**
     * Component user settings.
     */
    class StreamerComponentUserSettings
        : public UserSettings
    {
    public:
        AZ_CLASS_ALLOCATOR(StreamerComponentUserSettings, SystemAllocator, 0);
        AZ_RTTI(AZ::StreamerComponentUserSettings, "{DBE98B4C-766C-453E-84AD-AF5425696591}", UserSettings);

        StreamerComponentUserSettings()
            : m_enableDrilling(true)
            , m_drillingTrackStreams(false)
        {}

        bool    m_enableDrilling;               ///< True if we will enable drilling support.
        bool    m_drillingTrackStreams;         ///< True if we want the driller to track streams even when it's not started.
    };

    //=========================================================================
    // StreamerComponent
    // [5/29/2012]
    //=========================================================================
    StreamerComponent::StreamerComponent()
    {
        IO::Streamer::Descriptor defaultDesc;
        m_maxNumOpenLimitedStream = defaultDesc.m_maxNumOpenLimitedStream;
        m_cacheBlockSize = defaultDesc.m_cacheBlockSize;
        m_numCacheBlocks = defaultDesc.m_numCacheBlocks;
        m_driller = nullptr;

        {
            // assign default priority
            AZStd::thread_desc threadDesc;
            m_deviceThreadCpuId = threadDesc.m_cpuId;
            m_deviceThreadPriority = threadDesc.m_priority;
            m_deviceThreadSleepTimeMS = -1;

        #if (AZ_TRAIT_SET_STREAMER_COMPONENT_THREAD_AFFINITY_TO_USERTHREADS)
            m_deviceThreadCpuId = AFFINITY_MASK_USERTHREADS;
        #endif
        }
    }

    //=========================================================================
    // Activate
    // [5/29/2012]
    //=========================================================================
    void StreamerComponent::Activate()
    {
        AZStd::intrusive_ptr<StreamerComponentUserSettings> userSettings = UserSettings::CreateFind<StreamerComponentUserSettings>(AZ_CRC("StreamerComponentUserSettings", 0x4c57e868), UserSettings::CT_GLOBAL);
        if (userSettings->m_enableDrilling)
        {
            Debug::DrillerManager* drillerManager = nullptr;
            EBUS_EVENT_RESULT(drillerManager, ComponentApplicationBus, GetDrillerManager);
            if (drillerManager)
            {
                m_driller = aznew Debug::StreamerDriller(userSettings->m_drillingTrackStreams);
                drillerManager->Register(m_driller);
            }
        }

        AZ_Assert(IO::Streamer::IsReady() == false, "Streamer should be inactive at this stage!");
        IO::Streamer::Descriptor streamerDesc;

        streamerDesc.m_maxNumOpenLimitedStream = m_maxNumOpenLimitedStream;
        streamerDesc.m_cacheBlockSize = m_cacheBlockSize;
        streamerDesc.m_numCacheBlocks = m_numCacheBlocks;

        AZStd::thread_desc threadDesc;
        if (m_deviceThreadCpuId != threadDesc.m_cpuId)
        {
            threadDesc.m_cpuId = m_deviceThreadCpuId;
        }
        if (m_deviceThreadPriority != threadDesc.m_priority)
        {
            threadDesc.m_priority = m_deviceThreadPriority;
        }
        streamerDesc.m_deviceThreadSleepTimeMS = m_deviceThreadSleepTimeMS;
        streamerDesc.m_threadDesc = &threadDesc;

        const char* root = NULL;
        EBUS_EVENT_RESULT(root, ComponentApplicationBus, GetAppRoot);
        if (root && root[0] != 0)
        {
            streamerDesc.m_fileMountPoint = root;
        }

        IO::Streamer::Create(streamerDesc);
        AZ::SystemTickBus::Handler::BusConnect();
        StreamerComponentRequests::Bus::Handler::BusConnect();
    }

    //=========================================================================
    // Deactivate
    // [5/29/2012]
    //=========================================================================
    void StreamerComponent::Deactivate()
    {
        AZ_Assert(IO::Streamer::IsReady(), "Streamer should be created/active at this stage!");
        StreamerComponentRequests::Bus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
        IO::Streamer::Destroy();

        if (m_driller)
        {
            Debug::DrillerManager* drillerManager = NULL;
            EBUS_EVENT_RESULT(drillerManager, ComponentApplicationBus, GetDrillerManager);
            if (drillerManager)
            {
                drillerManager->Unregister(m_driller);
            }

            delete m_driller;
            m_driller = nullptr;
        }
    }

    //=========================================================================
    // OnSystemTick
    //=========================================================================
    void StreamerComponent::OnSystemTick()
    {
        IO::Streamer::Instance().ReceiveRequests();
    }

    //=========================================================================
    // GetStreamer
    //=========================================================================
    AZ::IO::Streamer* StreamerComponent::GetStreamer()
    {
        if (AZ::IO::Streamer::IsReady())
        {
            return &AZ::IO::Streamer::Instance();
        }

        return nullptr;
    }

    //=========================================================================
    // GetProvidedServices
    //=========================================================================
    void StreamerComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("DataStreamingService", 0xb1b981f5));
    }

    //=========================================================================
    // GetIncompatibleServices
    //=========================================================================
    void StreamerComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("DataStreamingService", 0xb1b981f5));
    }

    //=========================================================================
    // GetDependentServices
    //=========================================================================
    void StreamerComponent::GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("UserSettingsService", 0xa0eadff5));
        dependent.push_back(AZ_CRC("ProfilerService", 0x505033c9));
    }

    //=========================================================================
    // Reflect
    //=========================================================================
    void StreamerComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StreamerComponent, AZ::Component>()
                ->Version(2)
                ->Field("MaxNumberOpenLimitedStream", &StreamerComponent::m_maxNumOpenLimitedStream)
                ->Field("CacheBlockSize", &StreamerComponent::m_cacheBlockSize)
                ->Field("NumberOfCacheBlocks", &StreamerComponent::m_numCacheBlocks)
                ->Field("DeviceThreadCpuId", &StreamerComponent::m_deviceThreadCpuId)
                ->Field("DeviceThreadPriority", &StreamerComponent::m_deviceThreadPriority)
                ->Field("DeviceThreadSleepTimeMS", &StreamerComponent::m_deviceThreadSleepTimeMS)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StreamerComponent>(
                    "Streamer System", "Provides fully async, prioritized, time dependent data loading from all devices")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_maxNumOpenLimitedStream, "Max Open Limited Streams", "Maximum number of open limited streams (only certain streams are flagged limited) at the same time")
                        ->Attribute(AZ::Edit::Attributes::Max, 1024)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_cacheBlockSize, "Data cache block size", "Size in bytes of a single data cache block (used for read and write)")
                        ->Attribute(AZ::Edit::Attributes::Max, 10 * 1024 * 1024)
                        ->Attribute(AZ::Edit::Attributes::Step, 4096)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_numCacheBlocks, "Number of cache blocks", "Provides the number of cache blocks for the system")
                        ->Attribute(AZ::Edit::Attributes::Min, 4)
                        ->Attribute(AZ::Edit::Attributes::Max, 128)
                        ->Attribute(AZ::Edit::Attributes::Step, 2)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_deviceThreadCpuId, "Device thread CPU id", "CPU core id to use for all device threads")
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                        ->Attribute(AZ::Edit::Attributes::Max, 5)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_deviceThreadPriority, "Device thread priority", "Thread priority to use for all device threads")
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_deviceThreadSleepTimeMS, "Device thread sleep", "Speed time in millisecond between each operation. Use with caution!")
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000)
                    ;
            }

            // user settings
            serializeContext->Class<StreamerComponentUserSettings>()
                ->Field("enableDrilling", &StreamerComponentUserSettings::m_enableDrilling)
                ->Field("drillingTrackStreams", &StreamerComponentUserSettings::m_drillingTrackStreams)
                ;
        }
    }
} // namespace AZ

#endif // #ifndef AZ_UNITY_BUILD
