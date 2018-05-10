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
#include "LegacyTimeDemoRecorder_precompiled.h"
#include <platform_impl.h>

#include <AzCore/Memory/SystemAllocator.h>

#include "LegacyTimeDemoRecorderSystemComponent.h"

#include <IGem.h>

namespace LegacyTimeDemoRecorder
{
    class LegacyTimeDemoRecorderModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(LegacyTimeDemoRecorderModule, "{990D14D8-02AC-41C3-A0D2-247B650FCDA1}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(LegacyTimeDemoRecorderModule, AZ::SystemAllocator, 0);

        LegacyTimeDemoRecorderModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LegacyTimeDemoRecorderSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LegacyTimeDemoRecorderSystemComponent>(),
            };
        }
    };
}

#ifndef AZ_MONOLITHIC_BUILD
#include "Common_TypeInfo.h"
#if defined(AZ_RESTRICTED_PLATFORM)
#include AZ_RESTRICTED_FILE(LegacyTimeDemoRecorderModule_cpp, AZ_RESTRICTED_PLATFORM)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#if !defined(LINUX) && !defined(APPLE)
STRUCT_INFO_T_INSTANTIATE(Color_tpl, <float>)
#endif
STRUCT_INFO_T_INSTANTIATE(Quat_tpl, <float>)
#endif
#endif // AZ_MONOLITHIC_BUILD

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(LegacyTimeDemoRecorder_861c511671ac4454a835fac9e588f36a, LegacyTimeDemoRecorder::LegacyTimeDemoRecorderModule)
