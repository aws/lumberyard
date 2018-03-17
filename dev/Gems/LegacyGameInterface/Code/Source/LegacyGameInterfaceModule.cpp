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
#include "LegacyGameInterface_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "LegacyGameInterfaceSystemComponent.h"

#include <IGem.h>

namespace LegacyGameInterface
{
    class LegacyGameInterfaceModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(LegacyGameInterfaceModule, "{203B8808-913D-4B02-9876-A2290E79DEFB}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(LegacyGameInterfaceModule, AZ::SystemAllocator, 0);

        LegacyGameInterfaceModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LegacyGameInterfaceSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LegacyGameInterfaceSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(LegacyGameInterface_3108b261962e44b6a0c1c036c693bca2, LegacyGameInterface::LegacyGameInterfaceModule)
