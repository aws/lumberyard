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

#include "CloudGemComputeFarm_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CloudGemComputeFarmSystemComponent.h"

#include <AzCore/Module/Module.h>

namespace CloudGemComputeFarm
{
    class CloudGemComputeFarmModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CloudGemComputeFarmModule, "{B1A95129-DE0B-49CE-A592-9BFE89F175BF}", AZ::Module);
        AZ_CLASS_ALLOCATOR(CloudGemComputeFarmModule, AZ::SystemAllocator, 0);

        CloudGemComputeFarmModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemComputeFarmSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemComputeFarmSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemComputeFarm_99586a182bea4616a6d1d72cce733e8c, CloudGemComputeFarm::CloudGemComputeFarmModule)
