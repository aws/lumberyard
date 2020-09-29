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
#include <source/utils/utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/StringFunc/StringFunc.h>

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
static const char* TEST_ENGINE_DIR = "Z:\\Dummy";
#else
static const char* TEST_ENGINE_DIR = "/Dummy";
#endif


namespace AssetBundler
{
    class MockUtilsTest
        : public UnitTest::ScopedAllocatorSetupFixture
        , public AzFramework::ApplicationRequests::Bus::Handler
    {
    public:
        void SetUp() override
        {
            ScopedAllocatorSetupFixture::SetUp();
            AzFramework::ApplicationRequests::Bus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AzFramework::ApplicationRequests::Bus::Handler::BusDisconnect();
            ScopedAllocatorSetupFixture::TearDown();
        }

        // AzFramework::ApplicationRequests::Bus::Handler interface
        void NormalizePath(AZStd::string& /*path*/) override {}
        void NormalizePathKeepCase(AZStd::string& /*path*/) override {}
        void CalculateBranchTokenForAppRoot(AZStd::string& /*token*/) const override {}
        void QueryApplicationType(AzFramework::ApplicationTypeQuery& /*appType*/) const override {}

        const char* GetAppRoot() const  override
        {
            return TEST_ENGINE_DIR;
        }
    };


    TEST_F(MockUtilsTest, TestFilePath_StartsWithAFileSeparator_Valid)
    {
        AZStd::string relFilePath = "Foo/foo.xml";
        AzFramework::StringFunc::Prepend(relFilePath, AZ_CORRECT_FILESYSTEM_SEPARATOR);
        AZStd::string absoluteFilePath;
#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        AzFramework::StringFunc::Path::ConstructFull("Z:", relFilePath.c_str(), absoluteFilePath, true);
#else
        absoluteFilePath = relFilePath;
#endif
        FilePath filePath(relFilePath);
        EXPECT_EQ(filePath.AbsolutePath(), absoluteFilePath);
    }


    TEST_F(MockUtilsTest, TestFilePath_RelativePath_Valid)
    {
        AZStd::string relFilePath = "Foo\\foo.xml";
        AZStd::string absoluteFilePath;
        AzFramework::StringFunc::Path::ConstructFull(TEST_ENGINE_DIR, relFilePath.c_str(), absoluteFilePath, true);
        FilePath filePath(relFilePath);
        EXPECT_EQ(filePath.AbsolutePath(), absoluteFilePath);
    }

    TEST_F(MockUtilsTest, LooksLikeWildcardPattern_IsWildcardPattern_ExpectTrue)
    {
        EXPECT_TRUE(LooksLikeWildcardPattern("*"));
        EXPECT_TRUE(LooksLikeWildcardPattern("?"));
        EXPECT_TRUE(LooksLikeWildcardPattern("*/*"));
        EXPECT_TRUE(LooksLikeWildcardPattern("*/test?/*.xml"));
    }

    TEST_F(MockUtilsTest, LooksLikeWildcardPattern_IsNotWildcardPattern_ExpectFalse)
    {
        EXPECT_FALSE(LooksLikeWildcardPattern(""));
        EXPECT_FALSE(LooksLikeWildcardPattern("test"));
        EXPECT_FALSE(LooksLikeWildcardPattern("test/path.xml"));
    }
}
