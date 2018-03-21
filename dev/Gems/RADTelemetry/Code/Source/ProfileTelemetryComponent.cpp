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

#ifdef AZ_PROFILE_TELEMETRY

#include <AzCore/Debug/ProfilerBus.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <RADTelemetry/ProfileTelemetry.h>

#include "ProfileTelemetryComponent.h"

#ifdef AZ_MONOLITHIC_BUILD
    // The monolithic binary RAD Telemetry instance pointer
    tm_api* g_radTmApi;
#endif


namespace
{
    const char * ProfileChannel = "RADTelemetry";
    const AZ::u32 MaxProfileThreadCount = 128;
}

namespace RADTelemetry
{
    ProfileTelemetryComponent::ProfileTelemetryComponent()
    {
        // Connecting in the constructor because we need to catch ALL created threads
        AZStd::ThreadEventBus::Handler::BusConnect();
    }

    ProfileTelemetryComponent::~ProfileTelemetryComponent()
    {
        AZ_Assert(!m_running, "A telemetry session should not be open.");

        AZStd::ThreadEventBus::Handler::BusDisconnect();

        if (IsInitialized())
        {
            tmShutdown();
            free(m_buffer);
            m_buffer = nullptr;
        }
    }

    void ProfileTelemetryComponent::Activate()
    {
        ProfileTelemetryRequestBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }

    void ProfileTelemetryComponent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();
        ProfileTelemetryRequestBus::Handler::BusDisconnect();

