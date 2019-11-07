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
#include "stdafx.h"
#include <AzTest/AzTest.h>
#include <Util/EditorUtils.h>
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <Terrain/LayerWeight.h>

namespace TerrainTests
{
    class TerrainTestFixture
        : public testing::Test
    {
    protected:
        void SetUp() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    TEST_F(TerrainTestFixture, Test_LayerWeights_ZeroWeightShouldBeUndefined)
    {
        AZStd::vector<uint8> layerWeights{ 0, 0, 0 };
        AZStd::vector<uint8> layerIds{ 1, 2, 3 };

        // Test:  When initializing a LayerWeight with all 0 weights, the layer IDs
        // should all get set to "Undefined".
        LayerWeight layerWeight(layerIds, layerWeights);
        for (int layer = 0; layer < layerWeight.WeightCount; layer++)
        {
            EXPECT_TRUE(layerWeight.GetLayerId(layer) == LayerWeight::Undefined);
        }

        // Test:  Even when setting a specific layer ID to a 0 weight, its weight should
        // get set to "Undefined".
        layerWeight.SetWeight(1, 0);
        for (int layer = 0; layer < layerWeight.WeightCount; layer++)
        {
            EXPECT_TRUE(layerWeight.GetLayerId(layer) == LayerWeight::Undefined);
        }
    }
}
