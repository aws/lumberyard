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

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Asset/AssetBundler.h>

#include <source/utils/utils.h>
#include <source/utils/applicationManager.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <tests/main.h>

namespace AssetBundler
{
    const char DummyProjectName[] = "DummyProject";

    class MockApplicationManagerTest
        : public AssetBundler::ApplicationManager
    {
    public:
        friend class GTEST_TEST_CLASS_NAME_(ApplicationManagerTest, ValidatePlatformFlags_ReadConfigFiles_OK);
        explicit MockApplicationManagerTest(int* argc, char*** argv)
            : ApplicationManager(argc, argv)
        {
        }

    };

    class BasicApplicationManagerTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {

    };

    class ApplicationManagerTest
        : public UnitTest::ScopedAllocatorSetupFixture
    {
    public:
        
        void SetUp() override
        {
            UnitTest::ScopedAllocatorSetupFixture::SetUp();
            m_data = AZStd::make_unique<StaticData>();

            m_data->m_applicationManager.reset(aznew MockApplicationManagerTest(0, 0));
            m_data->m_applicationManager->Start(AzFramework::Application::Descriptor());

            const char* engineRoot = nullptr;
            AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
            ASSERT_TRUE(engineRoot) << "Unable to locate engine root.\n";
            AzFramework::StringFunc::Path::Join(engineRoot, RelativeTestFolder, m_data->m_testEngineRoot);

            m_data->m_localFileIO = aznew AZ::IO::LocalFileIO();
            m_data->m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
            // we need to set it to nullptr first because otherwise the 
            // underneath code assumes that we might be leaking the previous instance
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_data->m_localFileIO);

        }
        void TearDown() override
        {
            AZ::IO::FileIOBase::SetInstance(nullptr);
            delete m_data->m_localFileIO;
            AZ::IO::FileIOBase::SetInstance(m_data->m_priorFileIO);

            m_data->m_applicationManager->Stop();
            m_data->m_applicationManager.reset();
            m_data.reset();
            UnitTest::ScopedAllocatorSetupFixture::TearDown();
        }

        struct StaticData
        {
            AZStd::unique_ptr<MockApplicationManagerTest> m_applicationManager = {};
            AZ::IO::FileIOBase* m_priorFileIO = nullptr;
            AZ::IO::FileIOBase* m_localFileIO = nullptr;
            AZStd::string m_testEngineRoot;
        };

        AZStd::unique_ptr<StaticData> m_data;
    };

    TEST_F(ApplicationManagerTest, ValidatePlatformFlags_ReadConfigFiles_OK)
    {
        AzToolsFramework::AssetUtils::GetGemsInfo(m_data->m_testEngineRoot.c_str(), m_data->m_testEngineRoot.c_str(), DummyProjectName, m_data->m_applicationManager->m_gemInfoList);
        EXPECT_EQ(m_data->m_applicationManager->m_gemInfoList.size(), 3);
        AZStd::unordered_set<AZStd::string> gemsNameMap {"GemA", "GemB", "GemC"};
        for (const AzToolsFramework::AssetUtils::GemInfo& gemInfo : m_data->m_applicationManager->m_gemInfoList)
        {
            gemsNameMap.erase(gemInfo.m_gemName);
        }

        ASSERT_EQ(gemsNameMap.size(), 0);

        AzFramework::PlatformFlags platformFlags = GetEnabledPlatformFlags(m_data->m_testEngineRoot.c_str(), m_data->m_testEngineRoot.c_str(), DummyProjectName);
        AzFramework::PlatformFlags hostPlatformFlag = AzFramework::PlatformHelper::GetPlatformFlag(AzToolsFramework::AssetSystem::GetHostAssetPlatform());
        AzFramework::PlatformFlags expectedFlags = AzFramework::PlatformFlags::Platform_ES3 | AzFramework::PlatformFlags::Platform_IOS | AzFramework::PlatformFlags::Platform_PROVO | hostPlatformFlag;
        ASSERT_EQ(platformFlags, expectedFlags);
    }

    TEST_F(BasicApplicationManagerTest, ComputeComparisonTypeFromString_InvalidString_Fails)
    {
        auto invalidResult = AssetBundler::ParseComparisonType("notacomparisontype");
        EXPECT_EQ(invalidResult.IsSuccess(), false);
    }

    TEST_F(BasicApplicationManagerTest, ComputeComparisonTypeFromString_ValidString_Success)
    {
        auto deltaResult = AssetBundler::ParseComparisonType(AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Delta)]);
        EXPECT_EQ(deltaResult.IsSuccess(), true);
        EXPECT_EQ(deltaResult.GetValue(), AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Delta);
        auto unionResult = AssetBundler::ParseComparisonType(AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Union)]);
        EXPECT_EQ(unionResult.IsSuccess(), true);
        EXPECT_EQ(unionResult.GetValue(), AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Union);
        auto intersectionResult = AssetBundler::ParseComparisonType(AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Intersection)]);
        EXPECT_EQ(intersectionResult.IsSuccess(), true);
        EXPECT_EQ(intersectionResult.GetValue(), AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Intersection);
        auto complementResult = AssetBundler::ParseComparisonType(AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Complement)]);
        EXPECT_EQ(complementResult.IsSuccess(), true);
        EXPECT_EQ(complementResult.GetValue(), AzToolsFramework::AssetFileInfoListComparison::ComparisonType::Complement);
        auto filePatternResult = AssetBundler::ParseComparisonType(AzToolsFramework::AssetFileInfoListComparison::ComparisonTypeNames[aznumeric_cast<AZ::u8>(AzToolsFramework::AssetFileInfoListComparison::ComparisonType::FilePattern)]);
        EXPECT_EQ(filePatternResult.IsSuccess(), true);
        EXPECT_EQ(filePatternResult.GetValue(), AzToolsFramework::AssetFileInfoListComparison::ComparisonType::FilePattern);
    }
}
