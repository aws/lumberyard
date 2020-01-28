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

#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Platform/PlatformDefaults.h>

class PlatformHelperTest
    : public UnitTest::ScopedAllocatorSetupFixture
{
public:

};

TEST_F(PlatformHelperTest, SinglePlatformFlags_PlatformId_Valid)
{
    AzFramework::PlatformFlags platform = AzFramework::PlatformFlags::Platform_PC;
    AZStd::vector<AZStd::string> platforms = AzFramework::PlatformHelper::GetPlatforms(platform);
    EXPECT_EQ(platforms.size(), 1);
    EXPECT_EQ(platforms[0], "pc");
}

TEST_F(PlatformHelperTest, MultiplePlatformFlags_PlatformId_Valid)
{
    AzFramework::PlatformFlags platformFlags = AzFramework::PlatformFlags::Platform_PC | AzFramework::PlatformFlags::Platform_ES3;
    AZStd::vector<AZStd::string> platforms = AzFramework::PlatformHelper::GetPlatforms(platformFlags);
    EXPECT_EQ(platforms.size(), 2);
    EXPECT_EQ(platforms[0], "pc");
    EXPECT_EQ(platforms[1], "es3");
}

TEST_F(PlatformHelperTest, InvalidPlatformFlags_PlatformId_Empty)
{
    AZ::u32 platformFlags = 1 << 10; // Currently we do not have this bit set indicating a valid platform.
    AZStd::vector<AZStd::string> platforms = AzFramework::PlatformHelper::GetPlatforms(static_cast<AzFramework::PlatformFlags>(platformFlags));
    EXPECT_EQ(platforms.size(), 0);
}

TEST_F(PlatformHelperTest, GetPlatformName_Valid_OK)
{
    AZStd::string platformName = AzFramework::PlatformHelper::GetPlatformName(AzFramework::PlatformId::PC);
    EXPECT_EQ(platformName, AzFramework::PlatformPC);
}

TEST_F(PlatformHelperTest, GetPlatformName_Invalid_OK)
{
    AZStd::string platformName = AzFramework::PlatformHelper::GetPlatformName(static_cast<AzFramework::PlatformId>(-1));
    EXPECT_TRUE(platformName.compare("invalid") == 0);
}

TEST_F(PlatformHelperTest, GetPlatformIndexByName_Valid_OK)
{
    AZ::u32 platformIndex = AzFramework::PlatformHelper::GetPlatformIndexFromName(AzFramework::PlatformPC);
    EXPECT_EQ(platformIndex, AzFramework::PlatformId::PC);
}

TEST_F(PlatformHelperTest, GetPlatformIndexByName_Invalid_OK)
{
    AZ::u32 platformIndex = AzFramework::PlatformHelper::GetPlatformIndexFromName("dummy");
    EXPECT_EQ(platformIndex, -1);
}

TEST_F(PlatformHelperTest, GetServerPlatformIndexByName_Valid_OK)
{
    AZ::u32 platformIndex = AzFramework::PlatformHelper::GetPlatformIndexFromName(AzFramework::PlatformServer);
    EXPECT_EQ(platformIndex, AzFramework::PlatformId::SERVER);
}
