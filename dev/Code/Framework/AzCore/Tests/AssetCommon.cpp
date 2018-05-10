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
#include "TestTypes.h"
#include <AzCore/Asset/AssetCommon.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Data;

    TEST_F(AllocatorsFixture, AssetPreserveHintTest_Const_Copy)
    {
        // test to make sure that when we copy asset<T>s around using copy operations
        // that the asset Hint is preserved in the case of assets being copied from things missing asset hints.
        AssetId id("{52C79B55-B5AA-4841-AFC8-683D77716287}", 1);
        AssetId idWithDifferentAssetId("{EA554205-D887-4A01-9E39-A318DDE4C0FC}", 1);
        
        AssetType typeOfExample("{A99E8722-1F1D-4CA9-B89B-921EB3D907A9}");
        Asset<AssetData> assetWithHint(id, typeOfExample, "an asset hint");
        Asset<AssetData> differentAssetEntirely(idWithDifferentAssetId, typeOfExample, "");
        Asset<AssetData> sameAssetWithoutHint(id, typeOfExample, "");
        Asset<AssetData> sameAssetWithDifferentHint(id, typeOfExample, "a different hint");

        // if we copy an asset from one with the same id, but no hint, preserve the sources hint.
        assetWithHint = sameAssetWithoutHint;
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "an asset hint");

        // if we copy from an asset with same id, but with a different hint, we do not preserve the hint.
        assetWithHint = sameAssetWithDifferentHint;
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "a different hint");

        // if we copy different assets (different id or sub) the hint must be copied.
        // even if its empty.
        assetWithHint = Asset<AssetData>(id, typeOfExample, "an asset hint");
        assetWithHint = differentAssetEntirely;
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "");

        // ensure copy construction copies the hint.
        Asset<AssetData> copied(sameAssetWithDifferentHint);
        ASSERT_STREQ(copied.GetHint().c_str(), "a different hint");
    }

    TEST_F(AllocatorsFixture, AssetPreserveHintTest_Rvalue_Ref_Move)
    {
        // test to make sure that when we move asset<T>s around using move operators 
        // that the asset Hint is preserved in the case of assets being moved from things missing asset hints.
        AssetId id("{52C79B55-B5AA-4841-AFC8-683D77716287}", 1);
        AssetId idWithDifferentAssetId("{EA554205-D887-4A01-9E39-A318DDE4C0FC}", 1);

        AssetType typeOfExample("{A99E8722-1F1D-4CA9-B89B-921EB3D907A9}");
        Asset<AssetData> assetWithHint(id, typeOfExample, "an asset hint");
        Asset<AssetData> differentAssetEntirely(idWithDifferentAssetId, typeOfExample, "");
        Asset<AssetData> sameAssetWithoutHint(id, typeOfExample, "");
        Asset<AssetData> sameAssetWithDifferentHint(id, typeOfExample, "a different hint");

        // if we move an asset from one with the same id, but no hint, preserve the sources hint.
        assetWithHint = AZStd::move(sameAssetWithoutHint);
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "an asset hint");

        // if we move from an asset with same id, but with a different hint, we do not preserve the hint.
        assetWithHint = AZStd::move(sameAssetWithDifferentHint);
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "a different hint");

        // if we move different assets (different id or sub) the hint must be copied.
        // even if its empty.
        assetWithHint = Asset<AssetData>(id, typeOfExample, "an asset hint");
        assetWithHint = AZStd::move(differentAssetEntirely);
        ASSERT_STREQ(assetWithHint.GetHint().c_str(), "");

        // ensure move construction copies the hint.
        Asset<AssetData> copied(Asset<AssetData>(id, typeOfExample, "a different hint"));
        ASSERT_STREQ(copied.GetHint().c_str(), "a different hint");
    }
}

