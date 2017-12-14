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

#include "SystemComponentFixture.h"

#include <Integration/System/SystemComponent.h>
#include <Integration/AnimationBus.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace EMotionFX
{

    void SystemComponentFixture::SetUp()
    {
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_memoryBlocksByteSize = 10 * 1024 * 1024;
        mSystemEntity = mApp.Create(appDesc);

        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        AZ::Data::AssetManager::Descriptor amDesc;
        AZ::Data::AssetManager::Create(amDesc);

        AZ::IO::FileIOBase::SetInstance(&mLocalFileIO);
        const char* dir = mApp.GetExecutableFolder();
        mLocalFileIO.SetAlias("@assets@", dir);
        mLocalFileIO.SetAlias("@devassets@", dir);

        mSystemEntity->CreateComponent<EMotionFX::Integration::SystemComponent>();
        mSystemEntity->Init();
        mSystemEntity->Activate();
    }

    void SystemComponentFixture::TearDown()
    {
        // Clean things up in the reverse order that things happened in SetUp
        mSystemEntity->Deactivate();

        AZ::IO::FileIOBase::SetInstance(nullptr);

        AZ::Data::AssetManager::Destroy();

        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

        // Delete global static environment variables used for busses
        EMotionFX::Integration::ActorNotificationBus::StoragePolicy::s_defaultGlobalContext.Reset();
        EMotionFX::Integration::EMotionFXRequestBus::StoragePolicy::s_defaultGlobalContext.Reset();
        EMotionFX::Integration::SystemNotificationBus::StoragePolicy::s_defaultGlobalContext.Reset();
        EMotionFX::Integration::SystemRequestBus::StoragePolicy::s_defaultGlobalContext.Reset();

        // Don't call Destroy on mApp; its destructor will do that for us. We want
        // the OSAllocator that the ComponentApplication made to stick around
        // during the destructor of the LocalFileIO instance.
    }

} // end namespace EMotionFX
