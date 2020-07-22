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

#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/DataType.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/IGraphContext.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Slot.h>

/**
 * Used for testing the base GraphModel namespace
 */
class GraphModelTest
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

TEST_F(GraphModelTest, ExampleTest)
{
    ASSERT_TRUE(true);
}

AZ_UNIT_TEST_HOOK();
