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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <TwitchChatPlaySystemComponent.h>

namespace TwitchChatPlay
{
    class TwitchChatPlayModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(TwitchChatPlayModule, "{FC21CC46-D896-4370-8C77-C338735AE02D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(TwitchChatPlayModule, AZ::SystemAllocator, 0);

        TwitchChatPlayModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                TwitchChatPlaySystemComponent::CreateDescriptor(),
            });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<TwitchChatPlaySystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(TwitchChatPlay_025f343454f8495bb26bebffeb2103a6, TwitchChatPlay::TwitchChatPlayModule)
