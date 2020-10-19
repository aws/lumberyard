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

#include <AzCore/UnitTest/Helpers.h>

#include <AzCore/Math/ToString.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Quaternion.h>

namespace AZ
{
    // make gtest/gmock aware of these types so when a failure occurs we get more useful output
    std::ostream& operator<<(std::ostream& os, const Vector3& vec)
    {
        return os << ToString(vec).c_str();
    }

    std::ostream& operator<<(std::ostream& os, const Quaternion& quat)
    {
        return os << ToString(quat).c_str();
    }
} // namespace AZ
