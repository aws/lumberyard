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

#include "native/utilities/AssetUtils.h"

#include <QDir>

using namespace AssetUtilities;

class AssetUtilitiesTest
    : public AssetProcessor::AssetProcessorTest
{
};

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathRelPath_Valid)
{
    QString result = NormalizeFilePath("a/b\\c\\d/E.txt");
    ASSERT_TRUE(result.compare("a/b/c/d/E.txt", Qt::CaseSensitive) == 0);
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidPathFullPath_Valid)
{
    QString result = NormalizeFilePath("c:\\a/b\\c\\d/E.txt");
    ASSERT_TRUE(result.compare("c:/a/b/c/d/E.txt", Qt::CaseSensitive) == 0);
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirRelPath_Valid)
{
    QString result = NormalizeDirectoryPath("a/b\\c\\d");
    ASSERT_TRUE(result.compare("a/b/c/d", Qt::CaseSensitive) == 0);
}

TEST_F(AssetUtilitiesTest, NormlizeFilePath_NormalizedValidDirFullPath_Valid)
{
    QString result = NormalizeDirectoryPath("c:\\a/b\\c\\d\\");
    ASSERT_TRUE(result.compare("c:/a/b/c/d", Qt::CaseSensitive) == 0);
}

TEST_F(AssetUtilitiesTest, GetBranchToken_Basic_Valid)
{
    QString branchToken = GetBranchToken();
    ASSERT_GT(branchToken.size(), 0);
}


