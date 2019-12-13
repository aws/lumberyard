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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzQtComponents/Utilities/QtPluginPaths.h>
#include <QApplication>

namespace Physics
{
    class PhysXEditorTestEnvironment
        : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TeardownEnvironment() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

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

    TEST_F(PhysXEditorTest, EditorDummyTest_NoState_TrivialPass)
    {
        EXPECT_TRUE(true);
    }

    AZTEST_EXPORT int AZ_UNIT_TEST_HOOK_NAME(int argc, char** argv)
    {
        ::testing::InitGoogleMock(&argc, argv);
        AzQtComponents::PrepareQtPaths();
        QApplication app(argc, argv);
        AZ::Test::excludeIntegTests();
        AZ::Test::ApplyGlobalParameters(&argc, argv);
        AZ::Test::printUnusedParametersWarning(argc, argv);
        AZ::Test::addTestEnvironments({ new PhysXEditorTestEnvironment });
        int result = RUN_ALL_TESTS();
        return result;
    }
} // namespace Physics
#endif // AZ_TESTS_ENABLED
