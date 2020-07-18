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

#include "WhiteBox_precompiled.h"

#include "Components/WhiteBoxColliderComponent.h"
#include "WhiteBoxAllocator.h"
#include "WhiteBoxComponent.h"
#include "WhiteBoxModule.h"
#include "WhiteBoxSystemComponent.h"

// required for linking against CryAssert/CryDebugBreak etc when using EditorCommon lib
#include <platform_impl.h>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(WhiteBoxModule, AZ::SystemAllocator, 0)

    WhiteBoxModule::WhiteBoxModule()
        : CryHooksModule()
    {
        AZ::AllocatorInstance<WhiteBoxAllocator>::Create();

        // push results of [MyComponent]::CreateDescriptor() into m_descriptors here
        m_descriptors.insert(
            m_descriptors.end(),
            {
                WhiteBoxSystemComponent::CreateDescriptor(),
                WhiteBoxColliderComponent::CreateDescriptor(),
                WhiteBoxComponent::CreateDescriptor(),
            });
    }

    WhiteBoxModule::~WhiteBoxModule()
    {
        AZ::AllocatorInstance<WhiteBoxAllocator>::Destroy();
    }

    AZ::ComponentTypeList WhiteBoxModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity.
        return AZ::ComponentTypeList{
            azrtti_typeid<WhiteBoxSystemComponent>(),
        };
    }
} // namespace WhiteBox

#if !defined(WHITE_BOX_EDITOR)
// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(WhiteBox_c5833dbda2e045d3a5f16b7414280c27, WhiteBox::WhiteBoxModule)
#endif // !defined(WHITE_BOX_EDITOR)
