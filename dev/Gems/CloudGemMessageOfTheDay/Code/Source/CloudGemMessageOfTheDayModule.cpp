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

#include "CloudGemMessageOfTheDay_precompiled.h"
#include <platform_impl.h>

#include "CloudGemMessageOfTheDaySystemComponent.h"

#include <AzCore/Module/Module.h>

namespace CloudGemMessageOfTheDay
{
    class CloudGemMessageOfTheDayModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CloudGemMessageOfTheDayModule, "{797853F3-03BB-4571-A6BD-6616D89BB5E1}", AZ::Module);

        CloudGemMessageOfTheDayModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemMessageOfTheDaySystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemMessageOfTheDaySystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemMessageOfTheDay_21cba308da7040439f88fc3a45c71e18, CloudGemMessageOfTheDay::CloudGemMessageOfTheDayModule)
