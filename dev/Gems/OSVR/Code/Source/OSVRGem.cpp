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

#include "OSVR_precompiled.h"
#include <platform_impl.h>
#include <IGem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#if defined(AZ_PLATFORM_WINDOWS_X64)
#include "OSVRDevice.h"
#endif // Windows x64

namespace OSVR
{
    class OSVRGem
        : public CryHooksModule
    {
    public:
        AZ_RTTI(OSVRGem, "{AA62E94F-E9B1-477D-BD23-AD6B9EA66A7C}", AZ::Module);

        OSVRGem()
            : CryHooksModule()
        {
#if defined(AZ_PLATFORM_WINDOWS_X64)
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                OSVRDevice::CreateDescriptor(),
            });
#endif // Windows x64




            // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
            // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
            AZStd::vector<AZ::Uuid> typeIds;
            typeIds.reserve(m_descriptors.size());
            for (AZ::ComponentDescriptor* descriptor : m_descriptors)
            {
                typeIds.emplace_back(descriptor->GetUuid());
            }
            EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
#if defined(AZ_PLATFORM_WINDOWS_X64)
                azrtti_typeid<OSVRDevice>(),
#endif // Windows x64
            };
        }

        void OnSystemEvent(ESystemEvent event, UINT_PTR, UINT_PTR) override
        {
            switch (event)
            {
            case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
                RegisterExternalFlowNodes();
                break;
            }
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(OSVR_0ca799d093e742b28617e56b3da3f3ce, OSVR::OSVRGem)
