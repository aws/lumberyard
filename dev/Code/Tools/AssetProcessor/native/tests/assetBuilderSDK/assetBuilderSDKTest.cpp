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
#include "assetBuilderSDKTest.h"
#include "native/tests/BaseAssetProcessorTest.h"
#include <AssetBuilderSDK/AssetBuilderSDK.h>


namespace AssetProcessor
{
    TEST_F(AssetBuilderSDKTest, GetEnabledPlatformsCountUnitTest)
    {
        AssetBuilderSDK::CreateJobsRequest createJobsRequest;
        createJobsRequest.m_platformFlags = 0;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 0);

        createJobsRequest.m_platformFlags = 1;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 1);

        createJobsRequest.m_platformFlags = 2;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 1);

        createJobsRequest.m_platformFlags = 3;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 2);

        createJobsRequest.m_platformFlags = 4;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 1);

        createJobsRequest.m_platformFlags = 15;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 4);

        createJobsRequest.m_platformFlags = 16;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 1);

        //64 is 0x040 which currently is the next valid platform value which is invalid as of now, if we ever add a new platform entry to the Platform enum 
        // we will have to update these unit tests
        createJobsRequest.m_platformFlags = 64;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 0);

        createJobsRequest.m_platformFlags = 67; // 2 valid platforms as of now
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformsCount(), 2);      
    }

    TEST_F(AssetBuilderSDKTest, GetEnabledPlatformAtUnitTest)
    {
        AssetBuilderSDK::CreateJobsRequest createJobsRequest;
        createJobsRequest.m_platformFlags = 0;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_NONE);

        createJobsRequest.m_platformFlags = 1;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_PC);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(1), AssetBuilderSDK::Platform_NONE);

        createJobsRequest.m_platformFlags = 2;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_ES3);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(1), AssetBuilderSDK::Platform_NONE);

        createJobsRequest.m_platformFlags = 3;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_PC);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(1), AssetBuilderSDK::Platform_ES3);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(2), AssetBuilderSDK::Platform_NONE);

        createJobsRequest.m_platformFlags = 4;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_IOS);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(1), AssetBuilderSDK::Platform_NONE);

        createJobsRequest.m_platformFlags = 15;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_PC);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(1), AssetBuilderSDK::Platform_ES3);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(2), AssetBuilderSDK::Platform_IOS);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(3), AssetBuilderSDK::Platform_OSX);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(4), AssetBuilderSDK::Platform_NONE);

        createJobsRequest.m_platformFlags = 16;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_XBOXONE); // ACCEPTED_USE
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(1), AssetBuilderSDK::Platform_NONE);

        //64 is 0x040 which currently is the next valid platform value which is invalid as of now, if we ever add a new platform entry to the Platform enum 
        // we will have to update these unit tests
        createJobsRequest.m_platformFlags = 64;
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_NONE);

        createJobsRequest.m_platformFlags = 67; // 2 valid platforms
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(0), AssetBuilderSDK::Platform_PC);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(1), AssetBuilderSDK::Platform_ES3);
        ASSERT_EQ(createJobsRequest.GetEnabledPlatformAt(2), AssetBuilderSDK::Platform_NONE);
    }

    TEST_F(AssetBuilderSDKTest, IsPlatformEnabledUnitTest)
    {
        AssetBuilderSDK::CreateJobsRequest createJobsRequest;
        createJobsRequest.m_platformFlags = 0;
        ASSERT_FALSE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_PC));

        createJobsRequest.m_platformFlags = 1;
        ASSERT_TRUE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_PC));
        ASSERT_FALSE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_ES3));

        createJobsRequest.m_platformFlags = 3;
        ASSERT_TRUE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_PC));
        ASSERT_TRUE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_ES3));
        
        //64 is 0x040 which currently is the next valid platform value which is invalid as of now, if we ever add a new platform entry to the Platform enum 
        //we will have to update these unit tests
        createJobsRequest.m_platformFlags = 64; 
        ASSERT_FALSE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_PC));

        createJobsRequest.m_platformFlags = 67; // 2 valid platforms
        ASSERT_TRUE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_PC));
        ASSERT_TRUE(createJobsRequest.IsPlatformEnabled(AssetBuilderSDK::Platform_ES3));
    }

    TEST_F(AssetBuilderSDKTest, IsPlatformValidUnitTest)
    {
        AssetBuilderSDK::CreateJobsRequest createJobsRequest;
        ASSERT_TRUE(createJobsRequest.IsPlatformValid(AssetBuilderSDK::Platform_PC));
        ASSERT_TRUE(createJobsRequest.IsPlatformValid(AssetBuilderSDK::Platform_ES3));
        ASSERT_TRUE(createJobsRequest.IsPlatformValid(AssetBuilderSDK::Platform_IOS));
        ASSERT_TRUE(createJobsRequest.IsPlatformValid(AssetBuilderSDK::Platform_OSX));
        ASSERT_TRUE(createJobsRequest.IsPlatformValid(AssetBuilderSDK::Platform_XBOXONE)); // ACCEPTED_USE
        ASSERT_TRUE(createJobsRequest.IsPlatformValid(AssetBuilderSDK::Platform_PS4)); // ACCEPTED_USE
        //64 is 0x040 which currently is the next valid platform value which is invalid as of now, if we ever add a new platform entry to the Platform enum 
        //we will have to update this failure unit test
        ASSERT_FALSE(createJobsRequest.IsPlatformValid(static_cast<AssetBuilderSDK::Platform>(64))); 
    }
};
