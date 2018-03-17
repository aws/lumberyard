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

#include "CloudGemDynamicContent_precompiled.h"
#include <platform_impl.h>

#include "DynamicContentGem.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include <AzCore/RTTI/RTTI.h>

#if defined(DYNAMIC_CONTENT_EDITOR)
#include "DynamicContentEditorSystemComponent.h"
#endif

#include <AzCore/std/smart_ptr/make_shared.h>
#include <DynamicContentTransferManager.h>

namespace DynamicContent
{
    DynamicContentGem::DynamicContentGem()
    {
        m_descriptors.insert(m_descriptors.end(), {
            CloudCanvas::DynamicContent::DynamicContentTransferManager::CreateDescriptor()
        });
    }

    void DynamicContentGem::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        using namespace DynamicContent;

        switch (event)
        {
        case ESYSTEM_EVENT_FLOW_SYSTEM_REGISTER_EXTERNAL_NODES:
            RegisterExternalFlowNodes();
            break;

        case ESYSTEM_EVENT_GAME_POST_INIT:
            // Put your init code here
            // All other Gems will exist at this point
#if defined(PLATFORM_SUPPORTS_AWS_NATIVE_SDK)
            m_presignedManager = AZStd::make_shared<CloudCanvas::PresignedURLManager>();
#endif
            break;

        case ESYSTEM_EVENT_FULL_SHUTDOWN:
        case ESYSTEM_EVENT_FAST_SHUTDOWN:
            // Put your shutdown code here
            // Other Gems may have been shutdown already, but none will have destructed
            break;
        }
    }

    AZ::ComponentTypeList DynamicContent::DynamicContentGem::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<CloudCanvas::DynamicContent::DynamicContentTransferManager>(),
        };
    }
}
AZ_DECLARE_MODULE_CLASS(CloudGemDynamicContent_3a3eeef064a04c37b4513c6378f4f56a, DynamicContent::DynamicContentGem)
