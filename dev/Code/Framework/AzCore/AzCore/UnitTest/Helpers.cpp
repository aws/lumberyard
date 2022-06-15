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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Color.h>

// make gtest/gmock aware of these types so when a failure occurs we get more useful output
namespace AZ
{
    std::ostream& operator<<(std::ostream& os, const Vector2& vec)
    {
        return os
            << "(X: " << static_cast<float>(vec.GetX())
            << ", Y: " << static_cast<float>(vec.GetY())
            << ")";
    }

    std::ostream& operator<<(std::ostream& os, const Vector3& vec)
    {
        return os
            << "(X: " << static_cast<float>(vec.GetX())
            << ", Y: " << static_cast<float>(vec.GetY())
            << ", Z: " << static_cast<float>(vec.GetZ())
            << ")";
    }

    std::ostream& operator<<(std::ostream& os, const Vector4& vec)
    {
        return os
            << "(X: " << static_cast<float>(vec.GetX())
            << ", Y: " << static_cast<float>(vec.GetY())
            << ", Z: " << static_cast<float>(vec.GetZ())
            << ", W: " << static_cast<float>(vec.GetW())
            << ")";
    }

    std::ostream& operator<<(std::ostream& os, const Quaternion& quat)
    {
        return os
            << "(X: " << static_cast<float>(quat.GetX())
            << ", Y: " << static_cast<float>(quat.GetY())
            << ", Z: " << static_cast<float>(quat.GetZ())
            << ", W: " << static_cast<float>(quat.GetW())
            << ")";
    }

    std::ostream& operator<<(std::ostream& os, const Transform& transform)
    {
        return os
            << "row 0: " << transform.GetRow(0)
            << " row 1: " << transform.GetRow(1)
            << " row 2: " << transform.GetRow(2);
    }

    std::ostream& operator<<(std::ostream& os, const Color& color)
    {
        return os
            << "red: " << static_cast<float>(color.GetR())
            << " green: " << static_cast<float>(color.GetG())
            << " blue: " << static_cast<float>(color.GetB())
            << " alpha: " << static_cast<float>(color.GetA());
    }
} // namespace AZ
