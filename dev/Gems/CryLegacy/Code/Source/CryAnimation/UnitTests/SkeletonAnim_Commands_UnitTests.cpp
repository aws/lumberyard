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

#include "CryLegacy_precompiled.h"
#include "SkeletonAnim.h"

#include <AzTest/AzTest.h>

// This is only defined in a .cpp file, so link it as an extern function
extern uint32 MergeParametricExamples(const uint32 numExamples, const f32* const exampleBlendWeights, const int16* const exampleLocalAnimationIds, f32* mergedExampleWeightsOut, int* mergedExampleIndicesOut);

TEST(MergeParametricExamplesTests, ContainsZeroWeights)
{
    const f32 exampleBlendWeights[] = { 0.f, 1.f, -1.f, 0.f, 1.f };
    const int16 exampleLocalAnimationIds[] = { 22, 44, 44, 22, 44 };
    const uint32 examplesCount = 5;

    int mergedExampleIndices[5];
    f32 mergedExampleWeights[5];
    const uint32 mergedExampleCount = MergeParametricExamples(examplesCount, exampleBlendWeights, exampleLocalAnimationIds, mergedExampleWeights, mergedExampleIndices);
    EXPECT_EQ(2, mergedExampleCount);
    EXPECT_EQ(0, mergedExampleIndices[0]);
    EXPECT_EQ(1, mergedExampleIndices[1]);
    EXPECT_NEAR(0.f, mergedExampleWeights[0], 0.01f);
    EXPECT_NEAR(1.f, mergedExampleWeights[1], 0.01f);
}


TEST(MergeParametricExamplesTests, NoZeroWeights)
{
    const f32 exampleBlendWeights[] = { -0.1f, 0.9f, -0.1f, 0.f, 0.3f };
    const int16 exampleLocalAnimationIds[] = { 22, 44, 66, 22, 44 };
    const uint32 examplesCount = 5;

    int mergedExampleIndices[5];
    f32 mergedExampleWeights[5];
    const uint32 mergedExampleCount = MergeParametricExamples(examplesCount, exampleBlendWeights, exampleLocalAnimationIds, mergedExampleWeights, mergedExampleIndices);
    EXPECT_EQ(3, mergedExampleCount);
    EXPECT_EQ(0, mergedExampleIndices[0]);
    EXPECT_EQ(1, mergedExampleIndices[1]);
    EXPECT_EQ(2, mergedExampleIndices[2]);
    EXPECT_NEAR(-0.1f, mergedExampleWeights[0], 0.01f);
    EXPECT_NEAR(1.2f, mergedExampleWeights[1], 0.01f);
    EXPECT_NEAR(-0.1f, mergedExampleWeights[2], 0.01f);
}


TEST(MergeParametricExamplesTests, WeightsAddToZero)
{
    const f32 exampleBlendWeights[] = { -1.f, 1.f, -1.f, 1.f, 1.f };
    const int16 exampleLocalAnimationIds[] = { 22, 44, 44, 22, 44 };
    const uint32 examplesCount = 5;

    int mergedExampleIndices[5];
    f32 mergedExampleWeights[5];
    const uint32 mergedExampleCount = MergeParametricExamples(examplesCount, exampleBlendWeights, exampleLocalAnimationIds, mergedExampleWeights, mergedExampleIndices);
    EXPECT_EQ(2, mergedExampleCount);
    EXPECT_EQ(0, mergedExampleIndices[0]);
    EXPECT_EQ(1, mergedExampleIndices[1]);
    EXPECT_NEAR(0.f, mergedExampleWeights[0], 0.01f);
    EXPECT_NEAR(1.f, mergedExampleWeights[1], 0.01f);
}
