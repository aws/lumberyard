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
#include <AzTest/AzTest.h>

class AssetBuilderTests
    : public ::testing::Test
{
};

TEST_F(AssetBuilderTests, Sanity)
{
    ASSERT_TRUE(true);
}

// Setup a custom environment so we can test the trace bus handler without the TraceBusRedirector blocking asserts
class AssetBuilderTestEnvironment
    : public AZ::Test::ITestEnvironment
{
public:
    AZ_TEST_CLASS_ALLOCATOR(AssetBuilderTestEnvironment);


protected:
    void SetupEnvironment() override
    {
        
    }

    void TeardownEnvironment() override
    {
        
    }
};

AZ_UNIT_TEST_HOOK(new AssetBuilderTestEnvironment);
