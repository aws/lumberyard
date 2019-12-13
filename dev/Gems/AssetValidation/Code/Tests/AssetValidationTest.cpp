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

#include "AssetValidationTestShared.h"
#include <AssetValidationSystemComponent.h>
#include <AzFramework/Platform/PlatformDefaults.h>
#include <AzFramework/Asset/AssetSeedList.h>


TEST_F(AssetValidationTest, SeedListDependencyTest_AddAndRemoveList_Success)
{
    MockValidationComponent testComponent;

    AzFramework::AssetSeedList seedList;
    const char DummyAssetFile[] = "Dummy";
    seedList.push_back({ testComponent.m_assetIds[6], AzFramework::PlatformFlags::Platform_PC, DummyAssetFile });

    testComponent.TestAddSeedsFor(seedList, 2);
    testComponent.SeedMode();

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

    testComponent.TestRemoveSeedsFor(seedList, 0);

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

    testComponent.TestRemoveSeedsFor(seedList, 2);

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);
}


TEST_F(AssetValidationTest, SeedAssetDependencyTest_AddSingleAsset_DependenciesFound)
{
    MockValidationComponent testComponent;
    testComponent.AddSeedAssetId(testComponent.m_assetIds[8], 0);
    testComponent.SeedMode();

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath7"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);
}

TEST_F(AssetValidationTest, SeedListDependencyTest_AddAndRemoveListPaths_Success)
{
    MockValidationComponent testComponent;

    const char seedListPathName[] = "ValidPath";
    const char invalidSeedListPathName[] = "InvalidPath";

    AzFramework::AssetSeedList seedList;
    const char DummyAssetFile[] = "Dummy";
    seedList.push_back({ testComponent.m_assetIds[6], AzFramework::PlatformFlags::Platform_PC, DummyAssetFile });

    testComponent.m_validSeedList = seedList;
    testComponent.m_validSeedPath = seedListPathName;

    testComponent.TestAddSeedList(invalidSeedListPathName);
    testComponent.SeedMode();

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

    testComponent.TestAddSeedList(seedListPathName);

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

    testComponent.TestRemoveSeedList(invalidSeedListPathName);

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), true);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);

    testComponent.TestRemoveSeedList(seedListPathName);

    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath5"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath6"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath8"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath9"), false);
    EXPECT_EQ(testComponent.IsKnownAsset("AssetPath10"), false);
}


AZ_UNIT_TEST_HOOK();
