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

#include <platform_impl.h>

#include "FootstepComponent.h"

#include <IGem.h>

namespace Footsteps
{
    class FootstepsModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(FootstepsModule, "{D0DF3F65-CF4D-4E96-849E-FFDB10543E47}", CryHooksModule);

        FootstepsModule()
            : CryHooksModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                FootstepComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{};
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Footsteps_257393f09006498ab706cc4d4c8d4721, Footsteps::FootstepsModule)
