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

#include "CloudGemLeaderboard_precompiled.h"
#include <platform_impl.h>

#include "CloudGemLeaderboardSystemComponent.h"

#include <AzCore/Module/Module.h>

namespace CloudGemLeaderboard
{
    class CloudGemLeaderboardModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CloudGemLeaderboardModule, "{AD272FCE-0082-42EB-B251-8C6EA063778B}", AZ::Module);

        CloudGemLeaderboardModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemLeaderboardSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemLeaderboardSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemLeaderboard_a6db8cb7bfd645509f538bcb0ddc5343, CloudGemLeaderboard::CloudGemLeaderboardModule)
