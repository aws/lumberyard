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
#include <Tests/CryMathTestAux.h>

inline void PrintTo(const Vec3& v, std::ostream* os)
{
    auto originalPrecision = os->precision();
    os->precision(4);
    *os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    os->precision(originalPrecision);
}

inline void PrintTo(const Quat& q, std::ostream* os)
{
    auto originalPrecision = os->precision();
    os->precision(6);
    *os << "(w=" << q.w << ", (" << q.v.x << ", " << q.v.y << ", " << q.v.z << "))";
    os->precision(originalPrecision);
}

inline void PrintTo(const Vec3& v, ::testing::AssertionResult& result)
{
    result << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

inline void PrintTo(const Quat& q, ::testing::AssertionResult& result)
{
    result << "(" << q.w << ", (" << q.v.x << ", " << q.v.y << ", " << q.v.z << "))";
}

inline ::testing::AssertionResult AreEqual(const Vec3& expected, const Vec3& actual, float epsilon)
{
    // detect NaN (visual c++ specific)
    if (_isnan(actual.x) || _isnan(actual.y) || _isnan(actual.z))
    {
        ::testing::AssertionResult err = ::testing::AssertionFailure();
        err << "actual value includes NaN values: ";
        PrintTo(actual, err);
        return err;
    }

    if (fabs(expected.x - actual.x) > epsilon ||
        fabs(expected.y - actual.y) > epsilon ||
        fabs(expected.z - actual.z) > epsilon)
    {
        ::testing::AssertionResult err = ::testing::AssertionFailure();

        err << "vectors are different. Expected: ";
        PrintTo(expected, err);
        err << ", Actual: ";
        PrintTo(actual, err);
        return err;
    }

    return ::testing::AssertionSuccess();
}

inline ::testing::AssertionResult AreEqual(const Quat& expected, const Quat& actual, float epsilon)
{
    // detect NaN (visual c++ specific)
    if (_isnan(actual.w) || _isnan(actual.v.x) || _isnan(actual.v.y) || _isnan(actual.v.z))
    {
        ::testing::AssertionResult err = ::testing::AssertionFailure();
        err << "actual includes NaN values: ";
        PrintTo(actual, err);
        return err;
    }

    if (fabs(expected.w - actual.w) > epsilon ||
        fabs(expected.v.x - actual.v.x) > epsilon ||
        fabs(expected.v.y - actual.v.y) > epsilon ||
        fabs(expected.v.z - actual.v.z) > epsilon)
    {
        ::testing::AssertionResult err = ::testing::AssertionFailure();

        err << "quaternions are different. Expected: ";
        PrintTo(expected, err);
        err << ", Actual: ";
        PrintTo(actual, err);
        return err;
    }

    return ::testing::AssertionSuccess();
}
