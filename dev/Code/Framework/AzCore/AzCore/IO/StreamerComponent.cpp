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
        m_driller = nullptr;

        {
            // assign default priority
            AZStd::thread_desc threadDesc;
            m_deviceThreadCpuId = threadDesc.m_cpuId;
            m_deviceThreadPriority = threadDesc.m_priority;

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

        AZStd::thread_desc threadDesc;
        if (m_deviceThreadCpuId != threadDesc.m_cpuId)
        {
            threadDesc.m_cpuId = m_deviceThreadCpuId;
        }
        if (m_deviceThreadPriority != threadDesc.m_priority)
        {
            threadDesc.m_priority = m_deviceThreadPriority;
        }
        streamerDesc.m_threadDesc = &threadDesc;
        streamerDesc.m_streamStack = AZ::IO::StreamStack::CreateDefaultStreamStack(m_streamStackPreferences);

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

    const AZ::IO::StreamStack::Preferences* StreamerComponent::GetPreferences() const
    {
        return &m_streamStackPreferences;
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
        using namespace AZ::IO;

        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<StreamStack::CacheConfiguration>()->Version(1)
                ->Field("Configuration", &StreamStack::CacheConfiguration::m_configuration)
                ->Field("BlockSizeMB", &StreamStack::CacheConfiguration::m_blockSizeMB)
                ->Field("OnlyEpilogWrites", &StreamStack::CacheConfiguration::m_onlyEpilogWrites)
                ;

            serializeContext->Class<StreamStack::Preferences>()->Version(3)
                ->Field("PlatformIntegration", &StreamStack::Preferences::m_platformIntegration)
                ->Field("FileHandleCachePreference", &StreamStack::Preferences::m_fileHandleCache)
                ->Field("BlockCache", &StreamStack::Preferences::m_blockCache)
                ->Field("DedicatedCache", &StreamStack::Preferences::m_dedicatedCache)
                ->Field("ReadSplitter", &StreamStack::Preferences::m_readSplitter)
                ->Field("FullDecompressionJobThreads", &StreamStack::Preferences::m_fullDecompressionJobThreads)
                ->Field("FullDecompressionReads", &StreamStack::Preferences::m_fullDecompressionReads)
                ;
            
            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StreamStack::CacheConfiguration>("Cache Configuration",
                    "Preferences for a specific cache in the stream stack.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &StreamStack::CacheConfiguration::m_configuration,
                        "Block cache configuration", "Configures how the memory allocated for the block cache will be divided.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues,
                            AZStd::vector<AZ::Edit::EnumConstant<StreamStack::BlockCacheConfiguration>>
                            {
                                AZ::Edit::EnumConstant<StreamStack::BlockCacheConfiguration>(StreamStack::BlockCacheConfiguration::Disabled,
                                    "Disabled - In some cases a small cache may still be created."),
                                    AZ::Edit::EnumConstant<StreamStack::BlockCacheConfiguration>(StreamStack::BlockCacheConfiguration::SmallBlocks,
                                        "Small blocks - A large number of small sized blocks."),
                                    AZ::Edit::EnumConstant<StreamStack::BlockCacheConfiguration>(StreamStack::BlockCacheConfiguration::Balanced,
                                        "Balanced - An average number of blocks of average size."),
                                    AZ::Edit::EnumConstant<StreamStack::BlockCacheConfiguration>(StreamStack::BlockCacheConfiguration::LargeBlocks,
                                        "Large blocks - A small number of large sized blocks.")
                            })
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StreamStack::CacheConfiguration::m_blockSizeMB,
                        "Block cache size (MB)", "The size in megabytes of the block cache.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StreamStack::CacheConfiguration::m_onlyEpilogWrites,
                        "Only epilog writes", "If true both the prolog and epilog of a request can read from a cache, but only the epilog can write.")
                ;

                editContext->Class<StreamStack::Preferences>("Stream Stack Configuration",
                    "Preferences for the various components that make up the IO stack behind Streamer.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &StreamStack::Preferences::m_platformIntegration,
                        "Platform integration", "Whether to use platform agnostic or specific implementations")
                        ->Attribute(AZ::Edit::Attributes::EnumValues,
                            AZStd::vector<AZ::Edit::EnumConstant<StreamStack::PlatformIntegration>>
                            {
                                AZ::Edit::EnumConstant<StreamStack::PlatformIntegration>(StreamStack::PlatformIntegration::Agnostic, 
                                    "Agnostic - Use standard, cross-platform implementations."),
                                AZ::Edit::EnumConstant<StreamStack::PlatformIntegration>(StreamStack::PlatformIntegration::Specific, 
                                    "Specific - Where possible and available, use platform specific implementations."),
                            })
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &StreamStack::Preferences::m_fileHandleCache,
                        "File handle cache", "Indicate how big the cache for file handles (virtual) drives should be.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues, 
                            AZStd::vector<AZ::Edit::EnumConstant<StreamStack::FileHandleCache>>
                            {
                                AZ::Edit::EnumConstant<StreamStack::FileHandleCache>(StreamStack::FileHandleCache::Small, 
                                    "Small - Keep only a few file handles around to conserve resources."),
                                AZ::Edit::EnumConstant<StreamStack::FileHandleCache>(StreamStack::FileHandleCache::Balanced, 
                                    "Balanced - Keep enough file handles around to always keep archives open for the average game."),
                                AZ::Edit::EnumConstant<StreamStack::FileHandleCache>(StreamStack::FileHandleCache::Large, 
                                    "Large - Keep a large number of file handles open, best if loose files are used.")
                            })
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StreamStack::Preferences::m_blockCache,
                        "Block Cache Configuration", "Configuration for a cache that reads tries to cache the beginning and end of a request.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StreamStack::Preferences::m_dedicatedCache,
                        "Dedicated Cache Configuration", "Configuration for a cache that assigns a dedicated block cache per file.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &StreamStack::Preferences::m_readSplitter,
                        "Read splitter", "Configuration for how read requests are broken down in smaller pieces.")
                        ->Attribute(AZ::Edit::Attributes::EnumValues,
                            AZStd::vector<AZ::Edit::EnumConstant<StreamStack::ReadSplitterConfiguration>>
                            {
                                AZ::Edit::EnumConstant<StreamStack::ReadSplitterConfiguration>(StreamStack::ReadSplitterConfiguration::Disabled,
                                    "Disabled - Reads are never split."),
                                AZ::Edit::EnumConstant<StreamStack::ReadSplitterConfiguration>(StreamStack::ReadSplitterConfiguration::BlockCacheSize,
                                    "Block cache - Use the same size as used for the block cache."),
                                AZ::Edit::EnumConstant<StreamStack::ReadSplitterConfiguration>(StreamStack::ReadSplitterConfiguration::BalancedSize,
                                    "Balanced - Use a platform/hardware specific value that's a good balance between responsiveness and throughput.")
                            })
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StreamStack::Preferences::m_fullDecompressionJobThreads,
                        "Num full decompression threads", 
                        "The number of job threads used for decompression that requires reading the entire file.\n"
                        "Increase this number if the full file decompressor is frequently compression bound. Increasing this number will consume more memory.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StreamStack::Preferences::m_fullDecompressionReads,
                        "Num full decompression reads",
                        "The number of reads the full file decompressor will read ahead.\n"
                        "Increase this number if the full file decompressor is frequently read bound. Increasing this number will consume more memory. "
                        "Nodes below the full file decompressor node in the stream stack may not support multiple reads, in which case increasing this number will have limited effect.");
                    ;
            }

            serializeContext->Class<StreamerComponent, AZ::Component>()
                ->Version(6)
                ->Field("DeviceThreadCpuId", &StreamerComponent::m_deviceThreadCpuId)
                ->Field("DeviceThreadPriority", &StreamerComponent::m_deviceThreadPriority)
                ->Field("StreamStackConfiguration", &StreamerComponent::m_streamStackPreferences)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<StreamerComponent>(
                    "Streamer System", "Provides fully async, prioritized, time dependent data loading from all devices")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_deviceThreadCpuId, "Device thread CPU id", "CPU core id to use for all device threads")
                        ->Attribute(AZ::Edit::Attributes::Min, -1)
                        ->Attribute(AZ::Edit::Attributes::Max, 5)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &StreamerComponent::m_deviceThreadPriority, "Device thread priority", "Thread priority to use for all device threads")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &StreamerComponent::m_streamStackPreferences, "Preferred defaults",
                        "Provide hints to Streamer on how to setup the stream stack that does the actual IO work.")
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
