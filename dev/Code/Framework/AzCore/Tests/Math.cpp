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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Plane.h>

#include <AzCore/Math/Sfmt.h>
#include <AzCore/Math/Uuid.h>

#include <AzCore/std/containers/unordered_set.h>

using namespace AZ;

namespace UnitTest
{
    TEST(MATH_VectorFloat, Test)
    {
        //load/store float
        float x = 5.0f;
        VectorFloat v0 = VectorFloat::CreateFromFloat(&x);
        AZ_TEST_ASSERT(v0 == 5.0f);
        v0 = 6.0f;
        v0.StoreToFloat(&x);
        AZ_TEST_ASSERT(x == 6.0f);

        //operators
        VectorFloat v1(1.0f), v2(2.0f), v3(3.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(-v1, -1.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1 + v2, 3.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v3 - v2, 1.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v2 * v3, 6.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v3 / v2, 1.5f);

        //min/max
        AZ_TEST_ASSERT(v1.GetMin(v2) == 1.0f);
        AZ_TEST_ASSERT(v1.GetMax(v2) == 2.0f);
        AZ_TEST_ASSERT(v1.GetClamp(0.0f, 2.0f) == 1.0f);
        AZ_TEST_ASSERT(v1.GetClamp(2.0f, 3.0f) == 2.0f);
        AZ_TEST_ASSERT(v1.GetClamp(-1.0f, 0.0f) == 0.0f);

        //
        VectorFloat v4(-2.0f);
        AZ_TEST_ASSERT(v4.GetAbs() == 2.0f);
        AZ_TEST_ASSERT(v2.GetAbs() == 2.0f);

        //comparisons
        VectorFloat v5(3.0f);
        AZ_TEST_ASSERT(v3 == v5);
        AZ_TEST_ASSERT(v3 != v2);
        AZ_TEST_ASSERT(v1.IsLessThan(v2));
        AZ_TEST_ASSERT(!v2.IsLessThan(v1));
        AZ_TEST_ASSERT(!v2.IsLessThan(v2));
        AZ_TEST_ASSERT(v2.IsLessEqualThan(v3));
        AZ_TEST_ASSERT(!v2.IsLessEqualThan(v1));
        AZ_TEST_ASSERT(v2.IsLessEqualThan(v2));

        AZ_TEST_ASSERT(v3.IsGreaterThan(v1));
        AZ_TEST_ASSERT(!v2.IsGreaterThan(v3));
        AZ_TEST_ASSERT(!v2.IsGreaterThan(v2));
        AZ_TEST_ASSERT(v3.IsGreaterEqualThan(v1));
        AZ_TEST_ASSERT(!v2.IsGreaterEqualThan(v3));
        AZ_TEST_ASSERT(v1.IsGreaterEqualThan(v1));

        AZ_TEST_ASSERT_FLOAT_CLOSE(v3.GetReciprocal(), 0.33333f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v3.GetReciprocalApprox(), 0.33333f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v3.GetReciprocalExact(), 0.33333f);

        //
        AZ_TEST_ASSERT_FLOAT_CLOSE(v2.GetSqrt(), 1.41421f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v2.GetSqrtApprox(), 1.41421f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v2.GetSqrtExact(), 1.41421f);

        AZ_TEST_ASSERT_FLOAT_CLOSE(v2.GetSqrtReciprocal(), 0.70710f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v2.GetSqrtReciprocalApprox(), 0.70710f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v2.GetSqrtReciprocalExact(), 0.70710f);

        //trig
        AZ_TEST_ASSERT_FLOAT_CLOSE(VectorFloat(DegToRad(78.0f)).GetAngleMod(), DegToRad(78.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(VectorFloat(DegToRad(-150.0f)).GetAngleMod(), DegToRad(-150.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(VectorFloat(DegToRad(190.0f)).GetAngleMod(), DegToRad(-170.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(VectorFloat(DegToRad(390.0f)).GetAngleMod(), DegToRad(30.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(VectorFloat(DegToRad(-190.0f)).GetAngleMod(), DegToRad(170.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(VectorFloat(DegToRad(-400.0f)).GetAngleMod(), DegToRad(-40.0f));
        AZ_TEST_ASSERT(fabsf(VectorFloat(DegToRad(60.0f)).GetSin() - 0.866f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(VectorFloat(DegToRad(105.0f)).GetSin() - 0.966f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(VectorFloat(DegToRad(-174.0f)).GetSin() + 0.105f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(VectorFloat(DegToRad(60.0f)).GetCos() - 0.5f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(VectorFloat(DegToRad(105.0f)).GetCos() + 0.259f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(VectorFloat(DegToRad(-174.0f)).GetCos() + 0.995f) < 0.005f);

        VectorFloat sin, cos;
        VectorFloat v60(DegToRad(60.0f));
        VectorFloat v105(DegToRad(105.0f));
        VectorFloat v174(DegToRad(-174.0f));
        v60.GetSinCos(sin, cos);
        AZ_TEST_ASSERT(fabsf(sin - 0.866f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(cos - 0.5f) < 0.005f);
        v105.GetSinCos(sin, cos);
        AZ_TEST_ASSERT(fabsf(sin - 0.966f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(cos + 0.259f) < 0.005f);
        v174.GetSinCos(sin, cos);
        AZ_TEST_ASSERT(fabsf(sin + 0.105f) < 0.005f);
        AZ_TEST_ASSERT(fabsf(cos + 0.995f) < 0.005f);

        const float epsilon = 0.01f;

        int i0(1);
        int i1(2);
        AZ_TEST_ASSERT(!IsEven(i0));
        AZ_TEST_ASSERT(IsOdd(i0));
        AZ_TEST_ASSERT(IsEven(i1));
        AZ_TEST_ASSERT(!IsOdd(i1));

        VectorFloat f2(5.1f);
        VectorFloat f3(3.f);
        AZ_TEST_ASSERT(IsClose(GetMod(f2, f3), 2.1f, epsilon));

        VectorFloat f4(0.f);
        VectorFloat sinResult(0.f);
        VectorFloat cosResult(0.f);
        GetSinCos(f4, sinResult, cosResult);
        AZ_TEST_ASSERT(IsClose(sinResult, 0.f, epsilon));
        AZ_TEST_ASSERT(IsClose(cosResult, 1.f, epsilon));

    }

    TEST(MATH_Vector2, Test)
    {
        //Constructors
        Vector2 vA(0.0f);
        AZ_TEST_ASSERT((vA.GetX() == 0.0f) && (vA.GetY() == 0.0f));
        Vector2 vB(5.0f);
        AZ_TEST_ASSERT((vB.GetX() == 5.0f) && (vB.GetY() == 5.0f));
        Vector2 vC(1.0f, 2.0f);
        AZ_TEST_ASSERT((vC.GetX() == 1.0f) && (vC.GetY() == 2.0f));

        //Create functions
        AZ_TEST_ASSERT(Vector2::CreateOne() == Vector2(1.0f, 1.0f));
        AZ_TEST_ASSERT(Vector2::CreateZero() == Vector2(0.0f, 0.0f));
        float values[2] = { 10.0f, 20.0f };
        AZ_TEST_ASSERT(Vector2::CreateFromFloat2(values) == Vector2(10.0f, 20.0f));
        AZ_TEST_ASSERT(Vector2::CreateAxisX() == Vector2(1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector2::CreateAxisY() == Vector2(0.0f, 1.0f));

        // Create - Comparison functions
        vA.Set(-100.0f, 10.0f);
        vB.Set(35.0f, 10.0f);
        vC.Set(35.0f, 20.0f);
        Vector2 vD(15.0f, 30.0f);

        // Compare equal
        // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        Vector2 compareEqualAB = Vector2::CreateSelectCmpEqual(vA, vB, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareEqualAB.IsClose(Vector2(0.0f, 1.0f)));
        Vector2 compareEqualBC = Vector2::CreateSelectCmpEqual(vB, vC, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareEqualBC.IsClose(Vector2(1.0f, 0.0f)));

        // Compare greater equal
        // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        Vector2 compareGreaterEqualAB = Vector2::CreateSelectCmpGreaterEqual(vA, vB, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualAB.IsClose(Vector2(0.0f, 1.0f)));
        Vector2 compareGreaterEqualBD = Vector2::CreateSelectCmpGreaterEqual(vB, vD, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterEqualBD.IsClose(Vector2(1.0f, 0.0f)));

        // Compare greater
        ///operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        Vector2 compareGreaterAB = Vector2::CreateSelectCmpGreater(vA, vB, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterAB.IsClose(Vector2(0.0f, 0.0f)));
        Vector2 compareGreaterCA = Vector2::CreateSelectCmpGreater(vC, vA, Vector2(1.0f), Vector2(0.0f));
        AZ_TEST_ASSERT(compareGreaterCA.IsClose(Vector2(1.0f, 1.0f)));

        //Store float functions
        vA.Set(1.0f, 2.0f);
        vA.StoreToFloat2(values);
        AZ_TEST_ASSERT(values[0] == 1.0f && values[1] == 2.0f);

        //Get/Set
        vA.Set(2.0f, 3.0f);
        AZ_TEST_ASSERT(vA == Vector2(2.0f, 3.0f));
        AZ_TEST_ASSERT(vA.GetX() == 2.0f && vA.GetY() == 3.0f);
        vA.SetX(10.0f);
        AZ_TEST_ASSERT(vA == Vector2(10.0f, 3.0f));
        vA.SetY(11.0f);
        AZ_TEST_ASSERT(vA == Vector2(10.0f, 11.0f));
        vA.Set(15.0f);
        AZ_TEST_ASSERT(vA == Vector2(15.0f));
        vA.SetElement(0, 20.0f);
        AZ_TEST_ASSERT(vA.GetX() == 20.0f && vA.GetY() == 15.0f);
        AZ_TEST_ASSERT(vA.GetElement(0) == 20.0f && vA.GetElement(1) == 15.0f);
        AZ_TEST_ASSERT(vA(0) == 20.0f && vA(1) == 15.0f);
        vA.SetElement(1, 21.0f);
        AZ_TEST_ASSERT(vA.GetX() == 20.0f && vA.GetY() == 21.0f);
        AZ_TEST_ASSERT(vA.GetElement(0) == 20.0f && vA.GetElement(1) == 21.0f);
        AZ_TEST_ASSERT(vA(0) == 20.0f && vA(1) == 21.0f);

        //Length
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(3.0f, 4.0f).GetLengthSq(), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(4.0f, -3.0f).GetLength(), 5.0f);

        //Normalize
        AZ_TEST_ASSERT(Vector2(3.0f, 4.0f).GetNormalized().IsClose(Vector2(3.0f / 5.0f, 4.0f / 5.0f)));

        vA.Set(4.0f, 3.0f);
        vA.Normalize();
        AZ_TEST_ASSERT(vA.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));

        vA.Set(4.0f, 3.0f);
        float length = vA.NormalizeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(vA.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));

        vA.Set(4.0f, 3.0f);
        vA.NormalizeSafe();
        AZ_TEST_ASSERT(vA.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));

        vA.Set(0.0f);
        vA.NormalizeSafe();
        AZ_TEST_ASSERT(vA.IsClose(Vector2(1.0f, 0.0f)));

        vA.Set(4.0f, 3.0f);
        length = vA.NormalizeSafeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(vA.IsClose(Vector2(4.0f / 5.0f, 3.0f / 5.0f)));

        vA.Set(0.0f);
        length = vA.NormalizeSafeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 0.0f);
        AZ_TEST_ASSERT(vA.IsClose(Vector2(1.0f, 0.0f)));

        AZ_TEST_ASSERT(Vector2(1.0f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(Vector2(0.707f, 0.707f).IsNormalized());
        AZ_TEST_ASSERT(!Vector2(1.0f, 1.0f).IsNormalized());

        //Set Length
        vA.Set(3.0f, 4.0f);
        vA.SetLength(10.0f);
        AZ_TEST_ASSERT(vA.IsClose(Vector2(6.0f, 8.0f)));

        //Distance
        vA.Set(1.0f, 2.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(vA.GetDistanceSq(Vector2(-2.0f, 6.0f)), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(vA.GetDistance(Vector2(5.0f, -1.0f)), 5.0f);

        //Lerp/slerp
        AZ_TEST_ASSERT(Vector2(3.0f, 4.0f).Lerp(Vector2(10.0f, 2.0f), 0.5f) == Vector2(6.5f, 3.0f));
        Vector2 slerped = Vector2(1.0f, 0.0f).Slerp(Vector2(0.0f, 1.0f), 0.5f);
        AZ_TEST_ASSERT(slerped.IsClose(Vector2(0.707f, 0.707f)));

        //GetPerpendicular
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetPerpendicular() == Vector2(-2.0f, 1.0f));

        //Close
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 3.0f)));
        //Verify the closeness tolerance works
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsClose(Vector2(1.0f, 2.4f), 0.5f));

        //IsZero
        AZ_TEST_ASSERT(Vector2(0.0f).IsZero());
        AZ_TEST_ASSERT(!Vector2(1.0f).IsZero());
        //Verify the IsZero tolerance works
        AZ_TEST_ASSERT(Vector2(0.05f).IsZero(0.1f));

        //Operator== and Operator!=
        vA.Set(1.0f, 2.0f);
        AZ_TEST_ASSERT(vA == Vector2(1.0f, 2.0f));
        AZ_TEST_ASSERT(!(vA == Vector2(5.0f, 6.0f)));
        AZ_TEST_ASSERT(vA != Vector2(7.0f, 8.0f));
        AZ_TEST_ASSERT(!(vA != Vector2(1.0f, 2.0f)));

        //Comparisons - returns true only if all the components pass
        //IsLessThan
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsLessThan(Vector2(2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsLessThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsLessThan(Vector2(2.0f, 2.0f)));
        //IsLessEqualThan
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsLessEqualThan(Vector2(2.0f, 2.0f)));
        //isGreaterThan
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 1.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsGreaterThan(Vector2(0.0f, 2.0f)));
        //isGreaterEqualThan
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 1.0f)));
        AZ_TEST_ASSERT(!Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 3.0f)));
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).IsGreaterEqualThan(Vector2(0.0f, 2.0f)));

        //min,max,clamp
        AZ_TEST_ASSERT(Vector2(2.0f, 3.0f).GetMin(Vector2(1.0f, 4.0f)) == Vector2(1.0f, 3.0f));
        AZ_TEST_ASSERT(Vector2(2.0f, 3.0f).GetMax(Vector2(1.0f, 4.0f)) == Vector2(2.0f, 4.0f));
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetClamp(Vector2(5.0f, -10.0f), Vector2(10.0f, -5.0f)) == Vector2(5.0f, -5.0f));
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetClamp(Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f)) == Vector2(1.0f, 2.0f));

        //Select
        vA.Set(1.0f);
        vB.Set(2.0f);
        AZ_TEST_ASSERT(vA.GetSelect(Vector2(0.0f, 1.0f), vB) == Vector2(1.0f, 2.0f));
        vA.Select(Vector2(1.0f, 0.0f), vB);
        AZ_TEST_ASSERT(vA == Vector2(2.0f, 1.0f));

        //Abs
        AZ_TEST_ASSERT(Vector2(-1.0f, 2.0f).GetAbs() == Vector2(1.0f, 2.0f));
        AZ_TEST_ASSERT(Vector2(3.0f, -4.0f).GetAbs() == Vector2(3.0f, 4.0f));

        //Operators
        vA.Set(1.0f, 2.0f);
        vA += Vector2(3.0f, 4.0f);
        AZ_TEST_ASSERT(vA == Vector2(4.0f, 6.0f));

        vA.Set(10.0f, 11.0f);
        vA -= Vector2(2.0f, 4.0f);
        AZ_TEST_ASSERT(vA == Vector2(8.0f, 7.0f));

        vA.Set(2.0f, 4.0f);
        vA *= Vector2(3.0f, 6.0f);
        AZ_TEST_ASSERT(vA == Vector2(6.0f, 24.0f));

        vA.Set(15.0f, 20.0f);
        vA /= Vector2(3.0f, 2.0f);
        AZ_TEST_ASSERT(vA == Vector2(5.0f, 10.0f));

        vA.Set(2.0f, 3.0f);
        vA *= 5.0f;
        AZ_TEST_ASSERT(vA == Vector2(10.0f, 15.0f));

        vA.Set(20.0f, 30.0f);
        vA /= 10.0f;
        AZ_TEST_ASSERT(vA == Vector2(2.0f, 3.0f));

        AZ_TEST_ASSERT((-Vector2(1.0f, -2.0f)) == Vector2(-1.0f, 2.0f));
        AZ_TEST_ASSERT((Vector2(1.0f, 2.0f) + Vector2(2.0f, -1.0f)) == Vector2(3.0f, 1.0f));
        AZ_TEST_ASSERT((Vector2(1.0f, 2.0f) - Vector2(2.0f, -1.0f)) == Vector2(-1.0f, 3.0f));
        AZ_TEST_ASSERT((Vector2(3.0f, 2.0f) * Vector2(2.0f, -4.0f)) == Vector2(6.0f, -8.0f));
        AZ_TEST_ASSERT((Vector2(3.0f, 2.0f) * 2.0f) == Vector2(6.0f, 4.0f));
        AZ_TEST_ASSERT((Vector2(30.0f, 20.0f) / Vector2(10.0f, -4.0f)) == Vector2(3.0f, -5.0f));
        AZ_TEST_ASSERT((Vector2(30.0f, 20.0f) / 10.0f) == Vector2(3.0f, 2.0f));

        //Dot product
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector2(1.0f, 2.0f).Dot(Vector2(-1.0f, 5.0f)), 9.0f);

        //Madd : Multiply and add
        AZ_TEST_ASSERT(Vector2(1.0f, 2.0f).GetMadd(Vector2(2.0f, 6.0f), Vector2(3.0f, 4.0f)) == Vector2(5.0f, 16.0f));
        vA.Set(1.0f, 2.0f);
        vA.Madd(Vector2(2.0f, 6.0f), Vector2(3.0f, 4.0f));
        AZ_TEST_ASSERT(vA == Vector2(5.0f, 16.0f));

        //Project
        vA.Set(0.5f);
        vA.Project(Vector2(0.0f, 2.0f));
        AZ_TEST_ASSERT(vA == Vector2(0.0f, 0.5f));
        vA.Set(0.5f);
        vA.ProjectOnNormal(Vector2(0.0f, 1.0f));
        AZ_TEST_ASSERT(vA == Vector2(0.0f, 0.5f));
        vA.Set(2.0f, 4.0f);
        AZ_TEST_ASSERT(vA.GetProjected(Vector2(1.0f, 1.0f)) == Vector2(3.0f, 3.0f));
        AZ_TEST_ASSERT(vA.GetProjectedOnNormal(Vector2(1.0f, 0.0f)) == Vector2(2.0f, 0.0f));

        //IsFinite
        AZ_TEST_ASSERT(Vector2(1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        AZ_TEST_ASSERT(!Vector2(infinity, infinity).IsFinite());
    }

    TEST(MATH_Vector3, Test)
    {
        //constructors
        Vector3 v1(0.0f);
        AZ_TEST_ASSERT((v1.GetX() == 0.0f) && (v1.GetY() == 0.0f) && (v1.GetZ() == 0.0f));
        Vector3 v2(5.0f);
        AZ_TEST_ASSERT((v2.GetX() == 5.0f) && (v2.GetY() == 5.0f) && (v2.GetZ() == 5.0f));
        Vector3 v3(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT((v3.GetX() == 1.0f) && (v3.GetY() == 2.0f) && (v3.GetZ() == 3.0f));

        //Create functions
        AZ_TEST_ASSERT(Vector3::CreateOne() == Vector3(1.0f, 1.0f, 1.0f));
        AZ_TEST_ASSERT(Vector3::CreateZero() == Vector3(0.0f));
        float values[3] = { 10.0f, 20.0f, 30.0f };
        AZ_TEST_ASSERT(Vector3::CreateFromFloat3(values) == Vector3(10.0f, 20.0f, 30.0f));
        AZ_TEST_ASSERT(Vector3::CreateAxisX() == Vector3(1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3::CreateAxisY() == Vector3(0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3::CreateAxisZ() == Vector3(0.0f, 0.0f, 1.0f));

        //Store float functions
        v1.Set(1.0f, 2.0f, 3.0f);
        v1.StoreToFloat3(values);
        AZ_TEST_ASSERT(values[0] == 1.0f && values[1] == 2.0f && values[2] == 3.0f);
        float values4[4];
        v1.StoreToFloat4(values4);
        AZ_TEST_ASSERT(values4[0] == 1.0f && values4[1] == 2.0f && values4[2] == 3.0f);

        //Get/Set
        v1.Set(2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(v1 == Vector3(2.0f, 3.0f, 4.0f));
        v1.SetX(10.0f);
        AZ_TEST_ASSERT(v1 == Vector3(10.0f, 3.0f, 4.0f));
        v1.SetY(11.0f);
        AZ_TEST_ASSERT(v1 == Vector3(10.0f, 11.0f, 4.0f));
        v1.SetZ(12.0f);
        AZ_TEST_ASSERT(v1 == Vector3(10.0f, 11.0f, 12.0f));
        v1.Set(15.0f);
        AZ_TEST_ASSERT(v1 == Vector3(15.0f));
        v1.Set(values);
        AZ_TEST_ASSERT((v1.GetX() == 1.0f) && (v1.GetY() == 2.0f) && (v1.GetZ() == 3.0f));

        //index element
        v1.Set(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(v1(0) == 1.0f);
        AZ_TEST_ASSERT(v1(1) == 2.0f);
        AZ_TEST_ASSERT(v1(2) == 3.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 1.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 2.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 3.0f);
        v1.SetElement(0, 5.0f);
        v1.SetElement(1, 6.0f);
        v1.SetElement(2, 7.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 5.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 6.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 7.0f);

        //length
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(3.0f, 4.0f, 0.0f).GetLengthSq(), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLength(), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthApprox(), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthExact(), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocal(), 0.2f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocalApprox(), 0.2f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(0.0f, 4.0f, -3.0f).GetLengthReciprocalExact(), 0.2f);

        //normalize
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalized().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedApprox().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedExact().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));

        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafe().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafeApprox().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(3.0f, 0.0f, 4.0f).GetNormalizedSafeExact().IsClose(Vector3(3.0f / 5.0f, 0.0f, 4.0f / 5.0f)));
        AZ_TEST_ASSERT(Vector3(0.0f).GetNormalizedSafe() == Vector3(1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3(0.0f).GetNormalizedSafeApprox() == Vector3(1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3(0.0f).GetNormalizedSafeExact() == Vector3(1.0f, 0.0f, 0.0f));

        v1.Set(4.0f, 3.0f, 0.0f);
        v1.Normalize();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        v1.NormalizeApprox();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        v1.NormalizeExact();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));

        v1.Set(4.0f, 3.0f, 0.0f);
        float length = v1.NormalizeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        length = v1.NormalizeWithLengthApprox();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));
        v1.Set(4.0f, 3.0f, 0.0f);
        length = v1.NormalizeWithLengthExact();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(4.0f / 5.0f, 3.0f / 5.0f, 0.0f)));

        v1.Set(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafe();
        AZ_TEST_ASSERT(v1 == Vector3(1.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafeApprox();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafeApprox();
        AZ_TEST_ASSERT(v1 == Vector3(1.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f);
        v1.NormalizeSafeExact();
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        v1.NormalizeSafeExact();
        AZ_TEST_ASSERT(v1 == Vector3(1.0f, 0.0f, 0.0f));

        v1.Set(0.0f, 3.0f, 4.0f);
        length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLength();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector3(1.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthApprox();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthApprox();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector3(1.0f, 0.0f, 0.0f));
        v1.Set(0.0f, 3.0f, 4.0f);
        length = v1.NormalizeSafeWithLengthExact();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 5.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 3.0f / 5.0f, 4.0f / 5.0f)));
        v1.Set(0.0f);
        length = v1.NormalizeSafeWithLengthExact();
        AZ_TEST_ASSERT(length == 0.0f);
        AZ_TEST_ASSERT(v1 == Vector3(1.0f, 0.0f, 0.0f));

        AZ_TEST_ASSERT(Vector3(1.0f, 0.0f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(Vector3(0.707f, 0.707f, 0.0f).IsNormalized());
        AZ_TEST_ASSERT(!Vector3(1.0f, 1.0f, 0.0f).IsNormalized());

        //setlength
        v1.Set(3.0f, 4.0f, 0.0f);
        v1.SetLength(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(6.0f, 8.0f, 0.0f)));
        v1.Set(3.0f, 4.0f, 0.0f);
        v1.SetLengthApprox(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(6.0f, 8.0f, 0.0f)));
        v1.Set(3.0f, 4.0f, 0.0f);
        v1.SetLengthExact(10.0f);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(6.0f, 8.0f, 0.0f)));

