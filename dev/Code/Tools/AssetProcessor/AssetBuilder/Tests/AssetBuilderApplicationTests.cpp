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
#include <AssetBuilderApplication.h>

#include <AzCore/UnitTest/TestTypes.h>

namespace AssetBuilder
{
    using namespace UnitTest;

    using AssetBuilderAppTest = AllocatorsFixture;

    TEST_F(AssetBuilderAppTest, GetAppRootArg_AssetBuilderAppNoArgs_NoExtraction)
    {
        AssetBuilderApplication app(nullptr, nullptr);

        char appRootBuffer[AZ_MAX_PATH_LEN];
        ASSERT_FALSE(app.GetOptionalAppRootArg(appRootBuffer, AZ_MAX_PATH_LEN));
    }

    TEST_F(AssetBuilderAppTest, GetAppRootArg_AssetBuilderAppQuotedAppRoot_Success)
    {
    #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* appRootArg = R"str(-approot="C:\path\to\app\root\")str";
        const char* expectedResult = R"str(C:\path\to\app\root\)str";
    #else
        const char* appRootArg = R"str(-approot="/path/to/app/root")str";
        const char* expectedResult = R"str(/path/to/app/root/)str";
    #endif

        const char* argArray[] = {
            appRootArg
        };
        int argc = AZ_ARRAY_SIZE(argArray);
        char** argv = const_cast<char**>(argArray); // this is unfortunately necessary to get around osx's strict non-const string literal stance 

        AssetBuilderApplication app(&argc, &argv);

        char appRootBuffer[AZ_MAX_PATH_LEN] = { 0 };

        ASSERT_TRUE(app.GetOptionalAppRootArg(appRootBuffer, AZ_ARRAY_SIZE(appRootBuffer)));
        ASSERT_STREQ(appRootBuffer, expectedResult);
    }

    TEST_F(AssetBuilderAppTest, GetAppRootArg_AssetBuilderAppNoQuotedAppRoot_Success)
    {
    #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        const char* appRootArg = R"str(-approot=C:\path\to\app\root\)str";
        const char* expectedResult = R"str(C:\path\to\app\root\)str";
    #else
        const char* appRootArg = R"str(-approot=/path/to/app/root)str";
        const char* expectedResult = R"str(/path/to/app/root/)str";
    #endif

        const char* argArray[] = {
            appRootArg
        };
        int argc = AZ_ARRAY_SIZE(argArray);
        char** argv = const_cast<char**>(argArray); // this is unfortunately necessary to get around osx's strict non-const string literal stance

        AssetBuilderApplication app(&argc, &argv);

        char appRootBuffer[AZ_MAX_PATH_LEN] = { 0 };

        ASSERT_TRUE(app.GetOptionalAppRootArg(appRootBuffer, AZ_ARRAY_SIZE(appRootBuffer)));
        ASSERT_STREQ(appRootBuffer, expectedResult);
    }
} // namespace AssetBuilder
