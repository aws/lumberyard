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
#include <AzCore/AzCoreModule.h>

// Component includes
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Debug/FrameProfilerComponent.h>
#include <AzCore/IO/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/RTTI/AzStdReflectionComponent.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Serialization/ObjectStreamComponent.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace AZ
{
    AzCoreModule::AzCoreModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AzStdReflectionComponent::CreateDescriptor(),
            MemoryComponent::CreateDescriptor(),
            StreamerComponent::CreateDescriptor(),
            JobManagerComponent::CreateDescriptor(),
            AssetManagerComponent::CreateDescriptor(),
            ObjectStreamComponent::CreateDescriptor(),
            UserSettingsComponent::CreateDescriptor(),
            Debug::FrameProfilerComponent::CreateDescriptor(),
            SliceComponent::CreateDescriptor(),
            SliceSystemComponent::CreateDescriptor(),
            SliceMetadataInfoComponent::CreateDescriptor(),

#if !defined(AZCORE_EXCLUDE_LUA)
            ScriptSystemComponent::CreateDescriptor(),
#endif // #if !defined(AZCORE_EXCLUDE_LUA)
        });
    }
}