        //distance
        v1.Set(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistanceSq(Vector3(-2.0f, 6.0f, 3.0f)), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistance(Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistanceApprox(Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(v1.GetDistanceExact(Vector3(-2.0f, 2.0f, -1.0f)), 5.0f);

        //lerp/slerp
        AZ_TEST_ASSERT(Vector3(3.0f, 4.0f, 5.0f).Lerp(Vector3(10.0f, 2.0f, 1.0f), 0.5f) == Vector3(6.5f, 3.0f, 3.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 0.0f, 0.0f).Slerp(Vector3(0.0f, 1.0f, 0.0f), 0.5f).IsClose(Vector3(0.707f, 0.707f, 0.0f)));

        //dot product
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector3(1.0f, 2.0f, 3.0f).Dot(Vector3(-1.0f, 5.0f, 3.0f)), 18.0f);

        //cross products
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).Cross(Vector3(3.0f, 1.0f, -1.0f)) == Vector3(-5.0f, 10.0f, -5.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).CrossXAxis() == Vector3(0.0f, 3.0f, -2.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).CrossYAxis() == Vector3(-3.0f, 0.0f, 1.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).CrossZAxis() == Vector3(2.0f, -1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).XAxisCross() == Vector3(0.0f, -3.0f, 2.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).YAxisCross() == Vector3(3.0f, 0.0f, -1.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).ZAxisCross() == Vector3(-2.0f, 1.0f, 0.0f));

