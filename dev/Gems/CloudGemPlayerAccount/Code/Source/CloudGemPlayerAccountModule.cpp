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

#include "CloudGemPlayerAccount_precompiled.h"
#include <platform_impl.h>

#include "CloudGemPlayerAccountSystemComponent.h"

#include <AzCore/Module/Module.h>

namespace CloudGemPlayerAccount
{
    class CloudGemPlayerAccountModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(CloudGemPlayerAccountModule, "{2007F458-16CB-4C8D-A68C-75E1885FEDA5}", AZ::Module);

        CloudGemPlayerAccountModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CloudGemPlayerAccountSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CloudGemPlayerAccountSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CloudGemPlayerAccount_fd4ea4ff80a64bb9a90e55b46e9539ef, CloudGemPlayerAccount::CloudGemPlayerAccountModule)