        Disable();
    }

    void ProfileTelemetryComponent::OnThreadEnter(const AZStd::thread_id& id, const AZStd::thread_desc* desc)
    {
        (void)id;
        (void)desc;
#if AZ_TRAIT_OS_USE_WINDOWS_THREADS
        if (!desc)
        {
            // Skip unnamed threads
            return;
        }

        if (IsInitialized())
        {
            // We can send the thread name to Telemetry now
            const AZ::u32 newProfiledThreadCount = ++m_profiledThreadCount;
            AZ_Assert(newProfiledThreadCount <= MaxProfileThreadCount, "RAD Telemetry profiled threadcount exceeded MaxProfileThreadCount!");
            tmThreadName(0, id.m_id, desc->m_name);
            return;
        }

        // Save off to send on the next connection
        ScopedLock lock(m_threadNameLock);

        auto end = m_threadNames.end();
        auto itr = AZStd::find_if(m_threadNames.begin(), end, [id](const ThreadNameEntry& entry)
        {
            return entry.id == id;
        });

        if (itr != end)
        {
            itr->name = desc->m_name;
        }
        else
        {
            m_threadNames.push_back({ id, desc->m_name });
        }
#else
        const AZ::u32 newProfiledThreadCount = ++m_profiledThreadCount;
        AZ_Assert(newProfiledThreadCount <= MaxProfileThreadCount, "RAD Telemetry profiled threadcount exceeded MaxProfileThreadCount!");
#endif
    }

    void ProfileTelemetryComponent::OnThreadExit(const AZStd::thread_id& id)
    {
        (void)id;
#if AZ_TRAIT_OS_USE_WINDOWS_THREADS
        {
            ScopedLock lock(m_threadNameLock);

            auto end = m_threadNames.end();
            auto itr = AZStd::find_if(m_threadNames.begin(), end, [id](const ThreadNameEntry& entry)
            {
                return entry.id == id;
            });
            if (itr != end)
            {
                m_threadNames.erase(itr);
            }
            else
            {
                // assume it was already sent on to RAD Telemetry
                tmEndThread(0, id.m_id);
                --m_profiledThreadCount;
            }
        }
#else
        --m_profiledThreadCount;
#endif
    }

    void ProfileTelemetryComponent::OnSystemTick()
    {
        tmTick(0);
    }

    void ProfileTelemetryComponent::ToggleEnabled()
    {
        Initialize();

        if (!m_running)
        {
            Enable();
        }
        else
        {
            Disable();
        }
    }

    tm_api* ProfileTelemetryComponent::GetApiInstance()
    {
        Initialize();

        return TM_API_PTR;
    }

    void ProfileTelemetryComponent::Enable()
    {
        AZ_Printf(ProfileChannel, "Attempting to connect to the Telemetry server at %s:%d", m_address, m_port);

        tmSetCaptureMask(m_captureMask);
        tm_error result = tmOpen(
            0,                      // unused
            "ly",                   // program name, don't use slashes or weird character that will screw up a filename
            __DATE__ " " __TIME__,  // identifier, could be date time, or a build number ... whatever you want
            m_address,              // telemetry server address
            TMCT_TCP,               // network capture
            m_port,                 // telemetry server port
            TMOF_INIT_NETWORKING,   // flags
            3000                    // timeout in milliseconds ... pass -1 for infinite
        );

        switch (result)
        {
            case TM_OK:
            {
                m_running = true;
                AZ_Printf(ProfileChannel, "Connected to the Telemetry server!");

#if AZ_TRAIT_OS_USE_WINDOWS_THREADS
                ScopedLock lock(m_threadNameLock);
                for (const auto& threadNameEntry : m_threadNames)
                {
                    const AZ::u32 newProfiledThreadCount = ++m_profiledThreadCount;
                    AZ_Assert(newProfiledThreadCount <= MaxProfileThreadCount, "RAD Telemetry profiled threadcount exceeded MaxProfileThreadCount!");
                    tmThreadName(0, threadNameEntry.id.m_id, threadNameEntry.name.c_str());
                }
                m_threadNames.clear(); // Telemetry caches names so we can clear what we have sent on
#endif

                break;
            }

            case TMERR_DISABLED:
                AZ_Printf(ProfileChannel, "Telemetry is disabled via #define NTELEMETRY");
                break;

            case TMERR_UNINITIALIZED:
                AZ_Printf(ProfileChannel, "tmInitialize failed or was not called");
                break;

            case TMERR_NETWORK_NOT_INITIALIZED:
                AZ_Printf(ProfileChannel, "WSAStartup was not called before tmOpen! Call WSAStartup or pass TMOF_INIT_NETWORKING.");
                break;

            case TMERR_NULL_API:
                AZ_Printf(ProfileChannel, "There is no Telemetry API (the DLL isn't in the EXE's path)!");
                break;

            case TMERR_COULD_NOT_CONNECT:
                AZ_Printf(ProfileChannel, "Unable to connect to the Telemetry server at %s:%d (1. is it running? 2. check firewall settings)", m_address, m_port);
                break;

            case TMERR_UNKNOWN:
                AZ_Printf(ProfileChannel, "Unknown error occurred");
                break;

            default:
                AZ_Assert(false, "Unhandled tmOpen error case %d", result);
                break;
        }
    }

    void ProfileTelemetryComponent::Disable()
    {
        if (m_running)
        {
            m_running = false;
            tmClose(0);
            AZ_Printf(ProfileChannel, "Disconnected from the Telemetry server.");
        }
    }

    TM_EXPORT_API tm_api* g_tm_api; // Required for the RAD Telemetry as static lib case
    void ProfileTelemetryComponent::Initialize()
    {
        if (IsInitialized())
        {
            return;
        }

        tmLoadLibrary(TM_RELEASE);

        if (!TM_API_PTR)
        {
            // Telemetry must be compiled as a static lib, set our global instance pointer to the internal telemetry one
            TM_API_PTR = g_tm_api;
        }
        AZ_Assert(TM_API_PTR, "Invalid RAD Telemetry API pointer state");

        tmSetMaxThreadCount(MaxProfileThreadCount);

        const tm_int32 telemetryBufferSize = 16 * 1024 * 1024;
        m_buffer = static_cast<char*>(malloc(telemetryBufferSize));
        tmInitialize(telemetryBufferSize, m_buffer);

        // Notify so individual modules can update their Telemetry pointer
        AZ::Debug::ProfilerNotificationBus::Broadcast(&AZ::Debug::ProfilerNotifications::OnProfileSystemInitialized);
    }

    bool ProfileTelemetryComponent::IsInitialized() const {
        return m_buffer != nullptr;
    }

    void ProfileTelemetryComponent::SetAddress(const char *address, AZ::u16 port)
    {
        m_address = address;
        m_port = port;
    }

    void ProfileTelemetryComponent::SetCaptureMask(AZ::u32 mask)
    {
        m_captureMask = mask;
    }

    void ProfileTelemetryComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ProfileTelemetryComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void ProfileTelemetryComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ProfilerService"));
    }
}

#endif
