/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/
#include "StdAfx.h"
#include "CloudsGem.h"

#if !defined(AZ_MONOLITHIC_BUILD)
#include <platform_impl.h> // must be included once per DLL so things from CryCommon will function
#endif

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/list.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

#include <CloudComponent.h>
#include <CloudsSystemComponent.h>

#include <ICryAnimation.h>
#include <I3DEngine.h>
#include <IGem.h>

namespace CloudsGem
{
    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    CloudsGemModule::CloudsGemModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
                CloudComponent::CreateDescriptor(),
                CloudsSystemComponent::CreateDescriptor()
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

    //! Request system components on the system entity.
    //! These components' memory is owned by the system entity.
    AZ::ComponentTypeList CloudsGemModule::GetRequiredSystemComponents() const
    {
        return {
                   azrtti_typeid<CloudsSystemComponent>(),
        };
    }
}

#if !defined(CLOUDS_GEM_EDITOR)
    AZ_DECLARE_MODULE_CLASS(Clouds_4f293a6f11ab4cd7b9db3017d12b5eda, CloudsGem::CloudsGemModule)
#endif
