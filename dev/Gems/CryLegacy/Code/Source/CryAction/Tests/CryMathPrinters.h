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

// Custom type printers for googleteset
// https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md


void PrintTo(const Vec2& vec2, ::std::ostream* os)
{
    *os << "Vec2 (" << vec2.x << ", " << vec2.y << ")";
}