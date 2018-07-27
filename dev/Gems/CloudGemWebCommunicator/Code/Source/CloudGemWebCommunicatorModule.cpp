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

#include "CloudGemWebCommunicatorComponent.h"
#include <AzCore/Module/Module.h>

namespace CloudGemWebCommunicator
{
    class CloudGemWebCommunicatorModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CloudGemWebCommunicatorModule, "{BB162DCC-141F-45C0-B9DE-F71C66A41605}", AZ::Module);

        CloudGemWebCommunicatorModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemWebCommunicatorComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemWebCommunicatorComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemWebCommunicator_327bb13034c1459da8478826edba71f1, CloudGemWebCommunicator::CloudGemWebCommunicatorModule)
