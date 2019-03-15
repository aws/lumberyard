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

#include "PhysX_precompiled.h"

#ifdef AZ_TESTS_ENABLED

#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <Tests/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>

namespace Physics
{
    class PhysXEditorTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override;
        void TeardownEnvironment() override;

        AZ::ComponentApplication* m_application;
        AZ::Entity* m_systemEntity;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZ::IO::LocalFileIO m_fileIo;
    };

    class PhysXEditorTest
        : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
        }

        void TearDown() override
        {
        }
    };

    void PhysXEditorTestEnvironment::SetupEnvironment()
    {
        AZ::IO::FileIOBase::SetInstance(&m_fileIo);

        AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

        // Create application and descriptor
        m_application = aznew AZ::ComponentApplication;
        AZ::ComponentApplication::Descriptor appDesc;
        appDesc.m_useExistingAllocator = true;

        // Set up gems for loading
        AZ::DynamicModuleDescriptor dynamicModuleDescriptor;
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Gem.PhysX.4e08125824434932a0fe3717259caa47.v0.1.0";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        dynamicModuleDescriptor = AZ::DynamicModuleDescriptor();
        dynamicModuleDescriptor.m_dynamicLibraryPath = "Gem.LmbrCentral.ff06785f7145416b9d46fde39098cb0c.v0.1.0";
        appDesc.m_modules.push_back(dynamicModuleDescriptor);

        // Create system entity
        AZ::ComponentApplication::StartupParameters startupParams;
        m_systemEntity = m_application->Create(appDesc, startupParams);
        AZ_TEST_ASSERT(m_systemEntity);
        m_systemEntity->AddComponent(aznew AZ::MemoryComponent());
        m_systemEntity->AddComponent(aznew AZ::AssetManagerComponent());
        m_systemEntity->Init();
        m_systemEntity->Activate();

        // Set up transform component descriptor
        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
        m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
    }

    void PhysXEditorTestEnvironment::TeardownEnvironment()
    {
        m_transformComponentDescriptor.reset();
        m_serializeContext.reset();
        delete m_application;
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }

    TEST_F(PhysXEditorTest, EditorDummyTest_NoState_TrivialPass)
    {
        EXPECT_TRUE(true);
    }

    AZ_UNIT_TEST_HOOK(new PhysXEditorTestEnvironment);
} // namespace Physics
#endif // AZ_TESTS_ENABLED
