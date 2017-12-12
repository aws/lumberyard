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

#include <platform_impl.h>
#include <IGem.h>
#include <IConsole.h>

#include "ProfileTelemetryComponent.h"

#ifdef AZ_PROFILE_TELEMETRY
namespace
{
    const char* s_telemetryAddress;
    int s_telemetryPort;
    int s_telemetryCaptureMask;
    int s_memCaptureEnabled;

    void CmdTelemetryToggleEnabled(IConsoleCmdArgs* args)
    {
        using TelemetryRequestBus = RADTelemetry::ProfileTelemetryRequestBus;
        using TelemetryRequests = RADTelemetry::ProfileTelemetryRequests;

        TelemetryRequestBus::Broadcast(&TelemetryRequests::SetAddress, s_telemetryAddress, s_telemetryPort);

        const AZ::u32 fullCaptureMask = static_cast<AZ::u32>(s_telemetryCaptureMask) | (s_memCaptureEnabled ? AZ_PROFILE_CAT_TO_RAD_CAPFLAGS(AZ::Debug::ProfileCategory::MemoryReserved) : 0);
        TelemetryRequestBus::Broadcast(&TelemetryRequests::SetCaptureMask, fullCaptureMask);

        TelemetryRequestBus::Broadcast(&TelemetryRequests::ToggleEnabled);
    }
}
#endif

namespace RADTelemetry
{
    class RADTelemetryModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(RADTelemetryModule, "{50BB63A6-4669-41F2-B93D-6EB8529413CD}", CryHooksModule);

        RADTelemetryModule()
            : CryHooksModule()
        {
#ifdef AZ_PROFILE_TELEMETRY
            m_descriptors.insert(m_descriptors.end(), {
                ProfileTelemetryComponent::CreateDescriptor(),
            });
#endif
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            AZ::ComponentTypeList components;

#ifdef AZ_PROFILE_TELEMETRY
            components.insert(components.end(),
                azrtti_typeid<ProfileTelemetryComponent>()
            );
#endif

            return components;
        }

        void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& initParams) override
        {
            CryHooksModule::OnCrySystemInitialized(system, initParams);
            
#ifdef AZ_PROFILE_TELEMETRY
            REGISTER_COMMAND("radtm_ToggleEnabled", &CmdTelemetryToggleEnabled, 0, "Enabled or Disable RAD Telemetry");

            REGISTER_CVAR2("radtm_Address", &s_telemetryAddress, "localhost", VF_NULL, "The IP address for the telemetry server");
            REGISTER_CVAR2("radtm_Port", &s_telemetryPort, 4719, VF_NULL, "The port for the RAD telemetry server");
            REGISTER_CVAR2("radtm_MemoryCaptureEnabled", &s_memCaptureEnabled, 0, VF_NULL, "Toggle for telemetry memory capture");

            const int defaultCaptureMask = static_cast<int>((1 << static_cast<AZ::u32>(AZ::Debug::ProfileCategory::FirstDetailedCategory)) - 1); // Set all the category bits "below" Detailed by default
            REGISTER_CVAR2("radtm_CaptureMask", &s_telemetryCaptureMask, defaultCaptureMask, VF_NULL, "The bit mask for the categories to be captured, 0 for all");
#endif
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(RADTelemetry_bdaf32823406492686e35200afc555b3, RADTelemetry::RADTelemetryModule)
