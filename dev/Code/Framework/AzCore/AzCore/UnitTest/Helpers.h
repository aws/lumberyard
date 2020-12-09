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

#include <AzCore/PlatformDef.h>

#include <gmock/gmock.h>
#include <ostream>

namespace UnitTest
{
    AZ_PUSH_DISABLE_WARNING(4100, "-Wno-unused-parameter")
    // matcher to make tests easier to read and failures more useful (more information is included in the output)
    MATCHER_P(IsClose, v, "") { return arg.IsClose(v); }
    MATCHER_P2(IsClose, v, t, "") { return arg.IsClose(v, t); }
    AZ_POP_DISABLE_WARNING
} // namespace UnitTest

namespace AZ
{
    class Vector3;
    class Quaternion;

    std::ostream& operator<<(std::ostream& os, const Vector3& vec);
    std::ostream& operator<<(std::ostream& os, const Quaternion& quat);
} // namespace AZ
