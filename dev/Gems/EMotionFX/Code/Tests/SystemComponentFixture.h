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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Asset/AssetCatalogComponent.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <Integration/System/SystemComponent.h>
#include <Integration/AnimationBus.h>


namespace EMotionFX
{
    template<class... Components>
    class EMotionFXTestModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(EMotionFXTestModule, "{32567457-5341-4D8D-91A9-E48D8395DE65}", AZ::Module);
        AZ_CLASS_ALLOCATOR(EMotionFXTestModule, AZ::OSAllocator, 0);

        EMotionFXTestModule()
        {
            m_descriptors.insert(m_descriptors.end(), {Components::CreateDescriptor()...});
        }
    };

    template<class... Components>
    class ComponentFixtureApp
        : public AzFramework::Application
    {
    public:

        ComponentFixtureApp()
        {
            m_defaultFileIO = AZStd::make_unique<AZ::IO::LocalFileIO>();
            AZ::IO::FileIOBase::SetInstance(m_defaultFileIO.get());
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return {azrtti_typeid<Components>()...};
        }

        void CreateStaticModules(AZStd::vector<AZ::Module*>& outModules) override
        {
            // This is intentionally bypassing the static modules that
            // AzFramework::Application would create. That creates more
            // components than these tests need.
            outModules.emplace_back(aznew EMotionFXTestModule<Components...>());
        }

        void StartCommon(AZ::Entity* systemEntity) override
        {
            m_systemEntity = systemEntity;
            AzFramework::Application::StartCommon(systemEntity);
        }

        AZ::Entity* GetSystemEntity() const
        {
            return m_systemEntity;
        }

    private:
        AZ::Entity* m_systemEntity = nullptr;
    };

    //! A fixture that constructs an EMotionFX::Integration::SystemComponent
    /*!
     * This fixture can be used by any test that needs the EMotionFX runtime to
     * be working. It will construct all necessary allocators for EMotionFX
     * objects to be successfully instantiated.
    */
    template<class... Components>
    class ComponentFixture
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:

        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();

            auto localFileIO = AZ::IO::FileIOBase::GetInstance();
            localFileIO->SetAlias("@root@", GetWorkingDirectory());
            localFileIO->SetAlias("@assets@", GetAssetFolder().c_str());
            localFileIO->SetAlias("@devassets@", GetAssetFolder().c_str());

            PreStart();

            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_createEditContext = true;

            m_app.Start(AZ::ComponentApplication::Descriptor{}, startupParameters);

            GetSerializeContext()->CreateEditContext();
        }

        void TearDown() override
        {
            // If we loaded the asset catalog, call this function to release all the assets that has been loaded internally.
            if (HasComponentType<AzFramework::AssetCatalogComponent>())
            {
                AZ::Data::AssetManager::Instance().DispatchEvents();
            }

            GetSerializeContext()->DestroyEditContext();
            // Clear the queue of messages from unit tests on our buses
            EMotionFX::Integration::ActorNotificationBus::ClearQueuedEvents();

            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

        ~ComponentFixture() override
        {
            if (GetSystemEntity()->GetState() == AZ::Entity::ES_ACTIVE)
            {
                GetSystemEntity()->Deactivate();
            }
        }

        AZ::SerializeContext* GetSerializeContext()
        {
            return m_app.GetSerializeContext();
        }

        AZ::Entity* GetSystemEntity() const
        {
            return m_app.GetSystemEntity();
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
            return m_app.GetExecutableFolder();
        }

    protected:

        virtual AZStd::string GetAssetFolder() const
        {
            return m_app.GetExecutableFolder();
        }

        // Runs after allocators are set up but before application startup
        // Used by the InitSceneAPI fixture to load the SceneAPI dlls
        virtual void PreStart() {}

        template<typename T>
        static constexpr bool HasComponentType()
        {
            // Return true if T is somewhere in the Components parameter pack
            // This expands to
            // return (((AZStd::is_same_v<Components[0], T> || AZStd::is_same_v<Components[1], T>) || AZStd::is_same_v<Components[2], T>) || ...)
            return ((... || AZStd::is_same_v<Components, T>));
        }

    protected:
        // The ComponentApplication must not be a pointer, because it cannot be
        // dynamically allocated. Calls to new will try to use the SystemAllocator
        // that has not been created yet. If one is created before
        // ComponentApplication::Create() is called, ComponentApplication will
        // complain that there's already a SystemAllocator, as it tries to make one
        // itself.
        ComponentFixtureApp<Components...> m_app;
    };

    using SystemComponentFixture = ComponentFixture<
        AZ::MemoryComponent,
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        EMotionFX::Integration::SystemComponent
    >;

    // Use this fixture if you want to load asset catalog. Some assets (reference anim graph for example)
    // can only be loaded when asset catalog is loaded.
    using SystemComponentFixtureWithCatalog = ComponentFixture<
        AZ::MemoryComponent,
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AzFramework::AssetCatalogComponent,
        EMotionFX::Integration::SystemComponent
    >;
} // end namespace EMotionFX
