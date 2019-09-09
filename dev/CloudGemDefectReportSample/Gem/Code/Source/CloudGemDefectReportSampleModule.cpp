/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "CloudGemDefectReportSample_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CloudGemDefectReportSampleSystemComponent.h"

#include <IGem.h>

namespace CloudGemDefectReportSample
{
    class CloudGemDefectReportSampleModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudGemDefectReportSampleModule, "{DF38CA34-7D30-46E1-88CA-2650C8F366C2}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(CloudGemDefectReportSampleModule, AZ::SystemAllocator, 0);

        CloudGemDefectReportSampleModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemDefectReportSampleSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemDefectReportSampleSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemDefectReportSample_99fd2a91d540451fa728e40d2e4dd16f, CloudGemDefectReportSample::CloudGemDefectReportSampleModule)
