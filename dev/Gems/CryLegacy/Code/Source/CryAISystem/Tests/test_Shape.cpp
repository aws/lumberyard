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
#include <AzTest/AzTest.h>
#include "../Shape.h"

TEST(DistancePointLinesegTest, Call_PointOnLine_DistanceSquaredIsZero)
{
    Vec3 start(1.0f, 0.0f, 0.0f);
    Vec3 p(2.0f, 0.0f, 0.0f);
    Vec3 end(3.0f, 0.0f, 0.0f);

    float t;
    float dist2 = CryAISystem::DistancePointLinesegSq(p, start, end, t);
    EXPECT_FLOAT_EQ(dist2, 0.0f);
}
