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
#include "../utils.h"

namespace
{
    //! Factory method to create a unit box from 0.0,0.0 to (k,k)
    void CreateUnitBox(Vec2 (&box)[2], float scale = 1.0f)
    {
        box[0] = Vec2(0.0f, 0.0f);
        box[1] = Vec2(scale, scale);
    }
}

TEST(IntersectionTests, BoxSegmentIntersect_NoIntersect_False)
{
    Vec2 box[2];
    CreateUnitBox(box);
    bool intersect = box_segment_intersect(box, Vec2(2.0f, 2.0f), Vec2(3.0f, 3.0f));
    EXPECT_FALSE(intersect);
}

TEST(IntersectionTests, BoxSegmentIntersect_SegmentCrossLeftEdge_True)
{
    Vec2 box[2];
    CreateUnitBox(box);
    bool intersect = box_segment_intersect(box, Vec2(-0.5f, 0.5f), Vec2(0.5f, 0.5f));
    EXPECT_TRUE(intersect);
}

TEST(IntersectionTests, BoxSegmentIntersect_SegmentCrossesTopEdge_True)
{
    Vec2 box[2];
    CreateUnitBox(box);
    bool intersect = box_segment_intersect(box, Vec2(0.5f, 1.5f), Vec2(0.5f, 0.5f));
    EXPECT_TRUE(intersect);
}

TEST(IntersectionTests, BoxSegmentIntersect_SegmentCrossesEntireBoxHorizontal_True)
{
    Vec2 box[2];
    CreateUnitBox(box);
    bool intersect = box_segment_intersect(box, Vec2(-0.5f, 0.5f), Vec2(1.5f, 0.5f));
    EXPECT_TRUE(intersect);
}

/* box/segment intersection returns false if the segment is completely inside the box,
   which seems to mean it's only detecting crossing from one side of the box to the
   other (inside<-->outside). Added a comment to the method to clarify this functionality
   based on how the code is actually functioning. */
TEST(IntersectionTests, BoxSegmentIntersect_SegmentInsideBox_False)
{
    Vec2 box[2];
    CreateUnitBox(box);
    bool intersect = box_segment_intersect(box, Vec2(0.2f), Vec2(0.8f));
    EXPECT_FALSE(intersect);
}