        //close
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 4.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsClose(Vector3(1.0f, 2.0f, 3.4f), 0.5f));

        //operator==
        v3.Set(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(v3 == Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(!(v3 == Vector3(1.0f, 2.0f, 4.0f)));
        AZ_TEST_ASSERT(v3 != Vector3(1.0f, 2.0f, 5.0f));
        AZ_TEST_ASSERT(!(v3 != Vector3(1.0f, 2.0f, 3.0f)));

        //comparisons
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(0.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsLessThan(Vector3(2.0f, 2.0f, 4.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(0.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsLessEqualThan(Vector3(2.0f, 2.0f, 4.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 1.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 3.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsGreaterThan(Vector3(0.0f, 2.0f, 2.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 1.0f, 2.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 3.0f, 2.0f)));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).IsGreaterEqualThan(Vector3(0.0f, 2.0f, 2.0f)));

        //min,max,clamp
        AZ_TEST_ASSERT(Vector3(2.0f, 5.0f, 6.0f).GetMin(Vector3(1.0f, 6.0f, 5.0f)) == Vector3(1.0f, 5.0f, 5.0f));
        AZ_TEST_ASSERT(Vector3(2.0f, 5.0f, 6.0f).GetMax(Vector3(1.0f, 6.0f, 5.0f)) == Vector3(2.0f, 6.0f, 6.0f));
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).GetClamp(Vector3(0.0f, -1.0f, 4.0f), Vector3(2.0f, 1.0f, 10.0f)) == Vector3(1.0f, 1.0f, 4.0f));

        //trig
        AZ_TEST_ASSERT(Vector3(DegToRad(78.0f), DegToRad(-150.0f), DegToRad(190.0f)).GetAngleMod().IsClose(Vector3(DegToRad(78.0f), DegToRad(-150.0f), DegToRad(-170.0f))));
        AZ_TEST_ASSERT(Vector3(DegToRad(390.0f), DegToRad(-190.0f), DegToRad(-400.0f)).GetAngleMod().IsClose(Vector3(DegToRad(30.0f), DegToRad(170.0f), DegToRad(-40.0f))));
        AZ_TEST_ASSERT(Vector3(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f)).GetSin().IsClose(Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        AZ_TEST_ASSERT(Vector3(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f)).GetCos().IsClose(Vector3(0.5f, -0.259f, -0.995f), 0.005f));
        Vector3 sin, cos;
        v1.Set(DegToRad(60.0f), DegToRad(105.0f), DegToRad(-174.0f));
        v1.GetSinCos(sin, cos);
        AZ_TEST_ASSERT(sin.IsClose(Vector3(0.866f, 0.966f, -0.105f), 0.005f));
        AZ_TEST_ASSERT(cos.IsClose(Vector3(0.5f, -0.259f, -0.995f), 0.005f));

        //abs
        AZ_TEST_ASSERT(Vector3(-1.0f, 2.0f, -5.0f).GetAbs() == Vector3(1.0f, 2.0f, 5.0f));

        //reciprocal
        AZ_TEST_ASSERT(Vector3(2.0f, 4.0f, 5.0f).GetReciprocal().IsClose(Vector3(0.5f, 0.25f, 0.2f)));
        AZ_TEST_ASSERT(Vector3(2.0f, 4.0f, 5.0f).GetReciprocalApprox().IsClose(Vector3(0.5f, 0.25f, 0.2f)));
        AZ_TEST_ASSERT(Vector3(2.0f, 4.0f, 5.0f).GetReciprocalExact().IsClose(Vector3(0.5f, 0.25f, 0.2f)));

        //operators
        AZ_TEST_ASSERT((-Vector3(1.0f, 2.0f, -3.0f)) == Vector3(-1.0f, -2.0f, 3.0f));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) + Vector3(-1.0f, 4.0f, 5.0f)) == Vector3(0.0f, 6.0f, 8.0f));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) - Vector3(-1.0f, 4.0f, 5.0f)) == Vector3(2.0f, -2.0f, -2.0f));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) * Vector3(-1.0f, 4.0f, 5.0f)) == Vector3(-1.0f, 8.0f, 15.0f));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) / Vector3(-1.0f, 4.0f, 5.0f)).IsClose(Vector3(-1.0f, 0.5f, 3.0f / 5.0f)));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) * 2.0f) == Vector3(2.0f, 4.0f, 6.0f));
        AZ_TEST_ASSERT((2.0f * Vector3(1.0f, 2.0f, 3.0f)) == Vector3(2.0f, 4.0f, 6.0f));
        AZ_TEST_ASSERT((Vector3(1.0f, 2.0f, 3.0f) / 2.0f).IsClose(Vector3(0.5f, 1.0f, 1.5f)));
        v1.Set(1.0f, 2.0f, 3.0f);
        v1 += Vector3(5.0f, 3.0f, -1.0f);
        AZ_TEST_ASSERT(v1 == Vector3(6.0f, 5.0f, 2.0f));
        v1 -= Vector3(2.0f, -1.0f, 3.0f);
        AZ_TEST_ASSERT(v1 == Vector3(4.0f, 6.0f, -1.0f));
        v1 *= 3.0f;
        AZ_TEST_ASSERT(v1 == Vector3(12.0f, 18.0f, -3.0f));
        v1 /= 2.0f;
        AZ_TEST_ASSERT(v1.IsClose(Vector3(6.0f, 9.0f, -1.5f)));

        //build tangent basis
        v1 = Vector3(1.0f, 1.0f, 2.0f).GetNormalized();
        v1.BuildTangentBasis(v2, v3);
        AZ_TEST_ASSERT(v2.IsNormalized());
        AZ_TEST_ASSERT(v3.IsNormalized());
        AZ_TEST_ASSERT(fabsf(v2.Dot(v1)) < 0.001f);
        AZ_TEST_ASSERT(fabsf(v3.Dot(v1)) < 0.001f);
        AZ_TEST_ASSERT(fabsf(v2.Dot(v3)) < 0.001f);

        //madd
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 3.0f).GetMadd(Vector3(2.0f, 1.0f, 4.0f), Vector3(1.0f, 2.0f, 4.0f)) == Vector3(3.0f, 4.0f, 16.0f));
        v1.Set(1.0f, 2.0f, 3.0f);
        v1.Madd(Vector3(2.0f, 1.0f, 4.0f), Vector3(1.0f, 2.0f, 4.0f));
        AZ_TEST_ASSERT(v1 == Vector3(3.0f, 4.0f, 16.0f));

        //isPerpendicular
        AZ_TEST_ASSERT(Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(Vector3(0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(!Vector3(1.0f, 2.0f, 0.0f).IsPerpendicular(Vector3(0.0f, 1.0f, 1.0f)));

        //Project
        v1.Set(0.5f, 0.5f, 0.5f);
        v1.Project(Vector3(0.0f, 2.0f, 1.0f));
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.6f, 0.3f));
        v1.Set(0.5f, 0.5f, 0.5f);
        v1.ProjectOnNormal(Vector3(0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(v1 == Vector3(0.0f, 0.5f, 0.0f));
        v1.Set(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(v1.GetProjected(Vector3(1.0f, 1.0f, 1.0f)) == Vector3(2.0f, 2.0f, 2.0f));
        AZ_TEST_ASSERT(v1.GetProjectedOnNormal(Vector3(1.0f, 0.0f, 0.0f)) == Vector3(1.0f, 0.0f, 0.0f));

        //IsFinite
        AZ_TEST_ASSERT(Vector3(1.0f, 1.0f, 1.0f).IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        AZ_TEST_ASSERT(!Vector3(infinity, infinity, infinity).IsFinite());

        //swap endian
        //v1.Set(1.0f,2.0f,3.0f);
        //v1.SwapEndian();
        //v1.SwapEndian();
        //AZ_TEST_ASSERT(v1==Vector3(1.0f,2.0f,3.0f));
    }

    TEST(MATH_Vector3, CompareTest)
    {
        Vector3 vA(-100.0f, 10.0f, 10.0f);
        Vector3 vB(35.0f, -11.0f, 10.0f);

        // compare equal
        Vector3 rEq = Vector3::CreateSelectCmpEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(rEq.IsClose(Vector3(0.0f, 0.0f, 1.0f)));

        // compare greater equal
        Vector3 rGr = Vector3::CreateSelectCmpGreaterEqual(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(rGr.IsClose(Vector3(0.0f, 1.0f, 1.0f)));

        // compare greater
        Vector3 rGrEq = Vector3::CreateSelectCmpGreater(vA, vB, Vector3(1.0f), Vector3(0.0f));
        AZ_TEST_ASSERT(rGrEq.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
    }
    
    TEST(MATH_Vector4, Test)
    {
        //constructors
        Vector4 v1(0.0f);
        AZ_TEST_ASSERT((v1.GetX() == 0.0f) && (v1.GetY() == 0.0f) && (v1.GetZ() == 0.0f) && (v1.GetW() == 0.0f));
        Vector4 v2(5.0f);
        AZ_TEST_ASSERT((v2.GetX() == 5.0f) && (v2.GetY() == 5.0f) && (v2.GetZ() == 5.0f) && (v2.GetW() == 5.0f));
        Vector4 v3(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT((v3.GetX() == 1.0f) && (v3.GetY() == 2.0f) && (v3.GetZ() == 3.0f) && (v3.GetW() == 4.0f));
        float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
        Vector4 v4 = Vector4::CreateFromFloat4(values);
        AZ_TEST_ASSERT((v4.GetX() == 10.0f) && (v4.GetY() == 20.0f) && (v4.GetZ() == 30.0f) && (v4.GetW() == 40.0f));
        Vector4 v5 = Vector4::CreateFromVector3(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT((v5.GetX() == 2.0f) && (v5.GetY() == 3.0f) && (v5.GetZ() == 4.0f) && (v5.GetW() == 1.0f));
        Vector4 v6 = Vector4::CreateFromVector3AndFloat(Vector3(2.0f, 3.0f, 4.0f), 5.0f);
        AZ_TEST_ASSERT((v6.GetX() == 2.0f) && (v6.GetY() == 3.0f) && (v6.GetZ() == 4.0f) && (v6.GetW() == 5.0f));

        //Create functions
        AZ_TEST_ASSERT(Vector4::CreateOne() == Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        AZ_TEST_ASSERT(Vector4::CreateZero() == Vector4(0.0f));
        AZ_TEST_ASSERT(Vector4::CreateAxisX() == Vector4(1.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector4::CreateAxisY() == Vector4(0.0f, 1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(Vector4::CreateAxisZ() == Vector4(0.0f, 0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(Vector4::CreateAxisW() == Vector4(0.0f, 0.0f, 0.0f, 1.0f));

        //Get/Set
        v1.Set(2.0f, 3.0f, 4.0f, 5.0f);
        AZ_TEST_ASSERT(v1 == Vector4(2.0f, 3.0f, 4.0f, 5.0f));
        v1.SetX(10.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 3.0f, 4.0f, 5.0f));
        v1.SetY(11.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 11.0f, 4.0f, 5.0f));
        v1.SetZ(12.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 11.0f, 12.0f, 5.0f));
        v1.SetW(13.0f);
        AZ_TEST_ASSERT(v1 == Vector4(10.0f, 11.0f, 12.0f, 13.0f));
        v1.Set(15.0f);
        AZ_TEST_ASSERT(v1 == Vector4(15.0f));
        v1.Set(values);
        AZ_TEST_ASSERT((v1.GetX() == 10.0f) && (v1.GetY() == 20.0f) && (v1.GetZ() == 30.0f) && (v1.GetW() == 40.0f));
        v1.Set(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT((v1.GetX() == 2.0f) && (v1.GetY() == 3.0f) && (v1.GetZ() == 4.0f) && (v1.GetW() == 1.0f));
        v1.Set(Vector3(2.0f, 3.0f, 4.0f), 5.0f);
        AZ_TEST_ASSERT((v1.GetX() == 2.0f) && (v1.GetY() == 3.0f) && (v1.GetZ() == 4.0f) && (v1.GetW() == 5.0f));

        //index operators
        v1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 1.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 2.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 3.0f);
        AZ_TEST_ASSERT(v1.GetElement(3) == 4.0f);
        AZ_TEST_ASSERT(v1(0) == 1.0f);
        AZ_TEST_ASSERT(v1(1) == 2.0f);
        AZ_TEST_ASSERT(v1(2) == 3.0f);
        AZ_TEST_ASSERT(v1(3) == 4.0f);
        v1.SetElement(0, 5.0f);
        v1.SetElement(1, 6.0f);
        v1.SetElement(2, 7.0f);
        v1.SetElement(3, 8.0f);
        AZ_TEST_ASSERT(v1.GetElement(0) == 5.0f);
        AZ_TEST_ASSERT(v1.GetElement(1) == 6.0f);
        AZ_TEST_ASSERT(v1.GetElement(2) == 7.0f);
        AZ_TEST_ASSERT(v1.GetElement(3) == 8.0f);

        //operator==
        v3.Set(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(v3 == Vector4(1.0f, 2.0f, 3.0f, 4.0));
        AZ_TEST_ASSERT(!(v3 == Vector4(1.0f, 2.0f, 3.0f, 5.0f)));
        AZ_TEST_ASSERT(v3 != Vector4(1.0f, 2.0f, 3.0f, 5.0f));
        AZ_TEST_ASSERT(!(v3 != Vector4(1.0f, 2.0f, 3.0f, 4.0f)));

        //comparisons
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(Vector4(0.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessThan(Vector4(1.0f, 2.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(Vector4(0.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsLessEqualThan(Vector4(2.0f, 2.0f, 4.0f, 5.0f)));

        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(Vector4(0.0f, 1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(Vector4(0.0f, 3.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterThan(Vector4(0.0f, 2.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(Vector4(0.0f, 1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(Vector4(0.0f, 3.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsGreaterEqualThan(Vector4(0.0f, 2.0f, 2.0f, 3.0f)));

        //dot product
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).Dot(Vector4(-1.0f, 5.0f, 3.0f, 2.0f)), 26.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(Vector4(1.0f, 2.0f, 3.0f, 4.0f).Dot3(Vector3(-1.0f, 5.0f, 3.0f)), 18.0f);

        //close
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(Vector4(1.0f, 2.0f, 3.0f, 5.0f)));
        AZ_TEST_ASSERT(Vector4(1.0f, 2.0f, 3.0f, 4.0f).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.4f), 0.5f));

        //homogenize
        v1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(v1.GetHomogenized().IsClose(Vector3(0.25f, 0.5f, 0.75f)));
        v1.Homogenize();
        AZ_TEST_ASSERT(v1.IsClose(Vector4(0.25f, 0.5f, 0.75f, 1.0f)));

        //abs
        AZ_TEST_ASSERT(Vector4(-1.0f, 2.0f, -5.0f, 1.0f).GetAbs() == Vector4(1.0f, 2.0f, 5.0f, 1.0f));
        AZ_TEST_ASSERT(Vector4(1.0f, -2.0f, 5.0f, -1.0f).GetAbs() == Vector4(1.0f, 2.0f, 5.0f, 1.0f));

        //reciprocal
        AZ_TEST_ASSERT(Vector4(2.0f, 4.0f, 5.0f, 10.0f).GetReciprocal().IsClose(Vector4(0.5f, 0.25f, 0.2f, 0.1f)));

        // operators
        AZ_TEST_ASSERT((-Vector4(1.0f, 2.0f, -3.0f, -1.0f)) == Vector4(-1.0f, -2.0f, 3.0f, 1.0f));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) + Vector4(-1.0f, 4.0f, 5.0f, 2.0f)) == Vector4(0.0f, 6.0f, 8.0f, 6.0f));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) - Vector4(-1.0f, 4.0f, 5.0f, 2.0f)) == Vector4(2.0f, -2.0f, -2.0f, 2.0f));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) * Vector4(-1.0f, 4.0f, 5.0f, 2.0f)) == Vector4(-1.0f, 8.0f, 15.0f, 8.0f));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) / Vector4(-1.0f, 4.0f, 5.0f, 2.0f)).IsClose(Vector4(-1.0f, 0.5f, 3.0f / 5.0f, 2.0f)));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f) == Vector4(2.0f, 4.0f, 6.0f, 8.0f));
        AZ_TEST_ASSERT((2.0f * Vector4(1.0f, 2.0f, 3.0f, 4.0f)) == Vector4(2.0f, 4.0f, 6.0f, 8.0f));
        AZ_TEST_ASSERT((Vector4(1.0f, 2.0f, 3.0f, 4.0f) / 2.0f).IsClose(Vector4(0.5f, 1.0f, 1.5f, 2.0f)));
        v1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        v1 += Vector4(5.0f, 3.0f, -1.0f, 2.0f);
        AZ_TEST_ASSERT(v1 == Vector4(6.0f, 5.0f, 2.0f, 6.0f));
        v1 -= Vector4(2.0f, -1.0f, 3.0f, 1.0f);
        AZ_TEST_ASSERT(v1 == Vector4(4.0f, 6.0f, -1.0f, 5.0f));
        v1 *= 3.0f;
        AZ_TEST_ASSERT(v1 == Vector4(12.0f, 18.0f, -3.0f, 15.0f));
        v1 /= 2.0f;
        AZ_TEST_ASSERT(v1.IsClose(Vector4(6.0f, 9.0f, -1.5f, 7.5f)));
    }

    TEST(MATH_Transform, Test)
    {
        int testIndices[] = { 0, 1, 2, 3 };

        //create functions
        Transform t1 = Transform::CreateIdentity();
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(1.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(0.0f, 1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(0.0f, 0.0f, 1.0f, 0.0f));
        t1 = Transform::CreateZero();
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(0.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(0.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(0.0f));
        t1 = Transform::CreateFromValue(2.0f);
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(2.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(2.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(2.0f));
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        t1 = Transform::CreateRotationY(DegToRad(30.0f));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(0.866f, 0.0f, 0.5f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(0.0f, 1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(-0.5f, 0.0f, 0.866f, 0.0f)));
        t1 = Transform::CreateRotationZ(DegToRad(30.0f));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(0.866f, -0.5f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(0.5f, 0.866f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        t1 = Transform::CreateFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        t1 = Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)), Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        t1 = Transform::CreateScale(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(1.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(0.0f, 2.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(0.0f, 0.0f, 3.0f, 0.0f));
        t1 = Transform::CreateDiagonal(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(2.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(0.0f, 3.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(0.0f, 0.0f, 4.0f, 0.0f));
        t1 = Transform::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(0.0f, 1.0f, 0.0f, 2.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(0.0f, 0.0f, 1.0f, 3.0f));
        float testFloats[16] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f };
        float testFloatMtx[16];
        t1 = Transform::CreateFromRowMajorFloat12(testFloats);
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(5.0f, 6.0f, 7.0f, 8.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(9.0f, 10.0f, 11.0f, 12.0f));
        t1.StoreToRowMajorFloat12(testFloatMtx);
        AZ_TEST_ASSERT(memcmp(testFloatMtx, testFloats, sizeof(float) * 12) == 0);
        t1 = Transform::CreateFromColumnMajorFloat12(testFloats);
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(1.0f, 4.0f, 7.0f, 10.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(2.0f, 5.0f, 8.0f, 11.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(3.0f, 6.0f, 9.0f, 12.0f));
        t1.StoreToColumnMajorFloat12(testFloatMtx);
        AZ_TEST_ASSERT(memcmp(testFloatMtx, testFloats, sizeof(float) * 12) == 0);
        t1 = Transform::CreateFromColumnMajorFloat16(testFloats);
        AZ_TEST_ASSERT(t1.GetRow(0) == Vector4(1.0f, 5.0f, 9.0f, 13.0f));
        AZ_TEST_ASSERT(t1.GetRow(1) == Vector4(2.0f, 6.0f, 10.0f, 14.0f));
        AZ_TEST_ASSERT(t1.GetRow(2) == Vector4(3.0f, 7.0f, 11.0f, 15.0f));
        t1.StoreToColumnMajorFloat16(testFloatMtx);
        AZ_TEST_ASSERT(memcmp(&testFloatMtx[0], &testFloats[0], sizeof(float) * 3) == 0);
        AZ_TEST_ASSERT(memcmp(&testFloatMtx[4], &testFloats[4], sizeof(float) * 3) == 0);
        AZ_TEST_ASSERT(memcmp(&testFloatMtx[8], &testFloats[8], sizeof(float) * 3) == 0);
        AZ_TEST_ASSERT(memcmp(&testFloatMtx[12], &testFloats[12], sizeof(float) * 3) == 0);

        //element access
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(t1.GetElement(1, 2), -0.5f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(t1.GetElement(2, 2), 0.866f);
        t1.SetElement(2, 1, 5.0f);
        AZ_TEST_ASSERT(t1.GetElement(2, 1) == 5.0f);

        //index accessors
        AZ_TEST_ASSERT_FLOAT_CLOSE(t1(1, 2), -0.5f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(t1(2, 2), 0.866f);
        t1.SetElement(2, 1, 15.0f);
        AZ_TEST_ASSERT(t1(2, 1) == 15.0f);

        //row access
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetRowAsVector3(2).IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        t1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        t1.SetRow(1, Vector3(5.0f, 6.0f, 7.0f), 8.0f);
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
        t1.SetRow(2, Vector4(3.0f, 4.0f, 5.0f, 6.0));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(3.0f, 4.0f, 5.0f, 6.0f)));
        //test GetRow with non-constant, we have different implementations for constants and variables
        AZ_TEST_ASSERT(t1.GetRow(testIndices[0]).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(t1.GetRow(testIndices[1]).IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
        AZ_TEST_ASSERT(t1.GetRow(testIndices[2]).IsClose(Vector4(3.0f, 4.0f, 5.0f, 6.0f)));
        Vector4 row0, row1, row2;
        t1.GetRows(&row0, &row1, &row2);
        AZ_TEST_ASSERT(row0.IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(row1.IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
        AZ_TEST_ASSERT(row2.IsClose(Vector4(3.0f, 4.0f, 5.0f, 6.0f)));
        t1.SetRows(Vector4(10.0f, 11.0f, 12.0f, 13.0f), Vector4(14.0f, 15.0f, 16.0f, 17.0f), Vector4(18.0f, 19.0f, 20.0f, 21.0f));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(10.0f, 11.0f, 12.0f, 13.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(14.0f, 15.0f, 16.0f, 17.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(18.0f, 19.0f, 20.0f, 21.0f)));

        //column access
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(t1.GetColumn(1).IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        t1.SetColumn(3, 1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(t1.GetColumn(3).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f))); //checking all components in case others get messed up with the shuffling
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        t1.SetColumn(0, Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(t1.GetColumn(0).IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(2.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(3.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(4.0f, 0.5f, 0.866f, 3.0f)));
        //test GetColumn with non-constant, we have different implementations for constants and variables
        AZ_TEST_ASSERT(t1.GetColumn(testIndices[0]).IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(t1.GetColumn(testIndices[1]).IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        AZ_TEST_ASSERT(t1.GetColumn(testIndices[2]).IsClose(Vector3(0.0f, -0.5f, 0.866f)));
        AZ_TEST_ASSERT(t1.GetColumn(testIndices[3]).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        Vector3 col0, col1, col2, col3;
        t1.GetColumns(&col0, &col1, &col2, &col3);
        AZ_TEST_ASSERT(col0.IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(col1.IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        AZ_TEST_ASSERT(col2.IsClose(Vector3(0.0f, -0.5f, 0.866f)));
        AZ_TEST_ASSERT(col3.IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        t1.SetColumns(Vector3(10.0f, 11.0f, 12.0f), Vector3(13.0f, 14.0f, 15.0f), Vector3(16.0f, 17.0f, 18.0f), Vector3(19.0f, 20.0f, 21.0f));
        AZ_TEST_ASSERT(t1.GetColumn(0).IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        AZ_TEST_ASSERT(t1.GetColumn(1).IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        AZ_TEST_ASSERT(t1.GetColumn(2).IsClose(Vector3(16.0f, 17.0f, 18.0f)));
        AZ_TEST_ASSERT(t1.GetColumn(3).IsClose(Vector3(19.0f, 20.0f, 21.0f)));

        //basis access
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        t1.SetPosition(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(t1.GetBasisX().IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t1.GetBasisY().IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        AZ_TEST_ASSERT(t1.GetBasisZ().IsClose(Vector3(0.0f, -0.5f, 0.866f)));
        t1.SetBasisX(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(t1.GetBasisX().IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        t1.SetBasisY(4.0f, 5.0f, 6.0f);
        AZ_TEST_ASSERT(t1.GetBasisY().IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        t1.SetBasisZ(7.0f, 8.0f, 9.0f);
        AZ_TEST_ASSERT(t1.GetBasisZ().IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        t1.SetBasisX(Vector3(10.0f, 11.0f, 12.0f));
        AZ_TEST_ASSERT(t1.GetBasisX().IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        t1.SetBasisY(Vector3(13.0f, 14.0f, 15.0f));
        AZ_TEST_ASSERT(t1.GetBasisY().IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        t1.SetBasisZ(Vector3(16.0f, 17.0f, 18.0f));
        AZ_TEST_ASSERT(t1.GetBasisZ().IsClose(Vector3(16.0f, 17.0f, 18.0f)));
        Vector3 basis0, basis1, basis2, pos;
        t1.GetBasisAndPosition(&basis0, &basis1, &basis2, &pos);
        AZ_TEST_ASSERT(basis0.IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        AZ_TEST_ASSERT(basis1.IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        AZ_TEST_ASSERT(basis2.IsClose(Vector3(16.0f, 17.0f, 18.0f)));
        AZ_TEST_ASSERT(pos.IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        t1.SetBasisAndPosition(Vector3(1.0f, 2.0f, 3.0f), Vector3(4.0f, 5.0f, 6.0f), Vector3(7.0f, 8.0f, 9.0f), Vector3(10.0f, 11.0f, 12.0f));
        AZ_TEST_ASSERT(t1.GetBasisX().IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(t1.GetBasisY().IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        AZ_TEST_ASSERT(t1.GetBasisZ().IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        AZ_TEST_ASSERT(t1.GetPosition().IsClose(Vector3(10.0f, 11.0f, 12.0f)));

        //position access
        t1 = Transform::CreateTranslation(Vector3(5.0f, 6.0f, 7.0f));
        AZ_TEST_ASSERT(t1.GetPosition().IsClose(Vector3(5.0f, 6.0f, 7.0f)));
        t1.SetPosition(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(t1.GetPosition().IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        t1.SetPosition(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(t1.GetPosition().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(t1.GetTranslation().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        t1.SetTranslation(4.0f, 5.0f, 6.0f);
        AZ_TEST_ASSERT(t1.GetTranslation().IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        t1.SetTranslation(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(t1.GetTranslation().IsClose(Vector3(2.0f, 3.0f, 4.0f)));

        //multiplication by transforms
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        t1.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));
        Transform t2 = Transform::CreateRotationY(DegToRad(30.0f));
        t2.SetTranslation(Vector3(2.0f, 3.0f, 4.0f));
        Transform t3 = t1 * t2;
        AZ_TEST_ASSERT(t3.GetRow(0).IsClose(Vector4(0.866f, 0.0f, 0.5f, 3.0f)));
        AZ_TEST_ASSERT(t3.GetRow(1).IsClose(Vector4(0.25f, 0.866f, -0.433f, 2.598f)));
        AZ_TEST_ASSERT(t3.GetRow(2).IsClose(Vector4(-0.433f, 0.5f, 0.75f, 7.964f)));
        Transform t4 = t1;
        t4 *= t2;
        AZ_TEST_ASSERT(t4.GetRow(0).IsClose(Vector4(0.866f, 0.0f, 0.5f, 3.0f)));
        AZ_TEST_ASSERT(t4.GetRow(1).IsClose(Vector4(0.25f, 0.866f, -0.433f, 2.598f)));
        AZ_TEST_ASSERT(t4.GetRow(2).IsClose(Vector4(-0.433f, 0.5f, 0.75f, 7.964f)));

        //multiplication by vectors
        AZ_TEST_ASSERT((t1 * Vector3(1.0f, 2.0f, 3.0f)).IsClose(Vector3(2.0f, 2.224f, 6.598f), 0.02f));
        AZ_TEST_ASSERT((t1 * Vector4(1.0f, 2.0f, 3.0f, 4.0f)).IsClose(Vector4(5.0f, 8.224f, 15.598f, 4.0f), 0.02f));
        AZ_TEST_ASSERT(t1.TransposedMultiply3x3(Vector3(1.0f, 2.0f, 3.0f)).IsClose(Vector3(1.0f, 3.224f, 1.598f), 0.02f));
        AZ_TEST_ASSERT(t1.Multiply3x3(Vector3(1.0f, 2.0f, 3.0f)).IsClose(Vector3(1.0f, 0.224f, 3.598f), 0.02f));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT((v1 * t1).IsClose(Vector3(1.0f, 3.224f, 1.598f), 0.02f));
        v1 *= t1;
        AZ_TEST_ASSERT(v1.IsClose(Vector3(1.0f, 3.224f, 1.598f), 0.02f));
        Vector4 v2(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT((v2 * t1).IsClose(Vector4(1.0f, 3.224f, 1.598f, 18.0f), 0.02f));
        v2 *= t1;
        AZ_TEST_ASSERT(v2.IsClose(Vector4(1.0f, 3.224f, 1.598f, 18.0f), 0.02f));

        //transpose
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        t1.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));
        t2 = t1.GetTranspose();
        AZ_TEST_ASSERT(t2.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t2.GetRow(1).IsClose(Vector4(0.0f, 0.866f, 0.5f, 0.0f)));
        AZ_TEST_ASSERT(t2.GetRow(2).IsClose(Vector4(0.0f, -0.5f, 0.866f, 0.0f)));
        t2 = t1;
        t2.Transpose();
        AZ_TEST_ASSERT(t2.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(t2.GetRow(1).IsClose(Vector4(0.0f, 0.866f, 0.5f, 0.0f)));
        AZ_TEST_ASSERT(t2.GetRow(2).IsClose(Vector4(0.0f, -0.5f, 0.866f, 0.0f)));
        t3 = t1.GetTranspose3x3();
        AZ_TEST_ASSERT(t3.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(t3.GetRow(1).IsClose(Vector4(0.0f, 0.866f, 0.5f, 2.0f)));
        AZ_TEST_ASSERT(t3.GetRow(2).IsClose(Vector4(0.0f, -0.5f, 0.866f, 3.0f)));
        t3 = t1;
        t3.Transpose3x3();
        AZ_TEST_ASSERT(t3.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(t3.GetRow(1).IsClose(Vector4(0.0f, 0.866f, 0.5f, 2.0f)));
        AZ_TEST_ASSERT(t3.GetRow(2).IsClose(Vector4(0.0f, -0.5f, 0.866f, 3.0f)));

        //test fast inverse, orthogonal matrix with translation only
        t1 = Transform::CreateRotationX(1.0f);
        t1.SetPosition(Vector3(10.0f, -3.0f, 5.0f));
        AZ_TEST_ASSERT((t1 * t1.GetInverseFast()).IsClose(Transform::CreateIdentity(), 0.02f));
        t2 = Transform::CreateRotationZ(2.0f) * Transform::CreateRotationX(1.0f);
        t2.SetPosition(Vector3(-5.0f, 4.2f, -32.0f));
        t3 = t2.GetInverseFast();
        // allow a little bigger threshold, because of the 2 rot matrices (sin,cos differences)
        AZ_TEST_ASSERT((t2 * t3).IsClose(Transform::CreateIdentity(), 0.1f));
        AZ_TEST_ASSERT(t3.GetRow(0).IsClose(Vector4(-0.420f, 0.909f, 0.0f, -5.920f), 0.06f));
        AZ_TEST_ASSERT(t3.GetRow(1).IsClose(Vector4(-0.493f, -0.228f, 0.841f, 25.418f), 0.06f));
        AZ_TEST_ASSERT(t3.GetRow(2).IsClose(Vector4(0.765f, 0.353f, 0.542f, 19.703f), 0.06f));

        //test inverse, should handle non-orthogonal matrices
        t1.SetElement(0, 1, 23.1234f);
        AZ_TEST_ASSERT((t1 * t1.GetInverseFull()).IsClose(Transform::CreateIdentity()));

        //scale access
        t1 = Transform::CreateRotationX(DegToRad(40.0f)) * Transform::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(t1.RetrieveScale().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(t1.ExtractScale().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(t1.RetrieveScale().IsClose(Vector3::CreateOne()));
        t1.MultiplyByScale(Vector3(3.0f, 4.0f, 5.0f));
        AZ_TEST_ASSERT(t1.RetrieveScale().IsClose(Vector3(3.0f, 4.0f, 5.0f)));

        //polar decomposition
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        t1.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));
        t2 = Transform::CreateScale(Vector3(5.0f, 6.0f, 7.0f));
        t3 = t1 * t2;
        AZ_TEST_ASSERT(t3.GetPolarDecomposition().IsClose(t1));
        Transform t5;
        t3.GetPolarDecomposition(&t4, &t5);
        AZ_TEST_ASSERT((t4 * t5).IsClose(t3));
        AZ_TEST_ASSERT(t4.IsClose(t1));
        AZ_TEST_ASSERT(t5.IsClose(t2, 0.01f));

        //orthogonalize
        t1 = Transform::CreateRotationX(AZ::DegToRad(30.0f)) * Transform::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        t1.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));
        t1.SetElement(0, 1, 0.2f);
        t2 = t1.GetOrthogonalized();
        AZ_TEST_ASSERT(t2.GetColumn(0).GetLength().IsClose(1.0f));
        AZ_TEST_ASSERT(t2.GetColumn(1).GetLength().IsClose(1.0f));
        AZ_TEST_ASSERT(t2.GetColumn(2).GetLength().IsClose(1.0f));
        AZ_TEST_ASSERT(t2.GetColumn(0).Dot(t2.GetColumn(1)).IsClose(0.0f));
        AZ_TEST_ASSERT(t2.GetColumn(0).Dot(t2.GetColumn(2)).IsClose(0.0f));
        AZ_TEST_ASSERT(t2.GetColumn(1).Dot(t2.GetColumn(2)).IsClose(0.0f));
        AZ_TEST_ASSERT(t2.GetColumn(0).Cross(t2.GetColumn(1)).IsClose(t2.GetColumn(2)));
        AZ_TEST_ASSERT(t2.GetColumn(1).Cross(t2.GetColumn(2)).IsClose(t2.GetColumn(0)));
        AZ_TEST_ASSERT(t2.GetColumn(2).Cross(t2.GetColumn(0)).IsClose(t2.GetColumn(1)));
        t1.Orthogonalize();
        AZ_TEST_ASSERT(t1.IsClose(t2));

        //orthogonal
        t1 = Transform::CreateRotationX(AZ::DegToRad(30.0f));
        t1.SetTranslation(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(t1.IsOrthogonal(0.05f));
        t1.SetRow(1, t1.GetRow(1) * 2.0f);
        AZ_TEST_ASSERT(!t1.IsOrthogonal(0.05f));
        t1 = Transform::CreateRotationX(AZ::DegToRad(30.0f));
        t1.SetRow(1, Vector4(0.0f, 1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(!t1.IsOrthogonal(0.05f));

        //IsClose
        t1 = Transform::CreateRotationX(DegToRad(30.0f));
        t2 = t1;
        AZ_TEST_ASSERT(t1.IsClose(t2));
        t2.SetElement(0, 0, 2.0f);
        AZ_TEST_ASSERT(!t1.IsClose(t2));
        t2 = t1;
        t2.SetElement(0, 3, 2.0f);
        AZ_TEST_ASSERT(!t1.IsClose(t2));

        //set rotation part
        t1 = Transform::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
        t1.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        AZ_TEST_ASSERT(t1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(t1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(t1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));

        //determinant
        t1.SetRow(0, -1.0f, 2.0f, 3.0f, 1.0f);
        t1.SetRow(1, 4.0f, 5.0f, 6.0f, 2.0f);
        t1.SetRow(2, 7.0f, 8.0f, -9.0f, 3.0f);
        AZ_TEST_ASSERT(t1.GetDeterminant3x3().IsClose(240.0f));
    }

    TEST(MATH_Matrix4x4, Test)
    {
        int testIndices[] = { 0, 1, 2, 3 };

        //create functions
        Matrix4x4 m1 = Matrix4x4::CreateIdentity();
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(1.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(0.0f, 1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(0.0f, 0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(0.0f, 0.0f, 0.0f, 1.0f));
        m1 = Matrix4x4::CreateZero();
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(0.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(0.0f));
        m1 = Matrix4x4::CreateFromValue(2.0f);
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(2.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(2.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(2.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(2.0f));

        float testFloats[] = {
            1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
            9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f
        };
        float testFloatMtx[16];
        m1 = Matrix4x4::CreateFromRowMajorFloat16(testFloats);
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(1.0f, 2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(5.0f, 6.0f, 7.0f, 8.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(9.0f, 10.0f, 11.0f, 12.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(13.0f, 14.0f, 15.0f, 16.0f));
        m1.StoreToRowMajorFloat16(testFloatMtx);
        AZ_TEST_ASSERT(memcmp(testFloatMtx, testFloats, sizeof(testFloatMtx)) == 0);
        m1 = Matrix4x4::CreateFromColumnMajorFloat16(testFloats);
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(1.0f, 5.0f, 9.0f, 13.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(2.0f, 6.0f, 10.0f, 14.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(3.0f, 7.0f, 11.0f, 15.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(4.0f, 8.0f, 12.0f, 16.0f));
        m1.StoreToColumnMajorFloat16(testFloatMtx);
        AZ_TEST_ASSERT(memcmp(testFloatMtx, testFloats, sizeof(testFloatMtx)) == 0);

        m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateRotationY(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(0.866f, 0.0f, 0.5f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(-0.5f, 0.0f, 0.866f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateRotationZ(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(0.866f, -0.5f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.5f, 0.866f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)), Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
        m1 = Matrix4x4::CreateScale(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(1.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(0.0f, 2.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(0.0f, 0.0f, 3.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(0.0f, 0.0f, 0.0f, 1.0f));
        m1 = Matrix4x4::CreateDiagonal(Vector4(2.0f, 3.0f, 4.0f, 5.0f));
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(2.0f, 0.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(0.0f, 3.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(0.0f, 0.0f, 4.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(0.0f, 0.0f, 0.0f, 5.0f));
        m1 = Matrix4x4::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector4(0.0f, 1.0f, 0.0f, 2.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector4(0.0f, 0.0f, 1.0f, 3.0f));
        AZ_TEST_ASSERT(m1.GetRow(3) == Vector4(0.0f, 0.0f, 0.0f, 1.0f));

        m1 = Matrix4x4::CreateProjection(DegToRad(30.0f), 16.0f / 9.0f, 1.0f, 1000.0f);
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(-2.1f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 3.732f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.0f, 1.002f, -2.002f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        m1 = Matrix4x4::CreateProjectionFov(DegToRad(30.0f), DegToRad(60.0f), 1.0f, 1000.0f);
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(-3.732f, 0.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 1.732f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.0f, 1.002f, -2.002f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));
        m1 = Matrix4x4::CreateProjectionOffset(0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 1000.0f);
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(-4.0f, 0.0f, -3.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 4.0f, -1.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.0f, 1.002f, -2.002f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 1.0f, 0.0f)));

        //element access
        m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1.GetElement(1, 2), -0.5f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1.GetElement(2, 2), 0.866f);
        m1.SetElement(2, 1, 5.0f);
        AZ_TEST_ASSERT(m1.GetElement(2, 1) == 5.0f);

        //index accessors
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1(1, 2), -0.5f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1(2, 2), 0.866f);
        m1.SetElement(2, 1, 15.0f);
        AZ_TEST_ASSERT(m1(2, 1) == 15.0f);

        //row access
        m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRowAsVector3(2).IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        m1.SetRow(1, Vector3(5.0f, 6.0f, 7.0f), 8.0f);
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
        m1.SetRow(2, Vector4(3.0f, 4.0f, 5.0f, 6.0));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(3.0f, 4.0f, 5.0f, 6.0f)));
        m1.SetRow(3, Vector4(7.0f, 8.0f, 9.0f, 10.0));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(7.0f, 8.0f, 9.0f, 10.0f)));
        //test GetRow with non-constant, we have different implementations for constants and variables
        AZ_TEST_ASSERT(m1.GetRow(testIndices[0]).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(m1.GetRow(testIndices[1]).IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));
        AZ_TEST_ASSERT(m1.GetRow(testIndices[2]).IsClose(Vector4(3.0f, 4.0f, 5.0f, 6.0f)));
        AZ_TEST_ASSERT(m1.GetRow(testIndices[3]).IsClose(Vector4(7.0f, 8.0f, 9.0f, 10.0f)));

        //column access
        m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetColumn(1).IsClose(Vector4(0.0f, 0.866f, 0.5f, 0.0f)));
        m1.SetColumn(3, 1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(m1.GetColumn(3).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(m1.GetColumnAsVector3(3).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f))); //checking all components in case others get messed up with the shuffling
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 0.0f, 4.0f)));
        m1.SetColumn(0, Vector4(2.0f, 3.0f, 4.0f, 5.0f));
        AZ_TEST_ASSERT(m1.GetColumn(0).IsClose(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(2.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(3.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(4.0f, 0.5f, 0.866f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(5.0f, 0.0f, 0.0f, 4.0f)));
        //test GetColumn with non-constant, we have different implementations for constants and variables
        AZ_TEST_ASSERT(m1.GetColumn(testIndices[0]).IsClose(Vector4(2.0f, 3.0f, 4.0f, 5.0f)));
        AZ_TEST_ASSERT(m1.GetColumn(testIndices[1]).IsClose(Vector4(0.0f, 0.866f, 0.5f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetColumn(testIndices[2]).IsClose(Vector4(0.0f, -0.5f, 0.866f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetColumn(testIndices[3]).IsClose(Vector4(1.0f, 2.0f, 3.0f, 4.0f)));

        //position access
        m1 = Matrix4x4::CreateTranslation(Vector3(5.0f, 6.0f, 7.0f));
        AZ_TEST_ASSERT(m1.GetPosition().IsClose(Vector3(5.0f, 6.0f, 7.0f)));
        m1.SetPosition(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(m1.GetPosition().IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetPosition(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(m1.GetPosition().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(m1.GetTranslation().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        m1.SetTranslation(4.0f, 5.0f, 6.0f);
        AZ_TEST_ASSERT(m1.GetTranslation().IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        m1.SetTranslation(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(m1.GetTranslation().IsClose(Vector3(2.0f, 3.0f, 4.0f)));

        //multiplication by matrices
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        Matrix4x4 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f, 10.0f);
        m2.SetRow(1, 11.0f, 12.0f, 13.0f, 14.0f);
        m2.SetRow(2, 15.0f, 16.0f, 17.0f, 18.0f);
        m2.SetRow(3, 19.0f, 20.0f, 21.0f, 22.0f);
        Matrix4x4 m3 = m1 * m2;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector4(150.0f, 160.0f, 170.0f, 180.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector4(358.0f, 384.0f, 410.0f, 436.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector4(566.0f, 608.0f, 650.0f, 692.0f)));
        AZ_TEST_ASSERT(m3.GetRow(3).IsClose(Vector4(774.0f, 832.0f, 890.0f, 948.0f)));
        Matrix4x4 m4 = m1;
        m4 *= m2;
        AZ_TEST_ASSERT(m4.GetRow(0).IsClose(Vector4(150.0f, 160.0f, 170.0f, 180.0f)));
        AZ_TEST_ASSERT(m4.GetRow(1).IsClose(Vector4(358.0f, 384.0f, 410.0f, 436.0f)));
        AZ_TEST_ASSERT(m4.GetRow(2).IsClose(Vector4(566.0f, 608.0f, 650.0f, 692.0f)));
        AZ_TEST_ASSERT(m4.GetRow(3).IsClose(Vector4(774.0f, 832.0f, 890.0f, 948.0f)));

        //multiplication by vectors
        AZ_TEST_ASSERT((m1 * Vector3(1.0f, 2.0f, 3.0f)).IsClose(Vector3(18.0f, 46.0f, 74.0f)));
        AZ_TEST_ASSERT((m1 * Vector4(1.0f, 2.0f, 3.0f, 4.0f)).IsClose(Vector4(30.0f, 70.0f, 110.0f, 150.0f)));
        AZ_TEST_ASSERT(m1.TransposedMultiply3x3(Vector3(1.0f, 2.0f, 3.0f)).IsClose(Vector3(38.0f, 44.0f, 50.0f)));
        AZ_TEST_ASSERT(m1.Multiply3x3(Vector3(1.0f, 2.0f, 3.0f)).IsClose(Vector3(14.0f, 38.0f, 62.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT((v1 * m1).IsClose(Vector3(51.0f, 58.0f, 65.0f)));
        v1 *= m1;
        AZ_TEST_ASSERT(v1.IsClose(Vector3(51.0f, 58.0f, 65.0f)));
        Vector4 v2(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT((v2 * m1).IsClose(Vector4(90.0f, 100.0f, 110.0f, 120.0f)));
        v2 *= m1;
        AZ_TEST_ASSERT(v2.IsClose(Vector4(90.0f, 100.0f, 110.0f, 120.0f)));

        //transpose
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        m2 = m1.GetTranspose();
        AZ_TEST_ASSERT(m2.GetRow(0).IsClose(Vector4(1.0f, 5.0f, 9.0f, 13.0f)));
        AZ_TEST_ASSERT(m2.GetRow(1).IsClose(Vector4(2.0f, 6.0f, 10.0f, 14.0f)));
        AZ_TEST_ASSERT(m2.GetRow(2).IsClose(Vector4(3.0f, 7.0f, 11.0f, 15.0f)));
        AZ_TEST_ASSERT(m2.GetRow(3).IsClose(Vector4(4.0f, 8.0f, 12.0f, 16.0f)));
        m2 = m1;
        m2.Transpose();
        AZ_TEST_ASSERT(m2.GetRow(0).IsClose(Vector4(1.0f, 5.0f, 9.0f, 13.0f)));
        AZ_TEST_ASSERT(m2.GetRow(1).IsClose(Vector4(2.0f, 6.0f, 10.0f, 14.0f)));
        AZ_TEST_ASSERT(m2.GetRow(2).IsClose(Vector4(3.0f, 7.0f, 11.0f, 15.0f)));
        AZ_TEST_ASSERT(m2.GetRow(3).IsClose(Vector4(4.0f, 8.0f, 12.0f, 16.0f)));

        //test fast inverse, orthogonal matrix with translation only
        m1 = Matrix4x4::CreateRotationX(1.0f);
        m1.SetPosition(Vector3(10.0f, -3.0f, 5.0f));
        AZ_TEST_ASSERT((m1 * m1.GetInverseFast()).IsClose(Matrix4x4::CreateIdentity(), 0.02f));
        m2 = Matrix4x4::CreateRotationZ(2.0f) * Matrix4x4::CreateRotationX(1.0f);
        m2.SetPosition(Vector3(-5.0f, 4.2f, -32.0f));
        m3 = m2.GetInverseFast();
        // allow a little bigger threshold, because of the 2 rot matrices (sin,cos differences)
        AZ_TEST_ASSERT((m2 * m3).IsClose(Matrix4x4::CreateIdentity(), 0.1f));
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector4(-0.420f, 0.909f, 0.0f, -5.920f), 0.06f));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector4(-0.493f, -0.228f, 0.841f, 25.418f), 0.06f));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector4(0.765f, 0.353f, 0.542f, 19.703f), 0.06f));
        AZ_TEST_ASSERT(m3.GetRow(3).IsClose(Vector4(0.0f, 0.0f, 0.0f, 1.0f)));

        //test transform inverse, should handle non-orthogonal matrices, last row must still be (0,0,0,1) though
        m1.SetElement(0, 1, 23.1234f);
        AZ_TEST_ASSERT((m1 * m1.GetInverseTransform()).IsClose(Matrix4x4::CreateIdentity(), 0.01f));

        //test full inverse, any arbitrary matrix
        m1.SetRow(0, -1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 3.0f, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, -16.0f);
        AZ_TEST_ASSERT((m1 * m1.GetInverseFull()).IsClose(Matrix4x4::CreateIdentity()));

        //IsClose
        m1 = Matrix4x4::CreateRotationX(DegToRad(30.0f));
        m2 = m1;
        AZ_TEST_ASSERT(m1.IsClose(m2));
        m2.SetElement(0, 0, 2.0f);
        AZ_TEST_ASSERT(!m1.IsClose(m2));
        m2 = m1;
        m2.SetElement(0, 3, 2.0f);
        AZ_TEST_ASSERT(!m1.IsClose(m2));

        //set rotation part
        m1 = Matrix4x4::CreateTranslation(Vector3(1.0f, 2.0f, 3.0f));
        m1.SetRow(3, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRotationPartFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector4(1.0f, 0.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector4(0.0f, 0.866f, -0.5f, 2.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector4(0.0f, 0.5f, 0.866f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetRow(3).IsClose(Vector4(5.0f, 6.0f, 7.0f, 8.0f)));

        //get diagonal
        m1.SetRow(0, 1.0f, 2.0f, 3.0f, 4.0f);
        m1.SetRow(1, 5.0f, 6.0f, 7.0f, 8.0f);
        m1.SetRow(2, 9.0f, 10.0f, 11.0f, 12.0f);
        m1.SetRow(3, 13.0f, 14.0f, 15.0f, 16.0f);
        AZ_TEST_ASSERT(m1.GetDiagonal() == Vector4(1.0f, 6.0f, 11.0f, 16.0f));
    }

    TEST(MATH_Matrix3x3, Test)
    {
        int testIndices[] = { 0, 1, 2, 3 };

        //create functions
        Matrix3x3 m1 = Matrix3x3::CreateIdentity();
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(0.0f, 0.0f, 1.0f));
        m1 = Matrix3x3::CreateZero();
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(0.0f));
        m1 = Matrix3x3::CreateFromValue(2.0f);
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(2.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(2.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(2.0f));

        float testFloats[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };
        float testFloatsMtx[9];
        m1 = Matrix3x3::CreateFromRowMajorFloat9(testFloats);
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(4.0f, 5.0f, 6.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(7.0f, 8.0f, 9.0f));
        m1.StoreToRowMajorFloat9(testFloatsMtx);
        AZ_TEST_ASSERT(memcmp(testFloatsMtx, testFloats, sizeof(testFloatsMtx)) == 0);
        m1 = Matrix3x3::CreateFromColumnMajorFloat9(testFloats);
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(1.0f, 4.0f, 7.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(2.0f, 5.0f, 8.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(3.0f, 6.0f, 9.0f));
        m1.StoreToColumnMajorFloat9(testFloatsMtx);
        AZ_TEST_ASSERT(memcmp(testFloatsMtx, testFloats, sizeof(testFloatsMtx)) == 0);

        m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1 = Matrix3x3::CreateRotationY(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(0.866f, 0.0f, 0.5f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(-0.5f, 0.0f, 0.866f)));
        m1 = Matrix3x3::CreateRotationZ(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(0.866f, -0.5f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(0.5f, 0.866f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(0.0f, 0.0f, 1.0f)));
        m1 = Matrix3x3::CreateFromTransform(Transform::CreateRotationX(DegToRad(30.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1 = Matrix3x3::CreateFromMatrix4x4(Matrix4x4::CreateRotationX(DegToRad(30.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1 = Matrix3x3::CreateFromQuaternion(AZ::Quaternion::CreateRotationX(DegToRad(30.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(0.0f, 0.866f, -0.5f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1 = Matrix3x3::CreateScale(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(1.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(0.0f, 2.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(0.0f, 0.0f, 3.0f));
        m1 = Matrix3x3::CreateDiagonal(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(2.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(0.0f, 3.0f, 0.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(0.0f, 0.0f, 4.0f));
        m1 = Matrix3x3::CreateCrossProduct(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(m1.GetRow(0) == Vector3(0.0f, -3.0f, 2.0f));
        AZ_TEST_ASSERT(m1.GetRow(1) == Vector3(3.0f, 0.0f, -1.0f));
        AZ_TEST_ASSERT(m1.GetRow(2) == Vector3(-2.0f, 1.0f, 0.0f));

        //element access
        m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1.GetElement(1, 2), -0.5f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1.GetElement(2, 2), 0.866f);
        m1.SetElement(2, 1, 5.0f);
        AZ_TEST_ASSERT(m1.GetElement(2, 1) == 5.0f);

        //index accessors
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1(1, 2), -0.5f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(m1(2, 2), 0.866f);
        m1.SetElement(2, 1, 15.0f);
        AZ_TEST_ASSERT(m1(2, 1) == 15.0f);

        //row access
        m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(0.0f, 0.5f, 0.866f)));
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetRow(1, Vector3(4.0f, 5.0f, 6.0f));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        m1.SetRow(2, Vector3(7.0f, 8.0f, 9.0f));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        //test GetRow with non-constant, we have different implementations for constants and variables
        AZ_TEST_ASSERT(m1.GetRow(testIndices[0]).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetRow(testIndices[1]).IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        AZ_TEST_ASSERT(m1.GetRow(testIndices[2]).IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        Vector3 row0, row1, row2;
        m1.GetRows(&row0, &row1, &row2);
        AZ_TEST_ASSERT(row0.IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(row1.IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        AZ_TEST_ASSERT(row2.IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        m1.SetRows(Vector3(10.0f, 11.0f, 12.0f), Vector3(13.0f, 14.0f, 15.0f), Vector3(16.0f, 17.0f, 18.0f));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(16.0f, 17.0f, 18.0f)));

        //column access
        m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetColumn(1).IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        m1.SetColumn(2, 1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(m1.GetColumn(2).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(1.0f, 0.0f, 1.0f)));  //checking all components in case others get messed up with the shuffling
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(0.0f, 0.866f, 2.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(0.0f, 0.5f, 3.0f)));
        m1.SetColumn(0, Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(m1.GetColumn(0).IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(m1.GetRow(0).IsClose(Vector3(2.0f, 0.0f, 1.0f)));
        AZ_TEST_ASSERT(m1.GetRow(1).IsClose(Vector3(3.0f, 0.866f, 2.0f)));
        AZ_TEST_ASSERT(m1.GetRow(2).IsClose(Vector3(4.0f, 0.5f, 3.0f)));
        //test GetColumn with non-constant, we have different implementations for constants and variables
        AZ_TEST_ASSERT(m1.GetColumn(testIndices[0]).IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(m1.GetColumn(testIndices[1]).IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        AZ_TEST_ASSERT(m1.GetColumn(testIndices[2]).IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        Vector3 col0, col1, col2;
        m1.GetColumns(&col0, &col1, &col2);
        AZ_TEST_ASSERT(col0.IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(col1.IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        AZ_TEST_ASSERT(col2.IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetColumns(Vector3(10.0f, 11.0f, 12.0f), Vector3(13.0f, 14.0f, 15.0f), Vector3(16.0f, 17.0f, 18.0f));
        AZ_TEST_ASSERT(m1.GetColumn(0).IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        AZ_TEST_ASSERT(m1.GetColumn(1).IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        AZ_TEST_ASSERT(m1.GetColumn(2).IsClose(Vector3(16.0f, 17.0f, 18.0f)));

        //basis access
        m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.GetBasisX().IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(m1.GetBasisY().IsClose(Vector3(0.0f, 0.866f, 0.5f)));
        AZ_TEST_ASSERT(m1.GetBasisZ().IsClose(Vector3(0.0f, -0.5f, 0.866f)));
        m1.SetBasisX(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT(m1.GetBasisX().IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        m1.SetBasisY(4.0f, 5.0f, 6.0f);
        AZ_TEST_ASSERT(m1.GetBasisY().IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        m1.SetBasisZ(7.0f, 8.0f, 9.0f);
        AZ_TEST_ASSERT(m1.GetBasisZ().IsClose(Vector3(7.0f, 8.0f, 9.0f)));
        m1.SetBasisX(Vector3(10.0f, 11.0f, 12.0f));
        AZ_TEST_ASSERT(m1.GetBasisX().IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        m1.SetBasisY(Vector3(13.0f, 14.0f, 15.0f));
        AZ_TEST_ASSERT(m1.GetBasisY().IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        m1.SetBasisZ(Vector3(16.0f, 17.0f, 18.0f));
        AZ_TEST_ASSERT(m1.GetBasisZ().IsClose(Vector3(16.0f, 17.0f, 18.0f)));
        Vector3 basis0, basis1, basis2;
        m1.GetBasis(&basis0, &basis1, &basis2);
        AZ_TEST_ASSERT(basis0.IsClose(Vector3(10.0f, 11.0f, 12.0f)));
        AZ_TEST_ASSERT(basis1.IsClose(Vector3(13.0f, 14.0f, 15.0f)));
        AZ_TEST_ASSERT(basis2.IsClose(Vector3(16.0f, 17.0f, 18.0f)));
        m1.SetBasis(Vector3(1.0f, 2.0f, 3.0f), Vector3(4.0f, 5.0f, 6.0f), Vector3(7.0f, 8.0f, 9.0f));
        AZ_TEST_ASSERT(m1.GetBasisX().IsClose(Vector3(1.0f, 2.0f, 3.0f)));
        AZ_TEST_ASSERT(m1.GetBasisY().IsClose(Vector3(4.0f, 5.0f, 6.0f)));
        AZ_TEST_ASSERT(m1.GetBasisZ().IsClose(Vector3(7.0f, 8.0f, 9.0f)));

        //multiplication by matrices
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        Matrix3x3 m2;
        m2.SetRow(0, 7.0f, 8.0f, 9.0f);
        m2.SetRow(1, 10.0f, 11.0f, 12.0f);
        m2.SetRow(2, 13.0f, 14.0f, 15.0f);
        Matrix3x3 m3 = m1 * m2;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(66.0f, 72.0f, 78.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(156.0f, 171.0f, 186.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(246.0f, 270.0f, 294.0f)));
        Matrix3x3 m4 = m1;
        m4 *= m2;
        AZ_TEST_ASSERT(m4.GetRow(0).IsClose(Vector3(66.0f, 72.0f, 78.0f)));
        AZ_TEST_ASSERT(m4.GetRow(1).IsClose(Vector3(156.0f, 171.0f, 186.0f)));
        AZ_TEST_ASSERT(m4.GetRow(2).IsClose(Vector3(246.0f, 270.0f, 294.0f)));
        m3 = m1.TransposedMultiply(m2);
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(138.0f, 150.0f, 162.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(168.0f, 183.0f, 198.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(198.0f, 216.0f, 234.0f)));

        //multiplication by vectors
        AZ_TEST_ASSERT((m1 * Vector3(1.0f, 2.0f, 3.0f)).IsClose(Vector3(14.0f, 32.0f, 50.0f)));
        Vector3 v1(1.0f, 2.0f, 3.0f);
        AZ_TEST_ASSERT((v1 * m1).IsClose(Vector3(30.0f, 36.0f, 42.0f)));
        v1 *= m1;
        AZ_TEST_ASSERT(v1.IsClose(Vector3(30.0f, 36.0f, 42.0f)));

        //other matrix operations
        m3 = m1 + m2;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(20.0f, 22.0f, 24.0f)));
        m3 = m1;
        m3 += m2;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(20.0f, 22.0f, 24.0f)));
        m3 = m1 - m2;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        m3 = m1;
        m3 -= m2;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(-6.0f, -6.0f, -6.0f)));
        m3 = m1 * 2.0f;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = m1;
        m3 *= 2.0f;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = 2.0f * m1;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = m1 / 0.5f;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = m1;
        m3 /= 0.5f;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(2.0f, 4.0f, 6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(8.0f, 10.0f, 12.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(14.0f, 16.0f, 18.0f)));
        m3 = -m1;
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(-1.0f, -2.0f, -3.0f)));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(-4.0f, -5.0f, -6.0f)));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(-7.0f, -8.0f, -9.0f)));

        //transpose
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        m2 = m1.GetTranspose();
        AZ_TEST_ASSERT(m2.GetRow(0).IsClose(Vector3(1.0f, 4.0f, 7.0f)));
        AZ_TEST_ASSERT(m2.GetRow(1).IsClose(Vector3(2.0f, 5.0f, 8.0f)));
        AZ_TEST_ASSERT(m2.GetRow(2).IsClose(Vector3(3.0f, 6.0f, 9.0f)));
        m2 = m1;
        m2.Transpose();
        AZ_TEST_ASSERT(m2.GetRow(0).IsClose(Vector3(1.0f, 4.0f, 7.0f)));
        AZ_TEST_ASSERT(m2.GetRow(1).IsClose(Vector3(2.0f, 5.0f, 8.0f)));
        AZ_TEST_ASSERT(m2.GetRow(2).IsClose(Vector3(3.0f, 6.0f, 9.0f)));

        //test fast inverse, orthogonal matrix only
        m1 = Matrix3x3::CreateRotationX(1.0f);
        AZ_TEST_ASSERT((m1 * m1.GetInverseFast()).IsClose(Matrix3x3::CreateIdentity(), 0.02f));
        m2 = Matrix3x3::CreateRotationZ(2.0f) * Matrix3x3::CreateRotationX(1.0f);
        m3 = m2.GetInverseFast();
        // allow a little bigger threshold, because of the 2 rot matrices (sin,cos differences)
        AZ_TEST_ASSERT((m2 * m3).IsClose(Matrix3x3::CreateIdentity(), 0.1f));
        AZ_TEST_ASSERT(m3.GetRow(0).IsClose(Vector3(-0.420f, 0.909f, 0.0f), 0.06f));
        AZ_TEST_ASSERT(m3.GetRow(1).IsClose(Vector3(-0.493f, -0.228f, 0.841f), 0.06f));
        AZ_TEST_ASSERT(m3.GetRow(2).IsClose(Vector3(0.765f, 0.353f, 0.542f), 0.06f));

        //test full inverse, any arbitrary matrix
        m1.SetRow(0, -1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, -9.0f);
        AZ_TEST_ASSERT((m1 * m1.GetInverseFull()).IsClose(Matrix3x3::CreateIdentity()));

        //scale access
        m1 = Matrix3x3::CreateRotationX(DegToRad(40.0f)) * Matrix3x3::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        AZ_TEST_ASSERT(m1.RetrieveScale().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(m1.ExtractScale().IsClose(Vector3(2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(m1.RetrieveScale().IsClose(Vector3::CreateOne()));
        m1.MultiplyByScale(Vector3(3.0f, 4.0f, 5.0f));
        AZ_TEST_ASSERT(m1.RetrieveScale().IsClose(Vector3(3.0f, 4.0f, 5.0f)));

        //polar decomposition
        m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        m2 = Matrix3x3::CreateScale(Vector3(5.0f, 6.0f, 7.0f));
        m3 = m1 * m2;
        AZ_TEST_ASSERT(m3.GetPolarDecomposition().IsClose(m1));
        Matrix3x3 m5;
        m3.GetPolarDecomposition(&m4, &m5);
        AZ_TEST_ASSERT((m4 * m5).IsClose(m3));
        AZ_TEST_ASSERT(m4.IsClose(m1));
        AZ_TEST_ASSERT(m5.IsClose(m2, 0.01f));

        //orthogonalize
        m1 = Matrix3x3::CreateRotationX(AZ::DegToRad(30.0f)) * Matrix3x3::CreateScale(Vector3(2.0f, 3.0f, 4.0f));
        m1.SetElement(0, 1, 0.2f);
        m2 = m1.GetOrthogonalized();
        AZ_TEST_ASSERT(m2.GetRow(0).GetLength().IsClose(1.0f));
        AZ_TEST_ASSERT(m2.GetRow(1).GetLength().IsClose(1.0f));
        AZ_TEST_ASSERT(m2.GetRow(2).GetLength().IsClose(1.0f));
        AZ_TEST_ASSERT(m2.GetRow(0).Dot(m2.GetRow(1)).IsClose(0.0f));
        AZ_TEST_ASSERT(m2.GetRow(0).Dot(m2.GetRow(2)).IsClose(0.0f));
        AZ_TEST_ASSERT(m2.GetRow(1).Dot(m2.GetRow(2)).IsClose(0.0f));
        AZ_TEST_ASSERT(m2.GetRow(0).Cross(m2.GetRow(1)).IsClose(m2.GetRow(2)));
        AZ_TEST_ASSERT(m2.GetRow(1).Cross(m2.GetRow(2)).IsClose(m2.GetRow(0)));
        AZ_TEST_ASSERT(m2.GetRow(2).Cross(m2.GetRow(0)).IsClose(m2.GetRow(1)));
        m1.Orthogonalize();
        AZ_TEST_ASSERT(m1.IsClose(m2));

        //orthogonal
        m1 = Matrix3x3::CreateRotationX(AZ::DegToRad(30.0f));
        AZ_TEST_ASSERT(m1.IsOrthogonal(0.05f));
        m1.SetRow(1, m1.GetRow(1) * 2.0f);
        AZ_TEST_ASSERT(!m1.IsOrthogonal(0.05f));
        m1 = Matrix3x3::CreateRotationX(AZ::DegToRad(30.0f));
        m1.SetRow(1, Vector3(0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT(!m1.IsOrthogonal(0.05f));

        //IsClose
        m1 = Matrix3x3::CreateRotationX(DegToRad(30.0f));
        m2 = m1;
        AZ_TEST_ASSERT(m1.IsClose(m2));
        m2.SetElement(0, 0, 2.0f);
        AZ_TEST_ASSERT(!m1.IsClose(m2));
        m2 = m1;
        m2.SetElement(0, 2, 2.0f);
        AZ_TEST_ASSERT(!m1.IsClose(m2));

        //get diagonal
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        AZ_TEST_ASSERT(m1.GetDiagonal() == Vector3(1.0f, 5.0f, 9.0f));

        //determinant
        m1.SetRow(0, -1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, -9.0f);
        AZ_TEST_ASSERT(m1.GetDeterminant().IsClose(240.0f));

        //adjugate
        m1.SetRow(0, 1.0f, 2.0f, 3.0f);
        m1.SetRow(1, 4.0f, 5.0f, 6.0f);
        m1.SetRow(2, 7.0f, 8.0f, 9.0f);
        m2 = m1.GetAdjugate();
        AZ_TEST_ASSERT(m2.GetRow(0).IsClose(Vector3(-3.0f, 6.0f, -3.0f)));
        AZ_TEST_ASSERT(m2.GetRow(1).IsClose(Vector3(6.0f, -12.0f, 6.0f)));
        AZ_TEST_ASSERT(m2.GetRow(2).IsClose(Vector3(-3.0f, 6.0f, -3.0f)));
    }

    TEST(MATH_Quaternion, Test)
    {
        //constructors
        AZ::Quaternion q1(0.0f, 0.0f, 0.0f, 1.0f);
        AZ_TEST_ASSERT((q1.GetX() == 0.0f) && (q1.GetY() == 0.0f) && (q1.GetZ() == 0.0f) && (q1.GetW() == 1.0f));
        AZ::Quaternion q2(5.0f);
        AZ_TEST_ASSERT((q2.GetX() == 5.0f) && (q2.GetY() == 5.0f) && (q2.GetZ() == 5.0f) && (q2.GetW() == 5.0f));
        AZ::Quaternion q3(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT((q3.GetX() == 1.0f) && (q3.GetY() == 2.0f) && (q3.GetZ() == 3.0f) && (q3.GetW() == 4.0f));
        AZ::Quaternion q4 = AZ::Quaternion::CreateFromVector3AndValue(Vector3(1.0f, 2.0f, 3.0f), 4.0f);
        AZ_TEST_ASSERT((q4.GetX() == 1.0f) && (q4.GetY() == 2.0f) && (q4.GetZ() == 3.0f) && (q4.GetW() == 4.0f));
        float values[4] = { 10.0f, 20.0f, 30.0f, 40.0f };
        AZ::Quaternion q5 = AZ::Quaternion::CreateFromFloat4(values);
        AZ_TEST_ASSERT((q5.GetX() == 10.0f) && (q5.GetY() == 20.0f) && (q5.GetZ() == 30.0f) && (q5.GetW() == 40.0f));
        AZ::Quaternion q6 = AZ::Quaternion::CreateFromTransform(Transform::CreateRotationX(DegToRad(60.0f)));
        AZ_TEST_ASSERT(q6.IsClose(AZ::Quaternion(0.5f, 0.0f, 0.0f, 0.866f)));
        AZ::Quaternion q7 = AZ::Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateRotationX(DegToRad(120.0f)));
        AZ_TEST_ASSERT(q7.IsClose(AZ::Quaternion(0.866f, 0.0f, 0.0f, 0.5f)));
        AZ::Quaternion q8 = AZ::Quaternion::CreateFromMatrix4x4(Matrix4x4::CreateRotationX(DegToRad(-60.0f)));
        AZ_TEST_ASSERT(q8.IsClose(AZ::Quaternion(-0.5f, 0.0f, 0.0f, 0.866f)));
        AZ::Quaternion q9 = AZ::Quaternion::CreateFromVector3(Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT((q9.GetX() == 1.0f) && (q9.GetY() == 2.0f) && (q9.GetZ() == 3.0f) && (q9.GetW() == 0.0f));
        AZ::Quaternion q10 = AZ::Quaternion::CreateFromAxisAngle(Vector3::CreateAxisZ(), DegToRad(45.0f));
        AZ_TEST_ASSERT(q10.IsClose(AZ::Quaternion::CreateRotationZ(DegToRad(45.0f))));

        //Create functions
        AZ_TEST_ASSERT(AZ::Quaternion::CreateIdentity() == AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f));
        AZ_TEST_ASSERT(AZ::Quaternion::CreateZero() == AZ::Quaternion(0.0f));
        AZ_TEST_ASSERT(AZ::Quaternion::CreateRotationX(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.5f, 0.0f, 0.0f, 0.866f)));
        AZ_TEST_ASSERT(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.0f, 0.5f, 0.0f, 0.866f)));
        AZ_TEST_ASSERT(AZ::Quaternion::CreateRotationZ(DegToRad(60.0f)).IsClose(AZ::Quaternion(0.0f, 0.0f, 0.5f, 0.866f)));

        //test shortest arc
        Vector3 v1 = Vector3(1.0f, 2.0f, 3.0f).GetNormalized();
        Vector3 v2 = Vector3(-2.0f, 7.0f, -1.0f).GetNormalized();
        q3 = AZ::Quaternion::CreateShortestArc(v1, v2); //q3 should transform v1 into v2
        AZ_TEST_ASSERT(v2.IsClose(q3 * v1));
        q4 = AZ::Quaternion::CreateShortestArc(Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
        AZ_TEST_ASSERT((q4 * Vector3(0.0f, 0.0f, 1.0f)).IsClose(Vector3(0.0f, 0.0f, 1.0f)));  //perpendicular vector should be unaffected
        AZ_TEST_ASSERT((q4 * Vector3(0.0f, -1.0f, 0.0f)).IsClose(Vector3(1.0f, 0.0f, 0.0f))); //make sure we rotate the right direction, i.e. actually shortest arc

        //Get/Set
        q1.SetX(10.0f);
        AZ_TEST_ASSERT(q1.GetX() == 10.0f);
        q1.SetY(11.0f);
        AZ_TEST_ASSERT(q1.GetY() == 11.0f);
        q1.SetZ(12.0f);
        AZ_TEST_ASSERT(q1.GetZ() == 12.0f);
        q1.SetW(13.0f);
        AZ_TEST_ASSERT(q1.GetW() == 13.0f);
        q1.Set(15.0f);
        AZ_TEST_ASSERT(q1 == AZ::Quaternion(15.0f));
        q1.Set(2.0f, 3.0f, 4.0f, 5.0f);
        AZ_TEST_ASSERT(q1 == AZ::Quaternion(2.0f, 3.0f, 4.0f, 5.0f));
        q1.Set(Vector3(5.0f, 6.0f, 7.0f), 8.0f);
        AZ_TEST_ASSERT(q1 == AZ::Quaternion(5.0f, 6.0f, 7.0f, 8.0f));
        q1.Set(values);
        AZ_TEST_ASSERT((q1.GetX() == 10.0f) && (q1.GetY() == 20.0f) && (q1.GetZ() == 30.0f) && (q1.GetW() == 40.0f));

        //GetElement/SetElement
        q1.SetElement(0, 1.0f);
        q1.SetElement(1, 2.0f);
        q1.SetElement(2, 3.0f);
        q1.SetElement(3, 4.0f);
        AZ_TEST_ASSERT(q1.GetElement(0) == 1.0f);
        AZ_TEST_ASSERT(q1.GetElement(1) == 2.0f);
        AZ_TEST_ASSERT(q1.GetElement(2) == 3.0f);
        AZ_TEST_ASSERT(q1.GetElement(3) == 4.0f);

        //index operators
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(q1(0) == 1.0f);
        AZ_TEST_ASSERT(q1(1) == 2.0f);
        AZ_TEST_ASSERT(q1(2) == 3.0f);
        AZ_TEST_ASSERT(q1(3) == 4.0f);

        //IsIdentity
        q1.Set(0.0f, 0.0f, 0.0f, 1.0f);
        AZ_TEST_ASSERT(q1.IsIdentity());

        //conjugate
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(q1.GetConjugate() == AZ::Quaternion(-1.0f, -2.0f, -3.0f, 4.0f));

        //inverse
        q1 = AZ::Quaternion::CreateRotationX(DegToRad(25.0f)) * AZ::Quaternion::CreateRotationY(DegToRad(70.0f));
        AZ_TEST_ASSERT((q1 * q1.GetInverseFast()).IsClose(AZ::Quaternion::CreateIdentity()));
        q2 = q1;
        q2.InvertFast();
        AZ_TEST_ASSERT(q1.GetX() == -q2.GetX());
        AZ_TEST_ASSERT(q1.GetY() == -q2.GetY());
        AZ_TEST_ASSERT(q1.GetZ() == -q2.GetZ());
        AZ_TEST_ASSERT(q1.GetW() == q2.GetW());
        AZ_TEST_ASSERT((q1 * q2).IsClose(AZ::Quaternion::CreateIdentity()));

        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT((q1 * q1.GetInverseFull()).IsClose(AZ::Quaternion::CreateIdentity()));

        //dot product
        AZ_TEST_ASSERT_FLOAT_CLOSE(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).Dot(AZ::Quaternion(-1.0f, 5.0f, 3.0f, 2.0f)), 26.0f);

        //length
        AZ_TEST_ASSERT_FLOAT_CLOSE(AZ::Quaternion(-1.0f, 2.0f, 1.0f, 3.0f).GetLengthSq(), 15.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(AZ::Quaternion(-4.0f, 2.0f, 0.0f, 4.0f).GetLength(), 6.0f);

        //normalize
        AZ_TEST_ASSERT(AZ::Quaternion(0.0f, -4.0f, 2.0f, 4.0f).GetNormalized().IsClose(AZ::Quaternion(0.0f, -0.666f, 0.333f, 0.666f)));
        q1.Set(2.0f, 0.0f, 4.0f, -4.0f);
        q1.Normalize();
        AZ_TEST_ASSERT(q1.IsClose(AZ::Quaternion(0.333f, 0.0f, 0.666f, -0.666f)));
        q1.Set(2.0f, 0.0f, 4.0f, -4.0f);
        float length = q1.NormalizeWithLength();
        AZ_TEST_ASSERT_FLOAT_CLOSE(length, 6.0f);
        AZ_TEST_ASSERT(q1.IsClose(AZ::Quaternion(0.333f, 0.0f, 0.666f, -0.666f)));

        //interpolation
        AZ_TEST_ASSERT(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).Lerp(AZ::Quaternion(2.0f, 3.0f, 4.0f, 5.0f), 0.5f).IsClose(AZ::Quaternion(1.5f, 2.5f, 3.5f, 4.5f)));
        AZ_TEST_ASSERT(AZ::Quaternion::CreateRotationX(DegToRad(10.0f)).Slerp(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)), 0.5f).IsClose(AZ::Quaternion(0.045f, 0.259f, 0.0f, 0.965f)));
        AZ_TEST_ASSERT(AZ::Quaternion::CreateRotationX(DegToRad(10.0f)).Squad(AZ::Quaternion::CreateRotationY(DegToRad(60.0f)), AZ::Quaternion::CreateRotationZ(DegToRad(35.0f)), AZ::Quaternion::CreateRotationX(DegToRad(80.0f)), 0.5f).IsClose(AZ::Quaternion(0.2f, 0.132f, 0.083f, 0.967f)));

        //close
        AZ_TEST_ASSERT(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));
        AZ_TEST_ASSERT(!AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f)));
        AZ_TEST_ASSERT(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f).IsClose(AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.4f), 0.5f));

        //operators
        AZ_TEST_ASSERT((-AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)) == AZ::Quaternion(-1.0f, -2.0f, -3.0f, -4.0f));
        AZ_TEST_ASSERT((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) + AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(3.0f, 5.0f, 8.0f, 3.0f)));
        AZ_TEST_ASSERT((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) - AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(-1.0f, -1.0f, -2.0f, 5.0f)));
        AZ_TEST_ASSERT((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) * AZ::Quaternion(2.0f, 3.0f, 5.0f, -1.0f)).IsClose(AZ::Quaternion(8.0f, 11.0f, 16.0f, -27.0f)));
        AZ_TEST_ASSERT((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) * 2.0f).IsClose(AZ::Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
        AZ_TEST_ASSERT((2.0f * AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)).IsClose(AZ::Quaternion(2.0f, 4.0f, 6.0f, 8.0f)));
        AZ_TEST_ASSERT((AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f) / 2.0f).IsClose(AZ::Quaternion(0.5f, 1.0f, 1.5f, 2.0f)));
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        q1 += AZ::Quaternion(5.0f, 6.0f, 7.0f, 8.0f);
        AZ_TEST_ASSERT(q1.IsClose(AZ::Quaternion(6.0f, 8.0f, 10.0f, 12.0f)));
        q1 -= AZ::Quaternion(3.0f, -1.0f, 5.0f, 7.0f);
        AZ_TEST_ASSERT(q1.IsClose(AZ::Quaternion(3.0f, 9.0f, 5.0f, 5.0f)));
        q1.Set(1.0f, 2.0f, 3.0f, 4.0f);
        q1 *= AZ:: Quaternion(2.0f, 3.0f, 5.0f, -1.0f);
        AZ_TEST_ASSERT(q1.IsClose(AZ::Quaternion(8.0f, 11.0f, 16.0f, -27.0f)));
        q1 *= 2.0f;
        AZ_TEST_ASSERT(q1.IsClose(AZ::Quaternion(16.0f, 22.0f, 32.0f, -54.0f)));
        q1 /= 4.0f;
        AZ_TEST_ASSERT(q1.IsClose(AZ::Quaternion(4.0f, 5.5f, 8.0f, -13.5f)));

        //operator==
        q3.Set(1.0f, 2.0f, 3.0f, 4.0f);
        AZ_TEST_ASSERT(q3 == AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0));
        AZ_TEST_ASSERT(!(q3 == AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f)));
        AZ_TEST_ASSERT(q3 != AZ::Quaternion(1.0f, 2.0f, 3.0f, 5.0f));
        AZ_TEST_ASSERT(!(q3 != AZ::Quaternion(1.0f, 2.0f, 3.0f, 4.0f)));

        //vector transform
        AZ_TEST_ASSERT((AZ::Quaternion::CreateRotationX(DegToRad(45.0f)) * Vector3(4.0f, 1.0f, 0.0f)).IsClose(Vector3(4.0f, 0.707f, 0.707f)));

        //GetImaginary
        q1.Set(21.0f, 22.0f, 23.0f, 24.0f);
        AZ_TEST_ASSERT(q1.GetImaginary() == Vector3(21.0f, 22.0f, 23.0f));

        //GetAngle
        q1 = AZ::Quaternion::CreateRotationX(DegToRad(35.0f));
        AZ_TEST_ASSERT(q1.GetAngle().IsClose(DegToRad(35.0f)));

        //make sure that our transformations concatenate in the correct order
        q1 = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));
        q2 = AZ::Quaternion::CreateRotationX(DegToRad(90.0f));
        Vector3 v = (q2 * q1) * Vector3(1.0f, 0.0f, 0.0f);
        AZ_TEST_ASSERT(v.IsClose(Vector3(0.0f, 0.0f, 1.0f)));
    }
    
    TEST(MATH_Aabb, Test)
    {
        //CreateNull
        //IsValid
        Aabb aabb = Aabb::CreateNull();
        AZ_TEST_ASSERT(!aabb.IsValid());
        AZ_TEST_ASSERT(aabb.GetMin().IsGreaterThan(aabb.GetMax()));

        //CreateFromPoint
        aabb = Aabb::CreateFromPoint(Vector3(0.0f));
        AZ_TEST_ASSERT(aabb.IsValid());
        AZ_TEST_ASSERT(aabb.IsFinite());
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(aabb.GetMax()));

        //CreateFromMinMax
        aabb = Aabb::CreateFromMinMax(Vector3(0.0f), Vector3(1.0f));
        AZ_TEST_ASSERT(aabb.IsValid());
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(0.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(1.0f)));

        //CreateCenterHalfExtents
        aabb = Aabb::CreateCenterHalfExtents(Vector3(1.0f), Vector3(1.0f));
        AZ_TEST_ASSERT(aabb.IsValid());
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(0.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(2.0f)));
        AZ_TEST_ASSERT(aabb.GetCenter().IsClose(Vector3(1.0f)));

        //CreateCenterRadius
        //GetExtents
        aabb = Aabb::CreateCenterRadius(Vector3(4.0f), 2.0f);
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(2.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(6.0f)));
        AZ_TEST_ASSERT(aabb.GetExtents().IsClose(Vector3(4.0f)));

        //CreatePoints
        const int numPoints = 3;
        Vector3 points[numPoints] =
        {
            Vector3(10.0f, 0.0f, -10.0f),
            Vector3(-10.0f, 10.0f, 0.0f),
            Vector3(0.0f, -10.0f, 10.0f)
        };
        aabb = Aabb::CreatePoints(points, numPoints);
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-10.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(10.0f)));

        //CreateFromObb
        Vector3 position(1.0f, 1.0f, 1.0f);
        Vector3 axisX(0.707f, 0.707f, 0.0f);
        Vector3 axisY(0.0f, 0.707f, 0.707f);
        Vector3 axisZ(0.707f, 0.0f, 0.707f);
        float halfLength = 1.0f;
        Obb obb = Obb::CreateFromPositionAndAxes(position, axisX, halfLength, axisY, halfLength, axisZ, halfLength);
        aabb = Aabb::CreateFromObb(obb);
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-0.414f, -0.414f, -0.414f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(2.414f, 2.414f, 2.414f)));
        AZ_TEST_ASSERT(aabb.GetCenter().IsClose(Vector3(1.0f, 1.0f, 1.0f)));

        //Set(min, max)
        //GetMin
        //GetMax
        aabb.Set(Vector3(1.0f), Vector3(4.0f));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(1.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(4.0f)));

        //SetMin
        aabb.SetMin(Vector3(0.0f));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(0.0f)));
        AZ_TEST_ASSERT(aabb.GetCenter().IsClose(Vector3(2.0f)));

        //SetMax
        aabb.SetMax(Vector3(6.0f));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(6.0f)));
        AZ_TEST_ASSERT(aabb.GetCenter().IsClose(Vector3(3.0f)));

        //GetWidth
        //GetHeight
        //GetDepth
        aabb.Set(Vector3(0.0f), Vector3(1.0f, 2.0f, 3.0f));
        AZ_TEST_ASSERT(aabb.GetWidth().IsClose(1.0f));
        AZ_TEST_ASSERT(aabb.GetHeight().IsClose(2.0f));
        AZ_TEST_ASSERT(aabb.GetDepth().IsClose(3.0f));

        //GetAsSphere
        aabb.Set(Vector3(-2.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f));
        Vector3 center;
        VectorFloat radius;
        aabb.GetAsSphere(center, radius);
        AZ_TEST_ASSERT(center.IsClose(Vector3(0.0f)));
        AZ_TEST_ASSERT(radius.IsClose(2.0f));

        //Contains(Vector3)
        aabb.Set(Vector3(-2.0f), Vector3(2.0f));
        AZ_TEST_ASSERT(aabb.Contains(Vector3(1.0f)));

        //Contains(Aabb)
        aabb.Set(Vector3(0.0f), Vector3(3.0f));
        Aabb aabb2 = Aabb::CreateFromMinMax(Vector3(1.0f), Vector3(2.0f));
        AZ_TEST_ASSERT(aabb.Contains(aabb2));
        AZ_TEST_ASSERT(!aabb2.Contains(aabb));

        //Overlaps
        aabb.Set(Vector3(0.0f), Vector3(2.0f));
        aabb2.Set(Vector3(1.0f), Vector3(3.0f));
        AZ_TEST_ASSERT(aabb.Overlaps(aabb2));
        aabb2.Set(Vector3(5.0f), Vector3(6.0f));
        AZ_TEST_ASSERT(!aabb.Overlaps(aabb2));

        //Expand
        aabb.Set(Vector3(0.0f), Vector3(2.0f));
        aabb.Expand(Vector3(1.0f));
        AZ_TEST_ASSERT(aabb.GetCenter().IsClose(Vector3(1.0f)));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-1.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(3.0f)));

        //GetExpanded
        aabb.Set(Vector3(-1.0f), Vector3(1.0f));
        aabb = aabb.GetExpanded(Vector3(9.0f));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-10.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(10.0f)));

        //AddPoint
        aabb.Set(Vector3(-1.0f), Vector3(1.0f));
        aabb.AddPoint(Vector3(1.0f));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-1.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(1.0f)));
        aabb.AddPoint(Vector3(2.0f));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-1.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(2.0f)));

        //AddAabb
        aabb.Set(Vector3(0.0f), Vector3(2.0f));
        aabb2.Set(Vector3(-2.0f), Vector3(0.0f));
        aabb.AddAabb(aabb2);
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-2.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(2.0f)));

        //GetDistance
        aabb.Set(Vector3(-1.0f), Vector3(1.0f));
        AZ_TEST_ASSERT(aabb.GetDistance(Vector3(2.0f, 0.0f, 0.0f)).IsClose(1.0f));
        // make sure a point inside the box returns zero, even if that point isn't the center.
        AZ_TEST_ASSERT(aabb.GetDistance(Vector3(0.5f, 0.0f, 0.0f)).IsClose(0.0f));

        //GetClamped
        aabb.Set(Vector3(0.0f), Vector3(2.0f));
        aabb2.Set(Vector3(1.0f), Vector3(4.0f));
        aabb = aabb.GetClamped(aabb2);
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(1.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(2.0f)));

        //Clamp
        aabb.Set(Vector3(0.0f), Vector3(2.0f));
        aabb2.Set(Vector3(-2.0f), Vector3(1.0f));
        aabb.Clamp(aabb2);
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(0.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(1.0f)));

        //SetNull
        aabb.SetNull();
        AZ_TEST_ASSERT(!aabb.IsValid());

        //Translate
        aabb.Set(Vector3(-1.0f), Vector3(1.0f));
        aabb.Translate(Vector3(2.0f));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(1.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(3.0f)));

        //GetTranslated
        aabb.Set(Vector3(1.0f), Vector3(3.0f));
        aabb = aabb.GetTranslated(Vector3(-2.0f));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(Vector3(-1.0f)));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(Vector3(1.0f)));

        //GetSurfaceArea
        aabb.Set(Vector3(0.0f), Vector3(1.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(aabb.GetSurfaceArea(), 6.0f);

        //IsFinite
        const float infinity = std::numeric_limits<float>::infinity();
        const Vector3 infiniteV3 = Vector3(infinity);
        aabb.Set(Vector3(0), infiniteV3);
        // A bounding box is only invalid if the min is greater than the max.
        // A bounding box with an infinite min or max is valid as long as this is true.
        AZ_TEST_ASSERT(aabb.IsValid());
        AZ_TEST_ASSERT(!aabb.IsFinite());
    }

    // Check if both aabb transform functions give the same result
    TEST(MATH_AabbTransform, CompareTest)
    {
        // create aabb
        Vector3 min(-100.0f, 50.0f, 0.0f);
        Vector3 max(120.0f, 300.0f, 50.0f);
        Aabb    aabb = Aabb::CreateFromMinMax(min, max);

        // make the transformation matrix
        Transform tm = Transform::CreateScale(Vector3(0.4f, 1.0f, 1.2f)) * Transform::CreateRotationX(1.0f);
        tm.SetTranslation(100.0f, 0.0f, -50.0f);

        // transform
        Obb     obb = aabb.GetTransformedObb(tm);
        Aabb    transAabb = aabb.GetTransformedAabb(tm);
        Aabb    transAabb2 = Aabb::CreateFromObb(obb);
        aabb.ApplyTransform(tm);

        // compare the transformations
        AZ_TEST_ASSERT(transAabb.GetMin().IsClose(transAabb2.GetMin()));
        AZ_TEST_ASSERT(transAabb.GetMax().IsClose(transAabb2.GetMax()));
        AZ_TEST_ASSERT(aabb.GetMin().IsClose(transAabb.GetMin()));
        AZ_TEST_ASSERT(aabb.GetMax().IsClose(transAabb.GetMax()));
    }

    TEST(MATH_Obb, Test)
    {
        //CreateFromPositionAndAxes
        //GetPosition
        //GetAxisX
        //GetAxisY
        //GetAxisZ
        //GetAxis
        //GetHalfLengthX
        //GetHalfLengthY
        //GetHalfLengthZ
        //GetHalfLength
        Vector3 position(1.0f, 2.0f, 3.0f);
        Vector3 axisX(0.707f, 0.707f, 0.0f);
        Vector3 axisY(0.0f, 0.707f, 0.707f);
        Vector3 axisZ(0.707f, 0.0f, 0.707f);
        float halfLength = 0.5f;
        Obb obb = Obb::CreateFromPositionAndAxes(position, axisX, halfLength, axisY, halfLength, axisZ, halfLength);
        AZ_TEST_ASSERT(obb.GetPosition().IsClose(position));
        AZ_TEST_ASSERT(obb.GetAxisX().IsClose(axisX));
        AZ_TEST_ASSERT(obb.GetAxisY().IsClose(axisY));
        AZ_TEST_ASSERT(obb.GetAxisZ().IsClose(axisZ));
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLengthX(), halfLength);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLengthY(), halfLength);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLengthZ(), halfLength);
        AZ_TEST_ASSERT(obb.GetAxis(0).IsClose(axisX));
        AZ_TEST_ASSERT(obb.GetAxis(1).IsClose(axisY));
        AZ_TEST_ASSERT(obb.GetAxis(2).IsClose(axisZ));
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLength(0), halfLength);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLength(1), halfLength);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLength(2), halfLength);

        //CreateFromAabb
        Vector3 min(-100.0f, 50.0f, 0.0f);
        Vector3 max(120.0f, 300.0f, 50.0f);
        Aabb aabb = Aabb::CreateFromMinMax(min, max);
        obb = Obb::CreateFromAabb(aabb);
        AZ_TEST_ASSERT(obb.GetPosition().IsClose(Vector3(10.0f, 175.0f, 25.0f)));
        AZ_TEST_ASSERT(obb.GetAxisX().IsClose(Vector3(1.0f, 0.0f, 0.0f)));
        AZ_TEST_ASSERT(obb.GetAxisY().IsClose(Vector3(0.0f, 1.0f, 0.0f)));
        AZ_TEST_ASSERT(obb.GetAxisZ().IsClose(Vector3(0.0f, 0.0f, 1.0f)));

        // Transform * Obb
        obb = Obb::CreateFromPositionAndAxes(position, axisX, halfLength, axisY, halfLength, axisZ, halfLength);
        Transform transform = Transform::CreateRotationY(DegToRad(90.0f));
        obb = transform * obb;
        AZ_TEST_ASSERT(obb.GetPosition().IsClose(Vector3(3.0f, 2.0f, -1.0f)));
        AZ_TEST_ASSERT(obb.GetAxisX().IsClose(Vector3(0.0f, 0.707f, -0.707f)));
        AZ_TEST_ASSERT(obb.GetAxisY().IsClose(Vector3(0.707f, 0.707f, 0.0f)));
        AZ_TEST_ASSERT(obb.GetAxisZ().IsClose(Vector3(0.707f, 0.0f, -0.707f)));

        //SetPosition
        //SetAxisX
        //SetAxisY
        //SetAxisZ
        //SetAxis
        //SetHalfLengthX
        //SetHalfLengthY
        //SetHalfLengthZ
        //SetHalfLength
        obb.SetPosition(position);
        AZ_TEST_ASSERT(obb.GetPosition().IsClose(position));
        obb.SetAxisX(axisX);
        AZ_TEST_ASSERT(obb.GetAxisX().IsClose(axisX));
        obb.SetAxisY(axisY);
        AZ_TEST_ASSERT(obb.GetAxisY().IsClose(axisY));
        obb.SetAxisZ(axisZ);
        AZ_TEST_ASSERT(obb.GetAxisZ().IsClose(axisZ));
        obb.SetAxis(1, axisX);
        AZ_TEST_ASSERT(obb.GetAxis(1).IsClose(axisX));
        obb.SetHalfLengthX(2.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLengthX(), 2.0f);
        obb.SetHalfLengthY(3.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLengthY(), 3.0f);
        obb.SetHalfLengthZ(4.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLengthZ(), 4.0f);
        obb.SetHalfLength(2, 5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(obb.GetHalfLength(2), 5.0f);

        //IsFinite
        obb = Obb::CreateFromPositionAndAxes(position, axisX, halfLength, axisY, halfLength, axisZ, halfLength);
        AZ_TEST_ASSERT(obb.IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        const Vector3 infiniteV3 = Vector3(infinity);
        // Test to make sure that setting properties of the bounding box
        // properly mark it as infinite, and when reset it becomes finite again.
        obb.SetPosition(infiniteV3);
        AZ_TEST_ASSERT(!obb.IsFinite());
        obb.SetPosition(position);
        AZ_TEST_ASSERT(obb.IsFinite());
        obb.SetAxisX(infiniteV3);
        AZ_TEST_ASSERT(!obb.IsFinite());
        obb.SetAxisX(axisX);
        AZ_TEST_ASSERT(obb.IsFinite());
        obb.SetHalfLengthY(infinity);
        AZ_TEST_ASSERT(!obb.IsFinite());
    }

    TEST(MATH_Handedness, Test)
    {
        //test to make sure our rotations follow the right hand rule,
        // a positive rotation around z should transform the x axis to the y axis
        Matrix4x4 matrix = Matrix4x4::CreateRotationZ(DegToRad(90.0f));
        Vector3 v = matrix * Vector3(1.0f, 0.0f, 0.0f);
        AZ_TEST_ASSERT(v.IsClose(Vector3(0.0f, 1.0f, 0.0f)));

        AZ::Quaternion quat = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));
        v = quat * Vector3(1.0f, 0.0f, 0.0f);
        AZ_TEST_ASSERT(v.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
    }

    TEST(MATH_MatrixQuaternion, ConversionTest)
    {
        Matrix4x4 rotMatrix = Matrix4x4::CreateRotationZ(DegToRad(90.0f));
        AZ::Quaternion rotQuat = AZ::Quaternion::CreateRotationZ(DegToRad(90.0f));

        AZ::Quaternion q = AZ::Quaternion::CreateFromMatrix4x4(rotMatrix);
        AZ_TEST_ASSERT(q.IsClose(rotQuat));

        Matrix4x4 m = Matrix4x4::CreateFromQuaternion(rotQuat);
        AZ_TEST_ASSERT(m.IsClose(rotMatrix));
    }

    TEST(MATH_Plane, Test)
    {
        Plane pl = Plane::CreateFromNormalAndDistance(Vector3(1.0f, 0.0f, 0.0f), -100.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetDistance(), -100.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetX(), 1.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetY(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetZ(), 0.0f);

        pl = Plane::CreateFromNormalAndPoint(Vector3(0.0f, 1.0f, 0.0f), Vector3(10, 10, 10));
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetDistance(), -10.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetX(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetY(), 1.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetZ(), 0.0f);

        pl = Plane::CreateFromCoefficients(0.0f, -1.0f, 0.0f, -5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetDistance(), -5.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetX(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetY(), -1.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetZ(), 0.0f);

        pl.Set(12.0f, 13.0f, 14.0f, 15.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetDistance(), 15.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetX(), 12.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetY(), 13.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetZ(), 14.0f);

        pl.Set(Vector3(22.0f, 23.0f, 24.0f), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetDistance(), 25.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetX(), 22.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetY(), 23.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetZ(), 24.0f);

        pl.Set(Vector4(32.0f, 33.0f, 34.0f, 35.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetDistance(), 35.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetX(), 32.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetY(), 33.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetZ(), 34.0f);

        pl.SetNormal(Vector3(0.0f, 0.0f, 1.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetX(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetY(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetNormal().GetZ(), 1.0f);

        pl.SetDistance(55.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetDistance(), 55.0f);

        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetW(), 55.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetX(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetY(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetZ(), 1.0f);

        Transform tm = Transform::CreateRotationY(DegToRad(90.0f));
        pl = pl.GetTransform(tm);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetW(), 55.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetX(), 1.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetY(), 0.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetZ(), 0.0f);

        float dist = pl.GetPointDist(Vector3(10.0f, 0.0f, 0.0f));
        AZ_TEST_ASSERT_FLOAT_CLOSE(dist, 65.0f);  // 55 + 10

        tm = Transform::CreateRotationZ(DegToRad(45.0f));
        pl.ApplyTransform(tm);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetW(), 55.0f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetX(), 0.707f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetY(), 0.707f);
        AZ_TEST_ASSERT_FLOAT_CLOSE(pl.GetPlaneEquationCoefficients().GetZ(), 0.0f);


        pl.SetNormal(Vector3(0.0f, 0.0f, 1.0f));
        Vector3 v1 = pl.GetProjected(Vector3(10.0f, 15.0f, 20.0f));
        AZ_TEST_ASSERT(v1 == Vector3(10.0f, 15.0f, 0.0f));

        pl.Set(0.0f, 0.0f, 1.0f, 10.0f);
        bool hit = pl.CastRay(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), v1);
        AZ_TEST_ASSERT(hit == true);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 0.0f, -10.0f)));

        pl.Set(0.0f, 1.0f, 0.0f, 10.0f);
        VectorFloat time;
        hit = pl.CastRay(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), time);
        AZ_TEST_ASSERT(hit == true);
        AZ_TEST_ASSERT_FLOAT_CLOSE(time, 10.999f);

        pl.Set(1.0f, 0.0f, 0.0f, 5.0f);
        hit = pl.CastRay(Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), time);
        AZ_TEST_ASSERT(hit == false);

        pl.Set(1.0f, 0.0f, 0.0f, 0.0f);
        hit = pl.IntersectSegment(Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), v1);
        AZ_TEST_ASSERT(hit == true);
        AZ_TEST_ASSERT(v1.IsClose(Vector3(0.0f, 0.0f, 0.0f)));

        pl.Set(0.0f, 1.0f, 0.0f, 0.0f);
        hit = pl.IntersectSegment(Vector3(0.0f, -10.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), time);
        AZ_TEST_ASSERT(hit == true);
        AZ_TEST_ASSERT_FLOAT_CLOSE(time, 0.5f);

        pl.Set(0.0f, 1.0f, 0.0f, 20.0f);
        hit = pl.IntersectSegment(Vector3(-1.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), time);
        AZ_TEST_ASSERT(hit == false);

        pl.Set(1.0f, 0.0f, 0.0f, 0.0f);
        AZ_TEST_ASSERT(pl.IsFinite());
        const float infinity = std::numeric_limits<float>::infinity();
        pl.Set(infinity, infinity, infinity, infinity);
        AZ_TEST_ASSERT(!pl.IsFinite());
    }

    class MATH_SfmtTest
        : public AllocatorsFixture
    {
        static const int BLOCK_SIZE  = 100000;
        static const int BLOCK_SIZE64 = 50000;
        static const int COUNT = 1000;

#if defined(AZ_SIMD)
        SimdVectorType* array1;
        SimdVectorType* array2;
#else
        AZ::u64* array1;
        AZ::u64* array2;
#endif

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
#if defined(AZ_SIMD)
            array1 = (SimdVectorType*)azmalloc(sizeof(SimdVectorType) * (BLOCK_SIZE / 4), AZStd::alignment_of<SimdVectorType>::value);
            array2 = (SimdVectorType*)azmalloc(sizeof(SimdVectorType) * (10000 / 4), AZStd::alignment_of<SimdVectorType>::value);
#else
            array1 = (AZ::u64*)azmalloc(sizeof(AZ::u64) * 2 * (BLOCK_SIZE / 4), AZStd::alignment_of<AZ::u64>::value);
            array2 = (AZ::u64*)azmalloc(sizeof(AZ::u64) * 2 * (10000 / 4), AZStd::alignment_of<AZ::u64>::value);
#endif
        }
        void TearDown() override
        {
            azfree(array1);
            azfree(array2);

            AllocatorsFixture::TearDown();
        }

        void check32()
        {
            int i;
            AZ::u32* array32 = (AZ::u32*)array1;
            AZ::u32* array32_2 = (AZ::u32*)array2;
            AZ::u32 ini[] = {0x1234, 0x5678, 0x9abc, 0xdef0};

            Sfmt g;
            EXPECT_LT(g.GetMinArray32Size(), 10000);
            g.FillArray32(array32, 10000);
            g.FillArray32(array32_2, 10000);
            g.Seed();
            for (i = 0; i < 10000; i++)
            {
                EXPECT_NE(array32[i], g.Rand32());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_NE(array32_2[i], g.Rand32());
            }
            g.Seed(ini, 4);
            g.FillArray32(array32, 10000);
            g.FillArray32(array32_2, 10000);
            g.Seed(ini, 4);
            for (i = 0; i < 10000; i++)
            {
                EXPECT_EQ(array32[i], g.Rand32());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_EQ(array32_2[i], g.Rand32());
            }
        }

        void check64()
        {
            int i;
            AZ::u64* array64 = (AZ::u64*)array1;
            AZ::u64* array64_2 = (AZ::u64*)array2;
            AZ::u32 ini[] = {5, 4, 3, 2, 1};

            Sfmt g;
            EXPECT_LT(g.GetMinArray64Size(), 5000);
            g.FillArray64(array64, 5000);
            g.FillArray64(array64_2, 5000);
            g.Seed();
            for (i = 0; i < 5000; i++)
            {
                EXPECT_NE(array64[i], g.Rand64());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_NE(array64_2[i], g.Rand64());
            }
            g.Seed(ini, 5);
            g.FillArray64(array64, 5000);
            g.FillArray64(array64_2, 5000);
            g.Seed(ini, 5);
            for (i = 0; i < 5000; i++)
            {
                EXPECT_EQ(array64[i], g.Rand64());
            }
            for (i = 0; i < 700; i++)
            {
                EXPECT_EQ(array64_2[i], g.Rand64());
            }
        }
    };

    TEST_F(MATH_SfmtTest, Test32Bit)
    {
        check32();
    }

    TEST_F(MATH_SfmtTest, Test64Bit)
    {
        check64();
    }

    TEST_F(MATH_SfmtTest, TestParallel32)
    {
        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
        {
            for (int i = 0; i < 10000; ++i)
            {
                sfmt.Rand32();
            }
        };
        AZStd::thread threads[8];
        for (int threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            threads[threadIdx] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    TEST_F(MATH_SfmtTest, TestParallel64)
    {
        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
        {
            for (int i = 0; i < 10000; ++i)
            {
                sfmt.Rand64();
            }
        };
        AZStd::thread threads[8];
        for (int threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            threads[threadIdx] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    TEST_F(MATH_SfmtTest, TestParallelInterleaved)
    {
        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
        {
            for (int i = 0; i < 10000; ++i)
            {
                AZ::u64 roll = sfmt.Rand64();
                if (roll % 2 == 0)
                {
                    sfmt.Rand32();
                }
                else
                {
                    sfmt.Rand64();
                }
            }
        };
        AZStd::thread threads[8];
        for (int threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
        {
            threads[threadIdx] = AZStd::thread(threadFunc);
        }

        for (auto& thread : threads)
        {
            thread.join();
        }
    }

    class MATH_UuidTest
        : public AllocatorsFixture
    {
        static const int numUuid  = 2000;
        Uuid* m_array;
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();

            m_array = (Uuid*)azmalloc(sizeof(Uuid) * numUuid, AZStd::alignment_of<Uuid>::value);
        }
        void TearDown() override
        {
            azfree(m_array);

            AllocatorsFixture::TearDown();
        }
        void run()
        {
            Uuid defId;
            defId.data[0] = 0xb5;
            defId.data[1] = 0x70;
            defId.data[2] = 0x0f;
            defId.data[3] = 0x2e;
            defId.data[4] = 0x66;
            defId.data[5] = 0x1b;
            defId.data[6] = 0x4a;
            defId.data[7] = 0xc0;
            defId.data[8] = 0x93;
            defId.data[9] = 0x35;
            defId.data[10] = 0x81;
            defId.data[11] = 0x7c;
            defId.data[12] = 0xb4;
            defId.data[13] = 0xc0;
            defId.data[14] = 0x9c;
            defId.data[15] = 0xcb;

            // null
            Uuid id = Uuid::CreateNull();
            AZ_TEST_ASSERT(id.IsNull());

            const char idStr1[] = "{B5700F2E-661B-4AC0-9335-817CB4C09CCB}";
            const char idStr2[] = "{B5700F2E661B4AC09335817CB4C09CCB}";
            const char idStr3[] = "B5700F2E-661B-4AC0-9335-817CB4C09CCB";
            const char idStr4[] = "B5700F2E661B4AC09335817CB4C09CCB";

            // create from string
            id = Uuid::CreateString(idStr1);
            AZ_TEST_ASSERT(id == defId);
            id = Uuid::CreateString(idStr2);
            AZ_TEST_ASSERT(id == defId);
            id = Uuid::CreateString(idStr3);
            AZ_TEST_ASSERT(id == defId);
            id = Uuid::CreateString(idStr4);
            AZ_TEST_ASSERT(id == defId);

            // variant
            AZ_TEST_ASSERT(id.GetVariant() == Uuid::VAR_RFC_4122);
            // version
            AZ_TEST_ASSERT(id.GetVersion() == Uuid::VER_RANDOM);

            // tostring
            char buffer[39];
            id = Uuid::CreateString(idStr1);
            AZ_TEST_ASSERT(id.ToString(buffer, 39, true, true) == 39);
            AZ_TEST_ASSERT(strcmp(buffer, idStr1) == 0);
            AZ_TEST_ASSERT(id.ToString(buffer, 35, true, false) == 35);
            AZ_TEST_ASSERT(strcmp(buffer, idStr2) == 0);
            AZ_TEST_ASSERT(id.ToString(buffer, 37, false, true) == 37);
            AZ_TEST_ASSERT(strcmp(buffer, idStr3) == 0);
            AZ_TEST_ASSERT(id.ToString(buffer, 33, false, false) == 33);
            AZ_TEST_ASSERT(strcmp(buffer, idStr4) == 0);

            AZ_TEST_ASSERT(id.ToString<AZStd::string>() == AZStd::string(idStr1));
            AZ_TEST_ASSERT(id.ToString<AZStd::string>(true, false) == AZStd::string(idStr2));
            AZ_TEST_ASSERT(id.ToString<AZStd::string>(false, true) == AZStd::string(idStr3));
            AZ_TEST_ASSERT(id.ToString<AZStd::string>(false, false) == AZStd::string(idStr4));

            AZStd::string str1;
            id.ToString(str1);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr1));
            id.ToString(str1, true, false);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr2));
            id.ToString(str1, false, true);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr3));
            id.ToString(str1, false, false);
            AZ_TEST_ASSERT(str1 == AZStd::string(idStr4));

            // operators
            Uuid idBigger("C5700F2E661B4ac09335817CB4C09CCB");
            AZ_TEST_ASSERT(id < idBigger);
            AZ_TEST_ASSERT(id != idBigger);
            AZ_TEST_ASSERT(idBigger > id);

            // hash
            AZStd::hash<AZ::Uuid> hash;
            size_t hashVal = hash(id);
            AZ_TEST_ASSERT(hashVal != 0);

            // test the hashing and equal function in a unordered container
            typedef AZStd::unordered_set<AZ::Uuid> UuidSetType;
            UuidSetType uuidSet;
            uuidSet.insert(id);
            AZ_TEST_ASSERT(uuidSet.find(id) != uuidSet.end());

            // check uniqueness (very quick and basic)
            for (int i = 0; i < numUuid; ++i)
            {
                m_array[i] = Uuid::Create();
            }

            for (int i = 0; i < numUuid; ++i)
            {
                Uuid uniqueToTest = Uuid::Create();
                for (int j = 0; j < numUuid; ++j)
                {
                    AZ_TEST_ASSERT(m_array[j] != uniqueToTest);
                }
            }

            // test the name function
            Uuid uuidName = Uuid::CreateName("BlaBla");
            // check variant
            AZ_TEST_ASSERT(uuidName.GetVariant() == Uuid::VAR_RFC_4122);
            // check version
            AZ_TEST_ASSERT(uuidName.GetVersion() == Uuid::VER_NAME_SHA1);
            // check id
            AZ_TEST_ASSERT(uuidName == Uuid::CreateName("BlaBla"));
        }
    };

    TEST_F(MATH_UuidTest, Test)
    {
        run();
    }
}
