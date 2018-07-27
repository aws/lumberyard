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

#include "native/tests/AssetProcessorTest.h"

#include "native/utilities/assetUtils.h"

#include <QDir>

using namespace AssetUtilities;

class AssetUtilitiesTest
    : public AssetProcessor::AssetProcessorTest
{
};

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathRelPath_Valid)
{
    QString result = NormalizeFilePath("a/b\\c\\d/E.txt");
    EXPECT_STREQ(result.toUtf8().constData(), "a/b/c/d/E.txt");
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathFullPath_Valid)
{
    QString result = NormalizeFilePath("c:\\a/b\\c\\d/E.txt");
    // on windows, drive letters are normalized to full
#if defined(AZ_PLATFORM_WINDOWS)
    ASSERT_TRUE(result.compare("C:/a/b/c/d/E.txt", Qt::CaseSensitive) == 0);
#else
    // on other platforms, C: is a relative path to a file called 'c:')
    EXPECT_STREQ(result.toUtf8().constData(), "c:/a/b/c/d/E.txt");
#endif
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirRelPath_Valid)
{
    QString result = NormalizeDirectoryPath("a/b\\c\\D");
    EXPECT_STREQ(result.toUtf8().constData(), "a/b/c/D");
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirFullPath_Valid)
{
    QString result = NormalizeDirectoryPath("c:\\a/b\\C\\d\\");

    // on windows, drive letters are normalized to full
#if defined(AZ_PLATFORM_WINDOWS)
    EXPECT_STREQ(result.toUtf8().constData(), "C:/a/b/C/d");
#else
    EXPECT_STREQ(result.toUtf8().constData(), "c:/a/b/C/d");
#endif
}

TEST_F(AssetUtilitiesTest, ComputeCRC32Lowercase_IsCaseInsensitive)
{
    const char* upperCaseString = "HELLOworld";
    const char* lowerCaseString = "helloworld";

    EXPECT_EQ(AssetUtilities::ComputeCRC32Lowercase(lowerCaseString), AssetUtilities::ComputeCRC32Lowercase(upperCaseString));

    // also try the length-based one.
    EXPECT_EQ(AssetUtilities::ComputeCRC32Lowercase(lowerCaseString, size_t(5)), AssetUtilities::ComputeCRC32Lowercase(upperCaseString, size_t(5)));
}

TEST_F(AssetUtilitiesTest, ComputeCRC32_IsCaseSensitive)
{
    const char* upperCaseString = "HELLOworld";
    const char* lowerCaseString = "helloworld";

    EXPECT_NE(AssetUtilities::ComputeCRC32(lowerCaseString), AssetUtilities::ComputeCRC32(upperCaseString));

    // also try the length-based one.
    EXPECT_NE(AssetUtilities::ComputeCRC32(lowerCaseString, size_t(5)), AssetUtilities::ComputeCRC32(upperCaseString, size_t(5)));
}