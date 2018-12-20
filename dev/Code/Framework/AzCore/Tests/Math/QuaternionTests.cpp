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

#include "TestTypes.h"

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_Quaternion, GeneralTest)
    {
        constexpr float normalizeEpsilon = 0.002f;
        //constructors
        AZ::Quaternion q1(0.0f, 0.0f, 0.0f, 1.0f);
        EXPECT_TRUE((q1.GetX() == 0.0f) && (q1.GetY() == 0.0f) && (q1.GetZ() == 0.0f) && (q1.GetW() == 1.0f));
        AZ::Quaternion q2(5.0f);
        EXPECT_TRUE((q2.GetX() == 5.0f) && (q2.GetY() == 5.0f) && (q2.GetZ() == 5.0f) && (q2.GetW() == 5.0f));
        AZ::Quaternion q3(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE((q3.GetX() == 1.0f) && (q3.GetY() == 2.0f) && (q3.GetZ() == 3.0f) && (q3.GetW() == 4.0f));
        AZ::Quaternion q4 = AZ::Quaternion::CreateFromVector3AndValue(Vector3(1.0f, 2.0f, 3.0f), 4.0f);
        EXPECT_TRUE((q4.GetX() == 1.0f) && (q4.GetY() == 2.0f) && (q4.GetZ() == 3.0f) && (q4.GetW() == 4.0f));
        float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
        AZ::Quaternion q5 = AZ::Quaternion::CreateFromFloat4(values);
        EXPECT_TRUE((q5.GetX() == 10.0f) && (q5.GetY() == 20.0f) && (q5.GetZ() == 30.0f) && (q5.GetW() == 40.0f));
        AZ::Quaternion q6 = AZ::Quaternion::CreateFromTransform(Transform::CreateRotationX(DegToRad(60.0f)));
        EXPECT_TRUE(q6.IsClose(AZ::Quaternion(0.5f, 0.0f, 0.0f, 0.866f)));
        AZ::Quaternion q7 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationX(DegToRad(120.0f)));
        EXPECT_TRUE(q7.IsClose(AZ::Quaternion(0.866f, 0.0f, 0.0f, 0.5f)));
        AZ::Quaternion q8 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationX(DegToRad(-60.0f)));
        EXPECT_TRUE(q8.IsClose(AZ::Quaternion(-0.5f, 0.0f, 0.0f, 0.866f)));
        AZ::Quaternion q9 = AZ::Quaternion::CreateFromVector3(Vector3(1.0f, 2.0f, 3.0f));
        EXPECT_TRUE((q9.GetX() == 1.0f) && (q9.GetY() == 2.0f) && (q9.GetZ() == 3.0f) && (q9.GetW() == 0.0f));
        AZ::Quaternion q10 = AZ::Quaternion::CreateFromAxisAngle(Vector3::CreateAxisZ(), DegToRad(45.0f));
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion::CreateRotationZ(DegToRad(45.0f))));

        //Create functions
        EXPECT_TRUE(AZ::Quaternion::CreateIdentity() == AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(AZ::Quaternion::CreateZero() == AZ::Quaternion(0.0f));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationX(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.5f, 0.0f, 0.0f, 0.866f)));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.0f, 0.5f, 0.0f, 0.866f)));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationZ(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.0f, 0.0f, 0.5f, 0.866f)));

        //test shortest arc
        Vector3 v1 = Vector3(1.0f, 2.0f, 3.0f).GetNormalized();
        Vector3 v2 = Vector3(-2.0f, 7.0f, -1.0f).GetNormalized();
        q3 = AZ::Quaternion::CreateShortestArc(v1, v2); //q3 should transform v1 into v2
        EXPECT_TRUE(v2.IsClose(q3 * v1));
        q4 = AZ::Quaternion::CreateShortestArc(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
        EXPECT_TRUE((q4 * Vector3(0.0f, 0.0f, 1.0f)).IsClose(Vector3(0.0f, 0.0f, 1.0f)));  //perpendicular vector should be unaffected
        EXPECT_TRUE((q4 * Vector3(0.0f, -1.0f, 0.0f)).IsClose(Vector3(1.0f, 0.0f, 0.0f))); //make sure we rotate the right direction, i.e. actually shortest arc

        //Get/Set
        q1.SetX(10.0f);
        EXPECT_TRUE(q1.GetX() == 10.0f);
        q1.SetY(11.0f);
        EXPECT_TRUE(q1.GetY() == 11.0f);
        q1.SetZ(12.0f);
        EXPECT_TRUE(q1.GetZ() == 12.0f);
        q1.SetW(13.0f);
        EXPECT_TRUE(q1.GetW() == 13.0f);
        q1.Set(15.0f);
        EXPECT_TRUE(q1 == AZ::Quaternion(15.0f));
        q1.Set(2.0f, 3.0f, 4.0f, 5.0f);
        EXPECT_TRUE(q1 == AZ::Quaternion(2.0f, 3.0f, 4.0f, 5.0f));
        q1.Set(Vector3(5.0f, 6.0f, 7.0f), 8.0f);
        EXPECT_TRUE(q1 == AZ::Quaternion(5.0f, 6.0f, 7.0f, 8.0f));
        q1.Set(values);
        EXPECT_TRUE((q1.GetX() == 10.0f) && (q1.GetY() == 20.0f) && (q1.GetZ() == 30.0f) && (q1.GetW() == 40.0f));

        //GetElement/SetElement
        q1.SetElement(0, 1.0f);
        q1.SetElement(1, 2.0f);
        q1.SetElement(2, 3.0f);
        q1.SetElement(3, 4.0f);
        EXPECT_TRUE(q1.GetElement(0) == 1.0f);
        EXPECT_TRUE(q1.GetElement(1) == 2.0f);
        EXPECT_TRUE(q1.GetElement(2) == 3.0f);
        EXPECT_TRUE(q1.GetElement(3) == 4.0f);

        //index operators
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(q1(0) == 1.0f);
        EXPECT_TRUE(q1(1) == 2.0f);
        EXPECT_TRUE(q1(2) == 3.0f);
        EXPECT_TRUE(q1(3) == 4.0f);

        //IsIdentity
        q1.Set(0.0f, 0.0f, 0.0f, 1.0f);
        EXPECT_TRUE(q1.IsIdentity());

        //conjugate
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(q1.GetConjugate() == AZ::Quaternion(-1.0f, -2.0f, -3.0f, 4.0f));

        //inverse
        q1 = AZ::Quaternion::CreateRotationX(DegToRad(25.0f)) * AZ::Quaternion::CreateRotationY(DegToRad(70.0f));
        EXPECT_TRUE((q1 * q1.GetInverseFast()).IsClose(AZ::Quaternion::CreateIdentity()));
        q2 = q1;
        q2.InvertFast();
        EXPECT_TRUE(q1.GetX() == -q2.GetX());
        EXPECT_TRUE(q1.GetY() == -q2.GetY());
        EXPECT_TRUE(q1.GetZ() == -q2.GetZ());
        EXPECT_TRUE(q1.GetW() == q2.GetW());
        EXPECT_TRUE((q1 * q2).IsClose(AZ::Quaternion::CreateIdentity()));

        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE((q1 * q1.GetInverseFull()).IsClose(AZ::Quaternion::CreateIdentity()));

        //dot product
        EXPECT_NEAR(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).Dot(AZ::Quaternion(-1.0f, 5.0f, 3.0f, 2.0f)), 26.0f, normalizeEpsilon);

        //length
        EXPECT_NEAR(AZ::Quaternion(-1.0f, 2.0f, 1.0f, 3.0f).GetLengthSq(), 15.0f, normalizeEpsilon);
        EXPECT_NEAR(AZ::Quaternion(-4.0f, 2.0f, 0.0f, 4.0f).GetLength(), 6.0f, normalizeEpsilon);

        //normalize
        EXPECT_TRUE(AZ::Quaternion(0.0f, -4.0f, 2.0f, 4.0f).GetNormalized().IsClose(AZ::Quaternion(0.0f, -0.666f, 0.333f, 0.666f)));
        q1.Set(2.0f, 0.0f, 4.0f, -4.0f);
        q1.Normalize();
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(0.333f, 0.0f, 0.666f, -0.666f)));
        q1.Set(2.0f, 0.0f, 4.0f, -4.0f);
        float length = q1.NormalizeWithLength();
        EXPECT_NEAR(length, 6.0f, normalizeEpsilon);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(0.333f, 0.0f, 0.666f, -0.666f)));

        //interpolation
        EXPECT_TRUE(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).Lerp(AZ::Quaternion(2.0f, 3.0f, 4.0f, 5.0f), 0.5f).IsClose(AZ::Quaternion(1.5f, 2.5f, 3.5f, 4.5f)));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationX(DegToRad(10.0f)).Slerp(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)), 0.5f).IsClose(AZ::Quaternion(0.045f, 0.259f, 0.0f, 0.965f)));
        EXPECT_TRUE(AZ::Quaternion::CreateRotationX(DegToRad(10.0f)).Squad(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)), AZ::Quaternion::CreateRotationZ(DegToRad(35.0f)), AZ::Quaternion::CreateRotationX(DegToRad(80.0f)), 0.5f).IsClose(AZ::Quaternion(0.2f, 0.132f, 0.083f, 0.967f)));

        //close
        EXPECT_TRUE(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));
        EXPECT_TRUE(!AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f)));
        EXPECT_TRUE(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.4f), 0.5f));

        //operators
        EXPECT_TRUE((-AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)) == AZ::Quaternion(-1.0f, -2.0f, -3.0f, -4.0f));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) + AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(3.0f, 5.0f, 8.0f, 3.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) - AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(-1.0f, -1.0f, -2.0f, 5.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) * AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(8.0f, 11.0f, 16.0f, -27.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f).IsClose(AZ::Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_TRUE((2.0f * AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)).IsClose(AZ::Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
        EXPECT_TRUE((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) / 2.0f).IsClose(AZ::Quaternion(0.5f, 1.0f, 1.5f, 2.0f)));
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        q1 += AZ::Quaternion(5.0f, 6.0f, 7.0f, 8.0f);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(6.0f, 8.0f, 10.0f, 12.0f)));
        q1 -= AZ::Quaternion(3.0f, -1.0f, 5.0f, 7.0f);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(3.0f, 9.0f, 5.0f, 5.0f)));
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        q1 *= AZ:: Quaternion(2.0f, 3.0f, 5.0f, -1.0f);
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(8.0f, 11.0f, 16.0f, -27.0f)));
        q1 *= 2.0f;
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(16.0f, 22.0f, 32.0f, -54.0f)));
        q1 /= 4.0f;
        EXPECT_TRUE(q1.IsClose(AZ::Quaternion(4.0f, 5.5f, 8.0f, -13.5f)));

        //operator==
        q3.Set(1.0f, 2.0f, 3.0f, 4.0f);
        EXPECT_TRUE(q3 == AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0));
        EXPECT_TRUE(!(q3 == AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f)));
        EXPECT_TRUE(q3 != AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f));
        EXPECT_TRUE(!(q3 != AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));

        //vector transform
        EXPECT_TRUE((AZ::Quaternion::CreateRotationX(DegToRad(45.0f)) * Vector3(4.0f, 1.0f, 0.0f)).IsClose(Vector3(4.0f, 0.707f, 0.707f)));

        //GetImaginary
        q1.Set(21.0f, 22.0f, 23.0f, 24.0f);
        EXPECT_TRUE(q1.GetImaginary() == Vector3(21.0f, 22.0f, 23.0f));

        //GetAngle
        q1 = AZ::Quaternion::CreateRotationX(DegToRad(35.0f));
        EXPECT_TRUE(q1.GetAngle().IsClose(DegToRad(35.0f)));

        //make sure that our transformations concatenate in the correct order
        q1 = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));
        q2 = AZ::Quaternion::CreateRotationX(DegToRad(90.0f));
        Vector3 v = (q2 * q1) * Vector3(1.0f, 0.0f, 0.0f);
        EXPECT_TRUE(v.IsClose(Vector3(0.0f, 0.0f, 1.0f)));
    }

    TEST(MATH_Quaternion, ToEulerDegrees)
    {
        VectorFloat halfAngle = 0.5f *  Constants::QuarterPi;
        float sin = sinf(halfAngle);
        float cos = cosf(halfAngle);
        AZ::Quaternion testQuat = AZ::Quaternion::CreateFromVector3AndValue(sin * AZ::Vector3::CreateAxisX(), cos);
        AZ::Vector3 resultVector = testQuat.GetEulerDegrees();
        EXPECT_TRUE(resultVector.IsClose(AZ::Vector3(45.0f, 0.0f, 0.0f)));

        resultVector = ConvertQuaternionToEulerDegrees(testQuat);
        EXPECT_TRUE(resultVector.IsClose(AZ::Vector3(45.0f, 0.0f, 0.0f)));
    }

    TEST(MATH_Quaternion, ToEulerRadians)
    {
        constexpr float getEulerRadiansEpsilon = 0.001f;
        VectorFloat halfAngle = 0.5f *  Constants::HalfPi;
        float sin = sinf(halfAngle);
        float cos = cosf(halfAngle);

        AZ::Quaternion testQuat = AZ::Quaternion::CreateFromVector3AndValue(sin * AZ::Vector3::CreateAxisY(), cos);
        AZ::Vector3 resultVector = testQuat.GetEulerRadians();
        EXPECT_NEAR(Constants::HalfPi, static_cast<float>(resultVector.GetY()), getEulerRadiansEpsilon);

        resultVector = ConvertQuaternionToEulerRadians(testQuat);
        EXPECT_NEAR(Constants::HalfPi, static_cast<float>(resultVector.GetY()), getEulerRadiansEpsilon);
    }

    TEST(MATH_Quaternion, FromEulerDegrees)
    {
        const AZ::Vector3 testDegrees(0.0f, 0.0f, 45.0f);
        AZ::Quaternion testQuat;
        testQuat.SetFromEulerDegrees(testDegrees);
        EXPECT_TRUE(testQuat.IsClose(AZ::Quaternion(0.0f, 0.0f, 0.382690936f, 0.924530089f)));
    }

    TEST(MATH_Quaternion, FromEulerRadians)
    {
        const AZ::Vector3 testRadians(0.0f, 0.0f, Constants::QuarterPi);
        AZ::Quaternion testQuat;
        testQuat.SetFromEulerRadians(testRadians);
        EXPECT_TRUE(testQuat.IsClose(AZ::Quaternion(0.0f, 0.0f, 0.382690936f, 0.924530089f)));
    }

    TEST(MATH_Quaternion, FromAxisAngle)
    {
        AZ::Quaternion q10 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), Constants::QuarterPi);
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion::CreateRotationZ(Constants::QuarterPi)));

        q10 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), Constants::HalfPi);
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion::CreateRotationY(Constants::HalfPi)));

        q10 = AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), Constants::TwoPi / 3.0f);
        EXPECT_TRUE(q10.IsClose(AZ::Quaternion::CreateRotationX(Constants::TwoPi / 3.0f)));
    }

    TEST(MATH_Quaternion, ToAxisAngle)
    {
        constexpr float axisAngleEpsilon = 0.002f;
        AZ::Quaternion testQuat = AZ::Quaternion::CreateIdentity();

        Vector3 resultAxis = Vector3::CreateZero();
        float resultAngle{};
        testQuat.ConvertToAxisAngle(resultAxis, resultAngle);
        EXPECT_TRUE(resultAxis.IsClose(Vector3::CreateAxisY()));
        EXPECT_NEAR(0.0f, resultAngle, axisAngleEpsilon);
    }
}
