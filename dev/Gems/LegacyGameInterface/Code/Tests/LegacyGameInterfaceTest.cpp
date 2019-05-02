/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#include "LegacyGameInterface_precompiled.h"

#include <AzTest/AzTest.h>

class LegacyGameInterfaceTest
    : public ::testing::Test
{
protected:
    void SetUp() override
    {

    }

    void TearDown() override
    {

    }
};

TEST_F(LegacyGameInterfaceTest, ExampleTest)
{
    // This module only contains implementation of an interface defined in the engine.
    // Activating these modules would require spinning up the whole engine, making this an integration test and not a unit test.
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
