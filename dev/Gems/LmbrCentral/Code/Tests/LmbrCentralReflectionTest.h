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
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Application/Application.h>

/**
 * Fixture class for tests that require a module to have been reflected.
 * An AZ application is created to handle the reflection. The application
 * starts up and shuts down only once for all tests using this fixture.
 * \tparam ApplicationT Type of AZ::ComponentApplication to use.
 * \tparam ModuleT Type of AZ::Module to reflect. It must be defined within this library.
 */
template<class ApplicationT, class ModuleT>
class ModuleReflectionTest
    : public ::testing::Test
{
protected:
    static ApplicationT* GetApplication() { return s_application.get(); }

    static void SetUpTestCase();
    static void TearDownTestCase();

private:
    // We need reflection from ApplicationT and nothing more.
    // This class lets us simplify the application that we run for tests.
    class InternalApplication : public ApplicationT
    {
    public:
        // Unhide these core startup/shutdown functions (They're 'protected' in AzFramework::Application)
        using ApplicationT::Create;
        using ApplicationT::Destroy;

        // Don't create any system components.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override { return AZ::ComponentTypeList(); }
    };

    static AZStd::unique_ptr<InternalApplication> s_application;
};

/**
 * Fixture class for tests that load an object whose class is reflected within a module.
 * Upon setup, the object is loaded from a source data buffer.
 * \tparam ApplicationT Type of AZ::ComponentApplication to start.
 * \tparam ModuleT Type of AZ::Module to reflect. It must be defined within this library.
 * \tparam ObjectT Type of object to load from buffer.
 */
template<class ApplicationT, class ModuleT, class ObjectT>
class LoadReflectedObjectTest
    : public ModuleReflectionTest<ApplicationT, ModuleT>
{
    typedef ModuleReflectionTest<ApplicationT, ModuleT> BaseType;

protected:
    using BaseType::GetApplication;

    void SetUp() override;
    void TearDown() override;

    virtual const char* GetSourceDataBuffer() const = 0;

    AZStd::unique_ptr<ObjectT> m_object;
};

template<class ApplicationT, class ModuleT>
void ModuleReflectionTest<ApplicationT, ModuleT>::SetUpTestCase()
{
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create(AZ::SystemAllocator::Descriptor());

    s_application.reset(new ModuleReflectionTest::InternalApplication);

    AZ::ComponentApplication::Descriptor appDescriptor;
    appDescriptor.m_useExistingAllocator = true;

    AZ::ComponentApplication::StartupParameters appStartup;

    // ModuleT is declared within this dll.
    // Therefore, we can treat it like a statically linked module.
    appStartup.m_createStaticModulesCallback =
        [](AZStd::vector<AZ::Module*>& modules)
    {
        modules.emplace_back(new ModuleT);
    };

    // Framework application types need to have CalculateAppRoot called before
    // calling Create
    AZ::ComponentApplication* app = s_application.get();
    if (auto frameworkApplication = azrtti_cast<AzFramework::Application*>(app))
    {
        frameworkApplication->CalculateAppRoot();
    }

    // Create() starts the application and returns the system entity.
    AZ::Entity* systemEntity = s_application->Create(appDescriptor, appStartup);

    // Nothing owns the system entity, and we don't need one, so delete it.
    delete systemEntity;
}

template<class ApplicationT, class ModuleT>
void ModuleReflectionTest<ApplicationT, ModuleT>::TearDownTestCase()
{
    s_application->Destroy();
    s_application.reset();

    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
}

template<class ApplicationT, class ModuleT>
AZStd::unique_ptr<typename ModuleReflectionTest<ApplicationT, ModuleT>::InternalApplication> ModuleReflectionTest<ApplicationT, ModuleT>::s_application;

template<class ApplicationT, class ModuleT, class ObjectT>
void LoadReflectedObjectTest<ApplicationT, ModuleT, ObjectT>::SetUp()
{
    const char* buffer = GetSourceDataBuffer();
    if (buffer)
    {
        // don't load any assets referenced from the data
        AZ::ObjectStream::FilterDescriptor filter;
        filter.m_assetCB = [](const AZ::Data::Asset<AZ::Data::AssetData>&) { return false; };

        m_object.reset(AZ::Utils::LoadObjectFromBuffer<ObjectT>(buffer, strlen(buffer) + 1, this->GetApplication()->GetSerializeContext(), filter));
    }
}

template<class ApplicationT, class ModuleT, class ObjectT>
void LoadReflectedObjectTest<ApplicationT, ModuleT, ObjectT>::TearDown()
{
    m_object.reset();
}
