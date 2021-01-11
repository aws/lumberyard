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
#include "Maestro_precompiled.h"

#if !defined(_RELEASE)

#include <AnimKey.h>
#include <Maestro/Types/AssetKey.h>

#include <Cinematics/AssetTrack.h>

#include <AzTest/AzTest.h>

namespace AssetTrackTest
{
    const AZ::Data::AssetId TRACK_DEFAULT_ASSET_ID = AZ::Data::AssetId{AZ::Uuid{"{98034A57-97C5-4973-9BDB-03BD51946481}", 0}};
    const AZStd::string TRACK_ASSET_TYPE_NAME = "Asset Type Name";
    const AZ::Data::AssetId KEY0_ASSET_ID = AZ::Data::AssetId{AZ::Uuid{"{3492ABBC-65CF-407D-9742-5E1BF0310E6A}"}, 1};
    const AZ::Data::AssetId KEY1_ASSET_ID = AZ::Data::AssetId{};
    const AZ::Data::AssetId KEY2_ASSET_ID = AZ::Data::AssetId{AZ::Uuid{"{5828D2E3-09CD-4465-9AD5-7D851D1BB559}"}, 2};

    class AssetTrackTest : public ::testing::Test
    {
    public:
        AssetTrackTest();

        void CreateAssetTestKeys();

        CAssetTrack m_assetTrack;
    };

    AssetTrackTest::AssetTrackTest()
    {
        // Set the default value for the track
        m_assetTrack.SetValue(0.0f, TRACK_DEFAULT_ASSET_ID, true);
        m_assetTrack.SetAssetTypeName(TRACK_ASSET_TYPE_NAME);

        CreateAssetTestKeys();
    }

    void AssetTrackTest::CreateAssetTestKeys()
    {
        AZ::IAssetKey key0;
        key0.time = 1.0f;
        key0.m_assetId = KEY0_ASSET_ID;
        m_assetTrack.CreateKey(key0.time);
        m_assetTrack.SetKey(0, &key0);

        AZ::IAssetKey key1;
        key1.time = 2.0f;
        key1.m_assetId = KEY1_ASSET_ID;
        m_assetTrack.CreateKey(key1.time);
        m_assetTrack.SetKey(1, &key1);

        AZ::IAssetKey key2;
        key2.time = 3.0f;
        key2.m_assetId = KEY2_ASSET_ID;
        m_assetTrack.CreateKey(key2.time);
        m_assetTrack.SetKey(2, &key2);

    }

    TEST_F(AssetTrackTest, Maestro_AssetTrackTest_FT)
    {
        AZ::Data::AssetId assetId;

        m_assetTrack.GetValue(0.0f, assetId);
        ASSERT_EQ(assetId, TRACK_DEFAULT_ASSET_ID);

        m_assetTrack.GetValue(1.5f, assetId);
        ASSERT_EQ(assetId, KEY0_ASSET_ID);

        // Invalid id means use whatever the original asset id was, so we expect the default here
        m_assetTrack.GetValue(2.5f, assetId);
        ASSERT_EQ(assetId, TRACK_DEFAULT_ASSET_ID);

        m_assetTrack.GetValue(3.5f, assetId);
        ASSERT_EQ(assetId, KEY2_ASSET_ID);
    };

    TEST_F(AssetTrackTest, Maestro_AssetTrackKeyAssetTypeNameTest_FT)
    {
        ASSERT_EQ(m_assetTrack.GetNumKeys(), 3);

        for (int keyIndex = 0; keyIndex < m_assetTrack.GetNumKeys(); ++keyIndex)
        {
            AZ::IAssetKey assetKey;
            m_assetTrack.GetKey(keyIndex, &assetKey);
            ASSERT_EQ(assetKey.m_assetTypeName, TRACK_ASSET_TYPE_NAME);
        }
    }

}; // namespace AssetTrackTest

#endif // !defined(_RELEASE)