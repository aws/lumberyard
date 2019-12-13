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

#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>

#include <Integration/System/SystemComponent.h>
#include <Integration/AnimationBus.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Jobs/JobManagerComponent.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace EMotionFX
{
    //! A fixture that constructs an EMotionFX::Integration::SystemComponent
    /*!
     * This fixture can be used by any test that needs the EMotionFX runtime to
     * be working. It will construct all necessary allocators for EMotionFX
     * objects to be successfully instantiated.
    */
    template<class... Components>
    class ComponentFixture : public UnitTest::AllocatorsTestFixture
    {
    public:
        void SetUp() override
        {
            UnitTest::AllocatorsTestFixture::SetUp();

            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_enableDrilling = false;
            mSystemEntity = mApp.Create(appDesc);

            mLocalFileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();

            AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

            AZ::IO::FileIOBase::SetInstance(mLocalFileIO.get());

            m_workingDirectory = mApp.GetExecutableFolder();
            const AZStd::string assetFolder = GetAssetFolder();

            mLocalFileIO->SetAlias("@root@", m_workingDirectory);
            mLocalFileIO->SetAlias("@assets@", assetFolder.c_str());
            mLocalFileIO->SetAlias("@devassets@", assetFolder.c_str());

            Activate();
        }

        void TearDown() override
        {
            // If we loaded the asset catalog, call this function to release all the assets that has been loaded internally.
            if (mSystemEntity->FindComponent<AzFramework::AssetCatalogComponent>())
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
            }

            Deactivate();

            // Clean things up in the reverse order that things happened in SetUp
            AZ::IO::FileIOBase::SetInstance(nullptr);

            mLocalFileIO.reset();

            AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();
            AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();

            mApp.Destroy();

            UnitTest::AllocatorsTestFixture::TearDown();
        }

        AZ::SerializeContext* GetSerializeContext() const
        {
            return m_serializeContext;
        }

        AZStd::string ResolvePath(const char* path)
        {
            AZStd::string result;
            result.resize(AZ::IO::MaxPathLength);
            AZ::IO::LocalFileIO::GetInstance()->ResolvePath(path, result.data(), result.size());
            return result;
        }

        const char* GetWorkingDirectory()
        {
            return m_workingDirectory;
        }

    protected:

        virtual AZStd::string GetAssetFolder() const
        {
            return mApp.GetExecutableFolder();
        }

        virtual void Activate()
        {
            // Poor-man's c++11 fold expression
            std::initializer_list<int> {(mApp.RegisterComponentDescriptor(Components::CreateDescriptor()), 0)...};
            std::initializer_list<int> {(mSystemEntity->CreateComponent<Components>(), 0)...};
            mSystemEntity->Init();
            mSystemEntity->Activate();

            m_serializeContext = mApp.GetSerializeContext();
            m_serializeContext->CreateEditContext();

            if (mSystemEntity->FindComponent<AzFramework::AssetCatalogComponent>())
            {
                AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::LoadCatalog, "@assets@/assetcatalog.xml");
            }
        }

        virtual void Deactivate()
        {
            m_serializeContext->DestroyEditContext();
            // Clear the queue of messages from unit tests on our buses
            EMotionFX::Integration::ActorNotificationBus::ClearQueuedEvents();

            mSystemEntity->Deactivate();
            // Remove SceneAPI components before the DLL is uninitialized
            std::initializer_list<int> {(([&]() {
                auto component = mSystemEntity->FindComponent<Components>();
                mSystemEntity->RemoveComponent(component);
                delete component;
            })(), 0)...};

            std::initializer_list<int> {(mApp.UnregisterComponentDescriptor(Components::CreateDescriptor()), 0)...};
        }

        // The ComponentApplication must not be a pointer, because it cannot be
        // dynamically allocated. Calls to new will try to use the SystemAllocator
        // that has not been created yet. If one is created before
        // ComponentApplication::Create() is called, ComponentApplication will
        // complain that there's already a SystemAllocator, as it tries to make one
        // itself.
        AZ::ComponentApplication mApp;

    private:
        // The destructor of the LocalFileIO object uses the AZ::OSAllocator. Make
        // sure that it still exists when this fixture is destroyed.
        AZStd::unique_ptr<AZ::IO::LocalFileIO> mLocalFileIO;

        AZ::Entity* mSystemEntity = nullptr;

        AZ::SerializeContext* m_serializeContext = nullptr;

        const char* m_workingDirectory = nullptr;
    };

    // Note that the SystemComponent depends on the AssetManagerComponent
    using SystemComponentFixture = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        EMotionFX::Integration::SystemComponent
    >;

    // Use this fixture if you want to load asset catalog. Some assets (reference anim graph for example)
    // can only be loaded when asset catalog is loaded.
    using SystemComponentFixtureWithCatalog = ComponentFixture<
        AzFramework::AssetCatalogComponent,
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        EMotionFX::Integration::SystemComponent
    >;
} // end namespace EMotionFX
