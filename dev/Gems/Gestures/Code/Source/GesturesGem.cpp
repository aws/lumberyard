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

#include "StdAfx.h"
#include <platform_impl.h>
#include <IGem.h>
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "GestureManager.h"

#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

namespace Gestures
{
    class GesturesModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(GesturesModule, "{5648A92C-04A3-4E30-B4E2-B0AEB280CA44}", AZ::Module);

        GesturesModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                Manager::CreateDescriptor(),
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

        ~GesturesModule() override = default;

        /**
        * Add required SystemComponents to the SystemEntity.
        */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<Manager>(),
            };
        }

        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override
        {
            switch (event)
            {
            case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
            {
                RegisterExternalFlowNodes();
            }
            break;
            }
        }
    };
}

AZ_DECLARE_MODULE_CLASS(Gestures_6056556b6088413984309c4a413593ad, Gestures::GesturesModule)
