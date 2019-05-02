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

#include "CryLegacyAnimation_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "CryLegacyAnimationSystemComponent.h"

#include <IGem.h>

namespace CryLegacyAnimation
{
    class CryLegacyAnimationModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CryLegacyAnimationModule, "{C63A0203-24A1-4375-852B-CBD1827B937E}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(CryLegacyAnimationModule, AZ::SystemAllocator, 0);

        CryLegacyAnimationModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                CryLegacyAnimationSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<CryLegacyAnimationSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(CryLegacyAnimation_b4807336b070494d974275a581593ed8, CryLegacyAnimation::CryLegacyAnimationModule)
