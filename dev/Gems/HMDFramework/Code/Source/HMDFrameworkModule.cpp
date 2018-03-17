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

#include "HMDFramework_precompiled.h"
#include <platform_impl.h>
#include "HMDDebuggerComponent.h"
#include "HMDLuaComponent.h"
#ifdef VR_EDITOR
#include "EditorVRPreviewComponent.h"
#endif // VR_EDITOR
#include <IGem.h>

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

namespace HMDFramework
{
    class HMDFrameworkModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(HMDFrameworkModule, "{57CFF7A2-A9D0-4D30-912E-4564C4DF19D3}", CryHooksModule);

        HMDFrameworkModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), 
            {
                HMDDebuggerComponent::CreateDescriptor(),
                AZ::VR::HMDLuaComponent::CreateDescriptor(),
#ifdef VR_EDITOR
                AZ::VR::EditorVRPreviewComponent::CreateDescriptor(),
#endif //VR_EDITOR
            });

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
            return AZ::ComponentTypeList
            {
                azrtti_typeid<HMDDebuggerComponent>(),
                azrtti_typeid<AZ::VR::HMDLuaComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(HMDFramework_24a3427048184feba39ba2cf75d45c4c, HMDFramework::HMDFrameworkModule)
