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

#pragma once

#include <AzCore/base.h>
#include <AzCore/PlatformDef.h>

#include <gmock/gmock.h>
#include <ostream>

namespace AZ
{
    class Vector2;
    class Vector3;
    class Vector4;
    class Quaternion;
    class Transform;
    class Color;

    std::ostream& operator<<(std::ostream& os, const Vector2& vec);
    std::ostream& operator<<(std::ostream& os, const Vector3& vec);
    std::ostream& operator<<(std::ostream& os, const Vector4& vec);
    std::ostream& operator<<(std::ostream& os, const Quaternion& quat);
    std::ostream& operator<<(std::ostream& os, const Transform& transform);
    std::ostream& operator<<(std::ostream& os, const Color& transform);
} // namespace AZ

namespace UnitTest
{
    // is-close matcher to make tests easier to read and failures more useful
    MATCHER_P(IsClose, expected, "")
    {
        AZ_UNUSED(result_listener);
        return arg.IsClose(expected);
    }

    // is-close matcher with tolerance to make tests easier to read and failures more useful
    MATCHER_P2(IsCloseTolerance, expected, tolerance, "")
    {
        AZ_UNUSED(result_listener);
        return arg.IsClose(expected, tolerance);
    }

    // is-close matcher for use with Pointwise container comparisons
    MATCHER(ContainerIsClose, "")
    {
        AZ_UNUSED(result_listener);
        const auto& [expected, actual] = arg;
        return expected.IsClose(actual);
    }

    // is-close matcher with tolerance for use with Pointwise container comparisons
    MATCHER_P(ContainerIsCloseTolerance, tolerance, "")
    {
        AZ_UNUSED(result_listener);
        const auto& [expected, actual] = arg;
        return expected.IsClose(actual, tolerance);
    }

    // IsFinite matcher to make it easier to validate Vector2, Vector3, Vector4 and Quaternion.
    // For example:
    //     AZ::Quaternion rotation;
    //     EXPECT_THAT(rotation, IsFinite());
    //
    //     AZStd::vector<AZ::Vector3> positions;
    //     EXPECT_THAT(positions, ::testing::Each(IsFinite()));
    MATCHER(IsFinite, "")
    {
        AZ_UNUSED(result_listener);
        return arg.IsFinite();
    }
} // namespace UnitTest
