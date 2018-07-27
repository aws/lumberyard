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
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/IntersectSegment.h>

#include <AzCore/Math/Sfmt.h>
#include <AzCore/Math/Uuid.h>

#include <AzCore/std/containers/unordered_set.h>

// output stream PrintTo() function for VectorFloat so GTest can print it.
namespace AZ {

    void PrintTo(const VectorFloat& vf, ::std::ostream* os)
    {
        *os << static_cast<float>(vf);
    }
}


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

        AZ_TEST_ASSERT(Vector2::CreateFromAngle() == Vector2(0.0f, 1.0f));
        // Simd SinCos is really innaccurate, so 0.01 is the best we can hope for.
        AZ_TEST_ASSERT(Vector2::CreateFromAngle(0.78539816339f).IsClose(Vector2(0.7071067811f, 0.7071067811f), 0.01f));
        AZ_TEST_ASSERT(Vector2::CreateFromAngle(4.0f).IsClose(Vector2(-0.7568024953f, -0.6536436208f), 0.01f));
        AZ_TEST_ASSERT(Vector2::CreateFromAngle(-1.0f).IsClose(Vector2(-0.8414709848f, 0.5403023058f), 0.01f));

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

    TEST(MATH_Intersection, ClosestSegmentSegment)
    {
        // line2 right and above line1 (no overlap, parallel)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(7.0f, 0.0f, 4.0f);
            Vector3 line2End(10.0f, 0.0f, 4.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 5.0f);
            EXPECT_TRUE(line1Proportion == 1.0f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line2 halfway over the top of the line1 (overlap, parallel)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 0.0f, 3.0f);
            Vector3 line2End(6.0f, 0.0f, 3.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 3.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line2 over the top of the line1 (inside, parallel)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(8.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 0.0f, 3.0f);
            Vector3 line2End(6.0f, 0.0f, 3.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 3.0f);
            EXPECT_TRUE(line1Proportion == 0.25f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line2 over the top of the line1 (overlap, skew (cross))
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(8.0f, 0.0f, 0.0f);
            Vector3 line2Start(4.0f, 4.0f, 4.0f);
            Vector3 line2End(4.0f, -4.0f, 4.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 4.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.5f);
        }

        // line2 flat diagonal to line1 (no overlap, skew)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 4.0f, 0.0f);
            Vector3 line2Start(10.0f, 0.0f, 0.0f);
            Vector3 line2End(6.0f, 4.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(line1Proportion == 1.0f);
            EXPECT_TRUE(line2Proportion == 1.0f);
        }

        // line2 perpendicular to line1 (skew, no overlap)
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 1.0f, 0.0f);
            Vector3 line2End(2.0f, 4.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 1.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // line 1 degenerates to point
        {
            Vector3 line1Start(4.0f, 2.0f, 0.0f);
            Vector3 line1End(4.0f, 2.0f, 0.0f);
            Vector3 line2Start(2.0f, 0.0f, 0.0f);
            Vector3 line2End(2.0f, 4.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(line1Proportion == 0.0f);
            EXPECT_TRUE(line2Proportion == 0.5f);
        }

        // line 2 degenerates to point
        {
            Vector3 line1Start(0.0f, 0.0f, 0.0f);
            Vector3 line1End(4.0f, 0.0f, 0.0f);
            Vector3 line2Start(2.0f, 1.0f, 0.0f);
            Vector3 line2End(2.0f, 1.0f, 0.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            EXPECT_TRUE(pointDifference == 1.0f);
            EXPECT_TRUE(line1Proportion == 0.5f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }

        // both lines degenerate to points
        {
            Vector3 line1Start(5.0f, 5.0f, 5.0f);
            Vector3 line1End(5.0f, 5.0f, 5.0f);
            Vector3 line2Start(10.0f, 10.0f, 10.0f);
            Vector3 line2End(10.0f, 10.0f, 10.0f);

            Vector3 line1ClosestPoint;
            Vector3 line2ClosestPoint;
            VectorFloat line1Proportion;
            VectorFloat line2Proportion;
            Intersect::ClosestSegmentSegment(line1Start, line1End, line2Start, line2End, line1Proportion, line2Proportion, line1ClosestPoint, line2ClosestPoint);
            float pointDifference = (line2ClosestPoint - line1ClosestPoint).GetLength();

            // (10, 10, 10) - (5, 5, 5) == (5, 5, 5)
            // |(5,5,5)| == sqrt(5*5+5*5+5*5) == sqrt(75)
            EXPECT_TRUE(pointDifference == std::sqrtf(75.0f));
            EXPECT_TRUE(line1Proportion == 0.0f);
            EXPECT_TRUE(line2Proportion == 0.0f);
        }
    }

    TEST(MATH_Intersection, ClosestPointSegment)
    {
        // point above center of line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(4.0f, 0.0f, 0.0f);
            Vector3 point(2.0f, 2.0f, 0.0f);

            Vector3 lineClosestPoint;
            VectorFloat lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(lineProportion == 0.5f);
        }

        // point same height behind line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(0.0f, 4.0f, 0.0f);
            Vector3 point(0.0f, -2.0f, 0.0f);

            Vector3 lineClosestPoint;
            VectorFloat lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 2.0f);
            EXPECT_TRUE(lineProportion == 0.0f);
        }

        // point passed end of line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(0.0f, 0.0f, 10.0f);
            Vector3 point(0.0f, 0.0f, 15.0f);

            Vector3 lineClosestPoint;
            VectorFloat lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 5.0f);
            EXPECT_TRUE(lineProportion == 1.0f);
        }

        // point above part way along line
        {
            Vector3 lineStart(0.0f, 0.0f, 0.0f);
            Vector3 lineEnd(4.0f, 4.0f, 0.0f);
            Vector3 point(3.0f, 3.0f, -1.0f);

            Vector3 lineClosestPoint;
            VectorFloat lineProportion;
            Intersect::ClosestPointSegment(point, lineStart, lineEnd, lineProportion, lineClosestPoint);

            float pointDifference = (lineClosestPoint - point).GetLength();

            EXPECT_TRUE(pointDifference == 1.0f);
            EXPECT_TRUE(lineProportion == 0.75f);
        }
    }

    class MATH_SplineTest
        : public AllocatorsFixture
    {
    public:
        void Linear_NearestAddressFromPosition()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(7.5f, 2.5f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-1.0f, -1.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-1.0f, 6.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(2.5f, 6.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-2.0f, 2.5f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = linearSpline.GetNearestAddressPosition(Vector3(-2.0f, 4.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.2f);
                }
            }
        }

        void Linear_NearestAddressFromDirection()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(2.5f, -2.5f, 1.0f), Vector3(0.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(2.5f, -10.0f, 0.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(7.5f, 2.5f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, -1.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = linearSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }
            }
        }

        void Linear_AddressByDistance()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(7.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(15.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(11.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.2f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(3.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.6f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(20.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(-5.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(17.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByDistance(20.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }
            }
        }

        void Linear_AddressByFraction()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(1.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.6f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.8f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.2f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.6f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(1.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(-0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(1.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = linearSpline.GetAddressByFraction(0.8f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.2f);
                }
            }
        }

        void Linear_GetPosition()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_TRUE(position == Vector3(5.0f, 2.5f, 0.0f));
                }

                {
                    Vector3 position = linearSpline.GetPosition(SplineAddress());
                    EXPECT_TRUE(position == Vector3(0.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 position = linearSpline.GetPosition(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_TRUE(position == Vector3(0.0f, 5.0f, 0.0f));
                }

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByDistance(4.0f));
                    EXPECT_TRUE(position == Vector3(4.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 position = linearSpline.GetPosition(SplineAddress(3, 0.0f));
                    EXPECT_TRUE(position == Vector3(0.0f, 5.0f, 0.0f));
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_TRUE(position == Vector3(5.0f, 5.0f, 0.0f));
                }

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByDistance(20.0f));
                    EXPECT_TRUE(position == Vector3(0.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 position = linearSpline.GetPosition(linearSpline.GetAddressByDistance(18.0f));
                    EXPECT_TRUE(position == Vector3(0.0f, 2.0f, 0.0f));
                }
            }
        }

        void Linear_GetNormal()
        {
            {
                LinearSpline linearSpline;

                // n shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_TRUE(normal == Vector3(0.0f, 1.0f, 0.0f));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(SplineAddress());
                    EXPECT_TRUE(normal == Vector3(-1.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_TRUE(normal == Vector3(1.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByDistance(4.0f));
                    EXPECT_TRUE(normal == Vector3(-1.0f, 0.0f, 0.0f));
                }
            }

            {
                LinearSpline linearSpline;

                // n shape spline (yz plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 5.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 5.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByDistance(4.0f));
                EXPECT_TRUE(normal == Vector3(1.0f, 0.0f, 0.0f));
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // n shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByDistance(20.0f));
                    EXPECT_TRUE(normal == Vector3(0.0f, -1.0f, 0.0f));
                }

                {
                    Vector3 normal = linearSpline.GetNormal(linearSpline.GetAddressByFraction(0.8f));
                    EXPECT_TRUE(normal == Vector3(0.0f, -1.0f, 0.0f));
                }
            }
        }

        void Linear_GetTangent()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 tangent = linearSpline.GetTangent(linearSpline.GetAddressByFraction(0.5f));
                    EXPECT_TRUE(tangent == Vector3(0.0f, 1.0f, 0.0f));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(SplineAddress());
                    EXPECT_TRUE(tangent == Vector3(1.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_TRUE(tangent == Vector3(-1.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(linearSpline.GetAddressByDistance(4.0f));
                    EXPECT_TRUE(tangent == Vector3(1.0f, 0.0f, 0.0f));
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    Vector3 tangent = linearSpline.GetTangent(SplineAddress(linearSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_TRUE(tangent == Vector3(0.0f, -1.0f, 0.0f));
                }

                {
                    Vector3 tangent = linearSpline.GetTangent(linearSpline.GetAddressByDistance(16.0f));
                    EXPECT_TRUE(tangent == Vector3(0.0f, -1.0f, 0.0f));
                }
            }
        }

        void Linear_Length()
        {
            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_TRUE(splineLength == 15.0f);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(0);
                    EXPECT_TRUE(segmentLength == 5.0f);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(2);
                    EXPECT_TRUE(segmentLength == 5.0f);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(4);
                    EXPECT_TRUE(segmentLength == 0.0f);
                }
            }

            {
                LinearSpline linearSpline;

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(2.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(2.0f, 3.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(4.0f, 3.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(4.0f, 10.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_TRUE(splineLength == 14.0f);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(0);
                    EXPECT_TRUE(segmentLength == 2.0f);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(3);
                    EXPECT_TRUE(segmentLength == 7.0f);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // backward C shaped spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_TRUE(splineLength == 20.0f);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(3);
                    EXPECT_TRUE(segmentLength == 5.0f);
                }
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // single line (y axis)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));

                {
                    float splineLength = linearSpline.GetSplineLength();
                    EXPECT_TRUE(splineLength == 20.0f);
                }

                {
                    float segmentLength = linearSpline.GetSegmentLength(1);
                    EXPECT_TRUE(segmentLength == 10.0f);
                }
            }
        }

        void Linear_Aabb()
        {
            {
                LinearSpline linearSpline;

                // slight n curve spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                linearSpline.GetAabb(aabb);

                EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(10.0f, 10.0f, 0.0f)));
            }

            {
                LinearSpline linearSpline;
                linearSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                linearSpline.GetAabb(aabb);

                EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(10.0f, 10.0f, 0.0f)));
            }

            {
                LinearSpline linearSpline;
                AZ::Transform translation = AZ::Transform::CreateFromMatrix3x3AndTranslation(Matrix3x3::CreateIdentity(), Vector3(25.0f, 25.0f, 0.0f));

                // slight n curve spline (xy plane)
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                linearSpline.GetAabb(aabb, translation);

                EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(25.0f, 25.0f, 0.0f)));
                EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(35.0f, 35.0f, 0.0f)));
            }
        }

        void Linear_GetLength()
        {
            {
                LinearSpline spline;

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 20.0f, 0.0f));

                auto distance = spline.GetLength(AZ::SplineAddress(1, 0.5f));
                EXPECT_NEAR(distance, 15.0f, 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(distance, 0.0f, 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(1, 1.0f));
                EXPECT_NEAR(distance, 20.0f, 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(5, 1.0f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);
            }

            {
                LinearSpline spline;

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(20.0f, 20.0f, 10.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 15.0f, -10.0f));

                float length = spline.GetSplineLength();

                for (float t = 0.0f; t <= length; t += 0.5f)
                {
                    auto result = spline.GetLength(spline.GetAddressByDistance(t));
                    EXPECT_NEAR(result, t, 1e-4);
                }
            }

            {
                LinearSpline spline;

                auto result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }
        }

        void CatmullRom_NearestAddressFromPosition()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight backward C curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(7.5f, 2.5f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(-1.0f, -1.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(-1.0f, 6.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(2.5f, 6.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = catmullRomSpline.GetNearestAddressPosition(Vector3(5.0f, -2.5f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }
            }
        }

        void CatmullRom_NearestAddressFromDirection()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight backward C curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(2.5f, -2.5f, 1.0f), Vector3(0.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(2.5f, -10.0f, 0.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(7.5f, 2.5f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(-2.5f, 7.5f, -1.0f), Vector3(1.0f, 0.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight backward C curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 5.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = catmullRomSpline.GetNearestAddressRay(Vector3(-2.5f, 2.5f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }
            }
        }

        /*
            Catmull-Rom Sample Points - (0.0f, 0.0f, 0.0f), (0.0f, 10.0f, 0.0f), (10.0f, 10.0f, 0.0f), (10.0f, 0.0f, 0.0f)
            (calculated using https://www.desmos.com/calculator/552cpvzfxw)

            t = 0         pos = (0.0f, 10.0f, 0.0f)
            t = 0.125     pos = (0.83984375.0f, 10.546875.0f, 0.0f)
            t = 0.25      pos = (2.03125.0f, 10.9375.0f, 0.0f)
            t = 0.375     pos = (3.45703125.0f, 11.171875.0f, 0.0f)
            t = 0.5       pos = (5.0.0f, 11.25.0f, 0.0f)
            t = 0.625     pos = (6.54296875.0f, 11.171875.0f, 0.0f)
            t = 0.75      pos = (7.96875.0f, 10.9375.0f, 0.0f)
            t = 0.875     pos = (9.16015625.0f, 10.546875.0f, 0.0f)
            t = 1         pos = (10.0f, 10.0f)

            delta = (0.83984375.0f, 0.546875.0f, 0.0f)     length = 1.00220
            delta = (1.19140625.0f, 0.390625.0f, 0.0f)     length = 1.25381
            delta = (1.42578125.0f, 0.234375.0f, 0.0f)     length = 1.44492
            delta = (1.54296875.0f, 0.078125.0f, 0.0f)     length = 1.54495
            delta = (1.54296875.0f, -0.078125.0f, 0.0f)    length = 1.54495
            delta = (1.42578125.0f, -0.234375.0f, 0.0f)    length = 1.44492
            delta = (1.19140625.0f, -0.390625.0f, 0.0f)    length = 1.25381
            delta = (0.83984375.0f, -0.546875.0f, 0.0f)    length = 1.00220

            total = 10.49176
        */

        void CatmullRom_AddressByDistance()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(20.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    // note: spline length is approx 10.49176
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(5.24588f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_NEAR(0.5f, splineAddress.m_segmentFraction, 0.0001f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(-10.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(50.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByDistance(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }
            }
        }

        void CatmullRom_AddressByFraction()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(1.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.75f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.75f);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(1.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = catmullRomSpline.GetAddressByFraction(0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }
            }
        }

        void CatmullRom_GetPosition()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.125f));
                    EXPECT_TRUE(position.IsClose(Vector3(0.83984375f, 10.546875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.625f));
                    EXPECT_TRUE(position.IsClose(Vector3(6.54296875f, 11.171875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(3, 0.5f));
                    EXPECT_TRUE(position.IsClose(Vector3(10.0f, 10.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(0, 0.0f));
                    EXPECT_TRUE(position.IsClose(Vector3(0.0f, 10.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.0f));
                    EXPECT_TRUE(position.IsClose(Vector3(0.0f, 10.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(2, 0.0f));
                    EXPECT_TRUE(position == Vector3(10.0f, 10.0f, 0.0f));
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.125f));
                    EXPECT_TRUE(position.IsClose(Vector3(0.83984375f, 10.546875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(1, 0.625f));
                    EXPECT_TRUE(position.IsClose(Vector3(6.54296875f, 11.171875f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(0, 0.0f));
                    EXPECT_TRUE(position.IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(3, 1.0f));
                    EXPECT_TRUE(position.IsClose(Vector3(0.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 position = catmullRomSpline.GetPosition(SplineAddress(0, 0.5f));
                    EXPECT_TRUE(position == Vector3(-1.25f, 5.0f, 0.0f));
                }
            }
        }

        void CatmullRom_GetNormal()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 0.0f));
                    EXPECT_TRUE(normal.IsClose(Vector3(-5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 1.0f));
                    EXPECT_TRUE(normal.IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 0.5f));
                    EXPECT_TRUE(normal.IsClose(Vector3(0.0f, 12.5f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(2, 0.0f));
                    EXPECT_TRUE(normal.IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(3, 0.5f));
                    EXPECT_TRUE(normal.IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(1, 0.5f));
                    EXPECT_TRUE(normal.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
                }

                {
                    Vector3 normal = catmullRomSpline.GetNormal(SplineAddress(3, 0.5f));
                    EXPECT_TRUE(normal.IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }
            }
        }

        void CatmullRom_GetTangent()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 0.0f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(5.0f, 5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 1.0f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(5.0f, -5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 0.5f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(12.5f, 0.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(2, 0.0f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(5.0f, -5.0f, 0.0f).GetNormalized()));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(3, 0.5f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(5.0f, -5.0f, 0.0f).GetNormalized()));
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(3, 0.5f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(-1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(1, 0.5f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = catmullRomSpline.GetTangent(SplineAddress(3, 1.0f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(-5.0f, 5.0f, 0.0f).GetNormalized()));
                }
            }
        }

        void CatmullRom_Length()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float length = catmullRomSpline.GetSplineLength();
                    EXPECT_NEAR(10.49176f, length, 0.0001f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(0);
                    EXPECT_TRUE(length == 0.0f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(1);
                    EXPECT_NEAR(10.49176f, length, 0.0001f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength((size_t)-1);
                    EXPECT_TRUE(length == 0.0f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(10);
                    EXPECT_TRUE(length == 0.0f);
                }
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float length = catmullRomSpline.GetSplineLength();
                    EXPECT_NEAR(41.96704, length, 0.0001f);
                }

                {
                    float length = catmullRomSpline.GetSegmentLength(3);
                    EXPECT_NEAR(10.49176f, length, 0.0001f);
                }
            }
        }

        void CatmullRom_Aabb()
        {
            {
                CatmullRomSpline catmullRomSpline;

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                catmullRomSpline.GetAabb(aabb);

                EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(0.0f, 10.0f, 0.0f)));
                EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(10.0f, 11.25f, 0.0f)));
            }

            {
                CatmullRomSpline catmullRomSpline;
                catmullRomSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                catmullRomSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                catmullRomSpline.GetAabb(aabb);

                EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-1.25f, -1.25f, 0.0f)));
                EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(11.25f, 11.25f, 0.0f)));
            }
        }

        void CatmullRom_GetLength()
        {
            {
                CatmullRomSpline spline;

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(20.0f, 20.0f, 10.0f));
                spline.m_vertexContainer.AddVertex(Vector3(10.0f, 15.0f, -10.0f));

                float length = spline.GetSplineLength();

                for (float t = 0.0f; t <= length; t += 0.5f)
                {
                    auto result = spline.GetLength(spline.GetAddressByDistance(t));
                    EXPECT_NEAR(result, t, 1e-4);
                }

                auto distance = spline.GetLength(AZ::SplineAddress(3, 0.5f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(4, 1.0f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);

                distance = spline.GetLength(AZ::SplineAddress(5, 1.0f));
                EXPECT_NEAR(distance, spline.GetSplineLength(), 1e-4);
            }

            {
                CatmullRomSpline spline;

                auto result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);

                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }

            {
                CatmullRomSpline spline;
                spline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                spline.m_vertexContainer.AddVertex(Vector3(5.0f, 5.0f, 0.0f));

                auto result = spline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }
        }

        void Bezier_NearestAddressFromPosition()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(-1.0f, -1.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(5.0f, 12.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(12.0f, -1.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(5.0f, -12.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    PositionSplineQueryResult posSplineQueryResult = bezierSpline.GetNearestAddressPosition(Vector3(12.0f, 12.0f, 0.0f));
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(posSplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }
            }
        }

        void Bezier_NearestAddressFromDirection()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(-1.0f, -1.0f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(5.0f, 12.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 1);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(12.0f, -1.0f, 0.0f), Vector3(1.0f, 1.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 1.0f);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    RaySplineQueryResult raySplineQueryResult = bezierSpline.GetNearestAddressRay(Vector3(-10.0f, -10.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(raySplineQueryResult.m_splineAddress.m_segmentFraction == 0.5f);
                }
            }
        }

        void Bezier_AddressByDistance()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(-5.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(100.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByDistance(100.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }
            }
        }

        void Bezier_AddressByFraction()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(0.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(-0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 0);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 0.0f);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(2.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(1.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 1);
                    EXPECT_FLOAT_EQ(splineAddress.m_segmentFraction, 0.5f);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(1.0f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 3);
                    EXPECT_TRUE(splineAddress.m_segmentFraction == 1.0f);
                }

                {
                    SplineAddress splineAddress = bezierSpline.GetAddressByFraction(0.5f);
                    EXPECT_TRUE(splineAddress.m_segmentIndex == 2);
                    EXPECT_FLOAT_EQ(0.0f, splineAddress.m_segmentFraction);
                }
            }
        }

        void Bezier_GetPosition()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = bezierSpline.GetPosition(bezierSpline.GetAddressByFraction(1.0f));
                    EXPECT_TRUE(position == Vector3(10.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress());
                    EXPECT_TRUE(position == Vector3(0.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress(bezierSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_TRUE(position == Vector3(10.0f, 0.0f, 0.0f));
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress());
                    EXPECT_TRUE(position == Vector3(0.0f, 0.0f, 0.0f));
                }

                {
                    Vector3 position = bezierSpline.GetPosition(SplineAddress(bezierSpline.GetSegmentCount() - 1, 1.0f));
                    EXPECT_TRUE(position == Vector3(0.0f, 0.0f, 0.0f));
                }
            }
        }

        void Bezier_GetNormal()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(1, 0.5f));
                    EXPECT_TRUE(normal.IsClose(Vector3(0.0f, 1.0f, 0.0f)));
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 normal = bezierSpline.GetNormal(SplineAddress(3, 0.5f));
                    EXPECT_TRUE(normal.IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }
            }
        }

        void Bezier_GetTangent()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = bezierSpline.GetTangent(SplineAddress(1, 0.5f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(1.0f, 0.0f, 0.0f)));
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    Vector3 tangent = bezierSpline.GetTangent(SplineAddress(3, 0.5f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(-1.0f, 0.0f, 0.0f)));
                }

                {
                    Vector3 tangent = bezierSpline.GetTangent(SplineAddress(2, 0.5f));
                    EXPECT_TRUE(tangent.IsClose(Vector3(0.0f, -1.0f, 0.0f)));
                }
            }
        }

        void Bezier_Length()
        {
            {
                BezierSpline bezierSpline;

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float splineLength = bezierSpline.GetSplineLength();
                    EXPECT_NEAR(splineLength, 31.8014f, 0.0001f);
                }

                // lengths should be exact as shape is symmetrical
                {
                    float segment0Length = bezierSpline.GetSegmentLength(0);
                    float segment2Length = bezierSpline.GetSegmentLength(2);
                    EXPECT_FLOAT_EQ(segment0Length, segment2Length);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // n shape (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                {
                    float splineLength = bezierSpline.GetSplineLength();
                    EXPECT_FLOAT_EQ(splineLength, 43.40867f);
                }

                {
                    float segment0Length = bezierSpline.GetSegmentLength(0);
                    float segment2Length = bezierSpline.GetSegmentLength(3);
                    EXPECT_FLOAT_EQ(segment0Length, segment2Length);
                }
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // single line (y axis)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));

                {
                    float splineLength = bezierSpline.GetSplineLength();
                    EXPECT_TRUE(splineLength == 20.0f);
                }

                {
                    float segmentLength = bezierSpline.GetSegmentLength(1);
                    EXPECT_TRUE(segmentLength == 10.0f);
                }
            }
        }

        void Bezier_GetLength()
        {
            {
                BezierSpline bezierSpline;

                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 20.0f, 0.0f));

                auto distance = bezierSpline.GetLength(AZ::SplineAddress(1, 0.5f));
                EXPECT_NEAR(distance, 15.0f, 1e-4);

                distance = bezierSpline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(distance, 0.0f, 1e-4);

                distance = bezierSpline.GetLength(AZ::SplineAddress(1, 1.0f));
                EXPECT_NEAR(distance, 20.0f, 1e-4);

                distance = bezierSpline.GetLength(AZ::SplineAddress(5, 1.0f));
                EXPECT_NEAR(distance, bezierSpline.GetSplineLength(), 1e-4);
            }

            {
                BezierSpline bezierSpline;

                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(20.0f, 20.0f, 10.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 15.0f, -10.0f));

                float length = bezierSpline.GetSplineLength();

                for (float t = 0.0f; t <= length; t += 0.5f)
                {
                    auto result = bezierSpline.GetLength(bezierSpline.GetAddressByDistance(t));
                    EXPECT_NEAR(result, t, 1e-4);
                }
            }

            {
                BezierSpline bezierSpline;

                auto result = bezierSpline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);

                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                result = bezierSpline.GetLength(AZ::SplineAddress(0, 0.0f));
                EXPECT_NEAR(result, 0.0f, 1e-4);
            }
        }

        void Bezier_Aabb()
        {
            {
                BezierSpline bezierSpline;

                // slight n curve spline (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                bezierSpline.GetAabb(aabb);

                EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-1.2948f, 0.0f, 0.0f)));
                EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(11.2948f, 11.7677f, 0.0f)));
            }

            {
                BezierSpline bezierSpline;
                bezierSpline.SetClosed(true);

                // slight n curve spline (xy plane)
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
                bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

                AZ::Aabb aabb;
                bezierSpline.GetAabb(aabb);

                EXPECT_TRUE(aabb.GetMin().IsClose(Vector3(-1.7677f, -1.7677f, 0.0f)));
                EXPECT_TRUE(aabb.GetMax().IsClose(Vector3(11.7677f, 11.7677f, 0.0f)));
            }
        }

        void Spline_Invalid()
        {
            {
                LinearSpline linearSpline;

                PositionSplineQueryResult posSplineQuery = linearSpline.GetNearestAddressPosition(Vector3::CreateZero());
                EXPECT_TRUE(posSplineQuery.m_splineAddress == SplineAddress());
                EXPECT_TRUE(posSplineQuery.m_distanceSq == std::numeric_limits<float>::max());
                RaySplineQueryResult raySplineQuery = linearSpline.GetNearestAddressRay(Vector3::CreateZero(), Vector3::CreateZero());
                EXPECT_TRUE(raySplineQuery.m_splineAddress == SplineAddress());
                EXPECT_TRUE(raySplineQuery.m_distanceSq == std::numeric_limits<float>::max());
                EXPECT_TRUE(raySplineQuery.m_rayDistance == std::numeric_limits<float>::max());
                SplineAddress linearAddressDistance = linearSpline.GetAddressByDistance(2.0f);
                EXPECT_TRUE(linearAddressDistance == SplineAddress());
                SplineAddress linearAddressFraction = linearSpline.GetAddressByFraction(0.0f);
                EXPECT_TRUE(linearAddressFraction == SplineAddress());
                Vector3 linearPosition = linearSpline.GetPosition(SplineAddress(0, 0.5f));
                EXPECT_TRUE(linearPosition == Vector3::CreateZero());
                Vector3 linearNormal = linearSpline.GetNormal(SplineAddress(1, 0.5f));
                EXPECT_TRUE(linearNormal == Vector3::CreateAxisX());
                Vector3 linearTangent = linearSpline.GetTangent(SplineAddress(2, 0.5f));
                EXPECT_TRUE(linearTangent == Vector3::CreateAxisX());
                float linearSegmentLength = linearSpline.GetSegmentLength(2);
                EXPECT_TRUE(linearSegmentLength == 0.0f);
                float linearSplineLength = linearSpline.GetSplineLength();
                EXPECT_TRUE(linearSplineLength == 0.0f);
            }

            {
                CatmullRomSpline catmullRomSpline;

                PositionSplineQueryResult posSplineQuery = catmullRomSpline.GetNearestAddressPosition(Vector3::CreateZero());
                EXPECT_TRUE(posSplineQuery.m_splineAddress == SplineAddress());
                EXPECT_TRUE(posSplineQuery.m_distanceSq == std::numeric_limits<float>::max());
                RaySplineQueryResult raySplineQuery = catmullRomSpline.GetNearestAddressRay(Vector3::CreateZero(), Vector3::CreateZero());
                EXPECT_TRUE(raySplineQuery.m_splineAddress == SplineAddress());
                EXPECT_TRUE(raySplineQuery.m_distanceSq == std::numeric_limits<float>::max());
                EXPECT_TRUE(raySplineQuery.m_rayDistance == std::numeric_limits<float>::max());
                SplineAddress catmullRomAddressDistance = catmullRomSpline.GetAddressByDistance(2.0f);
                EXPECT_TRUE(catmullRomAddressDistance == SplineAddress());
                SplineAddress catmullRomAddressFraction = catmullRomSpline.GetAddressByFraction(0.0f);
                EXPECT_TRUE(catmullRomAddressFraction == SplineAddress());
                Vector3 catmullRomPosition = catmullRomSpline.GetPosition(SplineAddress(0, 0.5f));
                EXPECT_TRUE(catmullRomPosition == Vector3::CreateZero());
                Vector3 catmullRomNormal = catmullRomSpline.GetNormal(SplineAddress(1, 0.5f));
                EXPECT_TRUE(catmullRomNormal == Vector3::CreateAxisX());
                Vector3 catmullRomTangent = catmullRomSpline.GetTangent(SplineAddress(2, 0.5f));
                EXPECT_TRUE(catmullRomTangent == Vector3::CreateAxisX());
                float catmullRomSegmentLength = catmullRomSpline.GetSegmentLength(2);
                EXPECT_TRUE(catmullRomSegmentLength == 0.0f);
                float catmullRomSplineLength = catmullRomSpline.GetSplineLength();
                EXPECT_TRUE(catmullRomSplineLength == 0.0f);
            }

            {
                BezierSpline bezierSpline;

                PositionSplineQueryResult posSplineQuery = bezierSpline.GetNearestAddressPosition(Vector3::CreateZero());
                EXPECT_TRUE(posSplineQuery.m_splineAddress == SplineAddress());
                EXPECT_TRUE(posSplineQuery.m_distanceSq == std::numeric_limits<float>::max());
                RaySplineQueryResult raySplineQuery = bezierSpline.GetNearestAddressRay(Vector3::CreateZero(), Vector3::CreateZero());
                EXPECT_TRUE(raySplineQuery.m_splineAddress == SplineAddress());
                EXPECT_TRUE(raySplineQuery.m_distanceSq == std::numeric_limits<float>::max());
                EXPECT_TRUE(raySplineQuery.m_rayDistance == std::numeric_limits<float>::max());
                SplineAddress bezierAddressDistance = bezierSpline.GetAddressByDistance(2.0f);
                EXPECT_TRUE(bezierAddressDistance == SplineAddress());
                SplineAddress bezierAddressFraction = bezierSpline.GetAddressByFraction(0.0f);
                EXPECT_TRUE(bezierAddressFraction == SplineAddress());
                Vector3 bezierPosition = bezierSpline.GetPosition(SplineAddress(0, 0.5f));
                EXPECT_TRUE(bezierPosition == Vector3::CreateZero());
                Vector3 bezierNormal = bezierSpline.GetNormal(SplineAddress(1, 0.5f));
                EXPECT_TRUE(bezierNormal == Vector3::CreateAxisX());
                Vector3 bezierTangent = bezierSpline.GetTangent(SplineAddress(2, 0.5f));
                EXPECT_TRUE(bezierTangent == Vector3::CreateAxisX());
                float bezierSegmentLength = bezierSpline.GetSegmentLength(2);
                EXPECT_TRUE(bezierSegmentLength == 0.0f);
                float bezierSplineLength = bezierSpline.GetSplineLength();
                EXPECT_TRUE(bezierSplineLength == 0.0f);
            }
        }

        void Spline_Set()
        {
            LinearSpline linearSpline;

            // slight n curve spline (xy plane)
            linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
            linearSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
            linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
            linearSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

            AZStd::vector<Vector3> vertices {
                Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), Vector3(10.0f, 10.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f)
            };

            LinearSpline linearSplineSetLValue;
            linearSplineSetLValue.m_vertexContainer.SetVertices(vertices);

            LinearSpline linearSplineSetRValue;
            linearSplineSetRValue.m_vertexContainer.SetVertices(AZStd::vector<Vector3>{ Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), Vector3(10.0f, 10.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f) });

            LinearSpline linearSplineSetLValueCopy;
            linearSplineSetLValueCopy.m_vertexContainer.SetVertices(linearSplineSetLValue.GetVertices());

            EXPECT_TRUE(linearSpline.GetVertexCount() == linearSplineSetLValue.GetVertexCount());
            EXPECT_TRUE(linearSpline.GetVertexCount() == linearSplineSetRValue.GetVertexCount());
            EXPECT_TRUE(linearSplineSetLValue.GetVertexCount() == linearSplineSetRValue.GetVertexCount());
            EXPECT_TRUE(linearSplineSetLValueCopy.GetVertexCount() == vertices.size());

            BezierSpline bezierSpline;

            // slight n curve spline (xy plane)
            bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 0.0f, 0.0f));
            bezierSpline.m_vertexContainer.AddVertex(Vector3(0.0f, 10.0f, 0.0f));
            bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 10.0f, 0.0f));
            bezierSpline.m_vertexContainer.AddVertex(Vector3(10.0f, 0.0f, 0.0f));

            BezierSpline bezierSplineSetLValue;
            bezierSplineSetLValue.m_vertexContainer.SetVertices(vertices);

            BezierSpline bezierSplineSetRValue;
            bezierSplineSetRValue.m_vertexContainer.SetVertices(AZStd::vector<Vector3>{ Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 10.0f, 0.0f), Vector3(10.0f, 10.0f, 0.0f), Vector3(10.0f, 0.0f, 0.0f) });

            BezierSpline bezierSplineSetLValueCopy;
            bezierSplineSetLValueCopy.m_vertexContainer.SetVertices(bezierSplineSetLValue.GetVertices());

            EXPECT_TRUE(bezierSpline.GetBezierData().size() == bezierSplineSetLValue.GetBezierData().size());
            EXPECT_TRUE(bezierSplineSetLValue.GetBezierData().size() == 4);
            EXPECT_TRUE(bezierSplineSetLValue.GetBezierData().size() == bezierSplineSetRValue.GetBezierData().size());
            EXPECT_TRUE(bezierSplineSetLValueCopy.GetBezierData().size() == 4);
            EXPECT_TRUE(bezierSplineSetLValueCopy.GetVertexCount() == vertices.size());
        }
    };

    TEST_F(MATH_SplineTest, Linear_NearestAddressFromPosition)
    {
        Linear_NearestAddressFromPosition();
    }

    TEST_F(MATH_SplineTest, Linear_NearestAddressFromDirection)
    {
        Linear_NearestAddressFromDirection();
    }

    TEST_F(MATH_SplineTest, Linear_AddressByDistance)
    {
        Linear_AddressByDistance();
    }

    TEST_F(MATH_SplineTest, Linear_AddressByFraction)
    {
        Linear_AddressByFraction();
    }

    TEST_F(MATH_SplineTest, Linear_GetPosition)
    {
        Linear_GetPosition();
    }

    TEST_F(MATH_SplineTest, Linear_GetNormal)
    {
        Linear_GetNormal();
    }

    TEST_F(MATH_SplineTest, Linear_GetTangent)
    {
        Linear_GetTangent();
    }

    TEST_F(MATH_SplineTest, Linear_Length)
    {
        Linear_Length();
    }

    TEST_F(MATH_SplineTest, Linear_Aabb)
    {
        Linear_Aabb();
    }

    TEST_F(MATH_SplineTest, Linear_GetLength)
    {
        Linear_GetLength();
    }

    TEST_F(MATH_SplineTest, CatmullRom_Length)
    {
        CatmullRom_Length();
    }

    TEST_F(MATH_SplineTest, CatmullRom_NearestAddressFromPosition)
    {
        CatmullRom_NearestAddressFromPosition();
    }

    TEST_F(MATH_SplineTest, CatmullRom_NearestAddressFromDirection)
    {
        CatmullRom_NearestAddressFromDirection();
    }

    TEST_F(MATH_SplineTest, CatmullRom_AddressByDistance)
    {
        CatmullRom_AddressByDistance();
    }

    TEST_F(MATH_SplineTest, CatmullRom_AddressByFraction)
    {
        CatmullRom_AddressByFraction();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetPosition)
    {
        CatmullRom_GetPosition();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetNormal)
    {
        CatmullRom_GetNormal();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetTangent)
    {
        CatmullRom_GetTangent();
    }

    TEST_F(MATH_SplineTest, CatmullRom_Aabb)
    {
        CatmullRom_Aabb();
    }

    TEST_F(MATH_SplineTest, CatmullRom_GetLength)
    {
        CatmullRom_GetLength();
    }

    TEST_F(MATH_SplineTest, Bezier_NearestAddressFromPosition)
    {
        Bezier_NearestAddressFromPosition();
    }

    TEST_F(MATH_SplineTest, Bezier_NearestAddressFromDirection)
    {
        Bezier_NearestAddressFromDirection();
    }

    TEST_F(MATH_SplineTest, Bezier_AddressByDistance)
    {
        Bezier_AddressByDistance();
    }

    TEST_F(MATH_SplineTest, Bezier_AddressByFraction)
    {
        Bezier_AddressByFraction();
    }

    TEST_F(MATH_SplineTest, Bezier_GetPosition)
    {
        Bezier_GetPosition();
    }

    TEST_F(MATH_SplineTest, Bezier_GetNormal)
    {
        Bezier_GetNormal();
    }

    TEST_F(MATH_SplineTest, Bezier_GetTangent)
    {
        Bezier_GetTangent();
    }

    TEST_F(MATH_SplineTest, Bezier_Length)
    {
        Bezier_Length();
    }

    TEST_F(MATH_SplineTest, Bezier_Aabb)
    {
        Bezier_Aabb();
    }

    TEST_F(MATH_SplineTest, Bezier_GetLength)
    {
        Bezier_GetLength();
    }

    TEST_F(MATH_SplineTest, Spline_Invalid)
    {
        Spline_Invalid();
    }

    TEST_F(MATH_SplineTest, Spline_Set)
    {
        Spline_Set();
    }

    class MATH_IntersectRayCappedCylinderTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_cylinderEnd1 = Vector3(1.1f, 2.2f, 3.3f);
            m_cylinderDir = Vector3(1.0, 1.0f, 1.0f);
            m_cylinderDir.NormalizeExact();
            m_radiusDir = Vector3(-1.0f, 1.0f, 0.0f);
            m_radiusDir.NormalizeExact();
            m_cylinderHeight = 5.5f;
            m_cylinderRadius = 3.768f;
        }

        void TearDown() override
        {

        }

        Vector3 m_cylinderEnd1;
        Vector3 m_cylinderDir;
        Vector3 m_radiusDir;
        float m_cylinderHeight;
        float m_cylinderRadius;
    };

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOverlapCylinderAxis)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayParallelToAndInsideCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir + 0.5f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayAlongCylinderSuface)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir + m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayParallelAndRayOriginInsideCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.2f * m_cylinderHeight * m_cylinderDir + 0.5f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayParallelToButOutsideCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 - 0.5f * m_cylinderHeight * m_cylinderDir + 1.1f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOuside_RayDirParallelButPointingAwayCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 1.5f * m_cylinderHeight * m_cylinderDir + 0.5f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = m_cylinderDir;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideEnds_BothIntersectionsInbetweenEnds)
    {
        Vector3 intersection2 = m_cylinderEnd1 + 0.8f * m_cylinderHeight * m_cylinderDir - m_cylinderRadius * m_radiusDir;
        Vector3 intersection1 = m_cylinderEnd1 + 0.2f * m_cylinderHeight * m_cylinderDir + m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = intersection2 - intersection1;
        Vector3 rayOrigin = intersection1 - 2.0f * rayDir;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideEnds_Intersection1OnEnd1_Intersection2InbetweenEnds)
    {
        Vector3 intersection2 = m_cylinderEnd1 + 0.6f * m_cylinderHeight * m_cylinderDir - m_cylinderRadius * m_radiusDir;
        Vector3 intersection1 = m_cylinderEnd1 + 0.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = intersection2 - intersection1;
        Vector3 rayOrigin = intersection1 - 2.0f * rayDir;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginInside_RayGoThroughCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.3f * m_cylinderHeight * m_cylinderDir - 0.4f * m_cylinderRadius * m_radiusDir;
        Vector3 intersection = m_cylinderEnd1 + 0.8f * m_cylinderHeight * m_cylinderDir + m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (intersection - rayOrigin).GetNormalizedExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirPointingAwayCylinder)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = rayOrigin - m_cylinderEnd1;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirMissingCylinderEnds)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (m_cylinderEnd1 + m_cylinderHeight * m_cylinderDir - 1.2f * m_cylinderRadius * m_radiusDir) - rayOrigin;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirShootingOutEnd1)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (m_cylinderEnd1 - 0.2f * m_cylinderRadius * m_radiusDir) - rayOrigin;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirShootingOutEnd2)
    {
        Vector3 rayOrigin = m_cylinderEnd1 + 0.4f * m_cylinderHeight * m_cylinderDir - 1.6f * m_cylinderRadius * m_radiusDir;
        Vector3 rayDir = (m_cylinderEnd1 + 0.67f * m_cylinderHeight * m_cylinderDir) - rayOrigin;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayCappedCylinderTest, RayOriginOutsideCylinderButInBetweenEnds_RayDirShootingOutCylinder)
    {
        Vector3 perpDir(1.0f, 0, 0);
        float parallelDiff = 0.453f;
        float perpendicularDiff = 2.54f;
        Vector3 rayOrigin = (m_cylinderEnd1 + parallelDiff * m_cylinderHeight * m_cylinderDir) + perpendicularDiff * m_cylinderRadius * perpDir;
        Vector3 rayDir = (m_cylinderEnd1 + m_cylinderHeight * m_cylinderDir - 2.6f * m_cylinderRadius * perpDir) - rayOrigin;
        rayDir.NormalizeExact();
        float t1 = AZ::g_fltMax;
        float t2 = AZ::g_fltMax;
        int hits = Intersect::IntersectRayCappedCylinder(rayOrigin, rayDir, m_cylinderEnd1, m_cylinderDir, m_cylinderHeight, m_cylinderRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    class MATH_IntersectRayConeTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_coneApex = Vector3(1.264f, 6.773f, 9.612f);
            m_coneDir = Vector3(-64.0f, -82.783f, -12.97f);
            m_radiusDir = Vector3(m_coneDir.GetY(), -m_coneDir.GetX(), 0.0f);

            m_coneDir.NormalizeExact();
            m_coneHeight = 1.643f;
            m_coneRadius = m_coneHeight;

            m_radiusDir.NormalizeExact();

            m_tangentDir = m_coneHeight * m_coneDir + m_coneRadius * m_radiusDir;
            m_tangentDir.NormalizeExact();
        }

        void TearDown() override
        {
        }

        Vector3 m_coneApex;
        Vector3 m_coneDir;
        Vector3 m_radiusDir;
        Vector3 m_tangentDir; // direction of tangent line on the surface from the apex to the base
        float m_coneHeight;
        float m_coneRadius;
    };

    TEST_F(MATH_IntersectRayConeTest, RayOriginAtApex_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeBelowApex_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 0.5f * m_coneHeight * m_coneDir - m_coneRadius * m_radiusDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeBelowBase_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 1.5f * m_coneHeight * m_coneDir - m_coneRadius * m_radiusDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeAboveApex_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex - 1.5f * m_coneHeight * m_coneDir - m_coneRadius * m_radiusDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideCone_RayDirParallelConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 0.5f * m_coneHeight * m_coneDir;
        Vector3 rayDir = m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideCone_RayDirParallelConeSurfaceOppositeDirection)
    {
        Vector3 rayOrigin = m_coneApex + 0.5f * m_coneHeight * m_coneDir;
        Vector3 rayDir = -m_tangentDir;
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideCone_RayDirThroughApex)
    {
        Vector3 rayOrigin = m_coneApex + 0.03f * m_coneDir + m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - rayOrigin).GetNormalizedExact();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 1);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideMirrorCone_RayDirThroughApex)
    {
        Vector3 rayOrigin = m_coneApex - 0.5f * m_coneHeight * m_coneDir + 0.25f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - rayOrigin).GetNormalizedExact();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideBase_RayDirThroughApex)
    {
        Vector3 rayOrigin = m_coneApex + 1.3f * m_coneHeight * m_coneDir + 0.3f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - rayOrigin).GetNormalizedExact();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideBase_RayDirThroughConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 1.3f * m_coneHeight * m_coneDir + 0.3f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex - 0.3f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalizedExact();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeApexSide_RayDirThroughConeSurface)
    {
        Vector3 rayOrigin = m_coneApex + 1.5f * m_coneHeight * m_coneDir + 0.7f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex + 0.7f * m_coneHeight * m_coneDir - 0.8f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalizedExact();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 2);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginOutsideConeApexSide_RayDirMissCone)
    {
        Vector3 rayOrigin = m_coneApex - 0.5f * m_coneHeight * m_coneDir + 0.7f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex + 1.1f * m_coneHeight * m_coneDir + 1.1f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalizedExact();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    TEST_F(MATH_IntersectRayConeTest, RayOriginInsideMirrorConeApexSide_RayDirMissCone)
    {
        Vector3 rayOrigin = m_coneApex - 0.7f * m_coneHeight * m_coneDir + 0.6f * m_coneRadius * m_radiusDir;
        Vector3 rayDir = (m_coneApex + 1.5f * m_coneHeight * m_coneDir + 1.5f * m_coneRadius * m_radiusDir - rayOrigin).GetNormalizedExact();
        float t1 = 0.0f;
        float t2 = 0.0f;
        int hits = Intersect::IntersectRayCone(rayOrigin, rayDir, m_coneApex, m_coneDir, m_coneHeight, m_coneRadius, t1, t2);
        EXPECT_EQ(hits, 0);
    }

    class MATH_IntersectRayQuadTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_vertexA = Vector3(1.04f, 2.46f, 5.26f);

            m_axisB = Vector3(1.0f, 2.0f, 3.0f);
            m_axisD = Vector3(3.0f, -2.0f, 1.0f);
            m_axisB.NormalizeExact();
            m_axisD.NormalizeExact();
            m_lengthAxisB = 4.56f;
            m_lengthAxisD = 7.19f;
            m_normal = m_axisB.Cross(m_axisD);
            m_normal.NormalizeExact();

            m_vertexB = m_vertexA + m_lengthAxisB * m_axisB;
            m_vertexC = m_vertexA + m_lengthAxisB * m_axisB + m_lengthAxisD * m_axisD;
            m_vertexD = m_vertexA + m_lengthAxisD * m_axisD;
        }

        void TearDown() override
        {

        }

        Vector3 m_vertexA;
        Vector3 m_vertexB;
        Vector3 m_vertexC;
        Vector3 m_vertexD;

        // two axes defining the quad plane, originating from m_vertexA
        Vector3 m_axisB;
        Vector3 m_axisD;
        float m_lengthAxisB;
        float m_lengthAxisD;

        Vector3 m_normal;
    };

    TEST_F(MATH_IntersectRayQuadTest, RayShootingAway_CCW)
    {
        Vector3 rayOrigin = (m_vertexA + 0.23f * m_axisB + 0.75f * m_axisD) + 2.0f * m_normal;
        Vector3 rayDir = rayOrigin - m_vertexC;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayShootingAway_CW)
    {
        Vector3 rayOrigin = (m_vertexA + 0.23f * m_axisB + 0.75f * m_axisD) + 2.0f * m_normal;
        Vector3 rayDir = rayOrigin - m_vertexC;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexC, m_vertexB, m_vertexA, m_vertexD, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleABC_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexB) - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleABC_CW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexB) - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexD, m_vertexC, m_vertexB, m_vertexA, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleACD_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexD) - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectTriangleACD_CW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = ((0.5f * (0.5f * m_vertexA + 0.5f * m_vertexC)) + 0.5f * m_vertexD) - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexD, m_vertexC, m_vertexB, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayIntersectLineAC_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = (0.5f * m_vertexA + 0.5f * m_vertexC) - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayShootOverAB_CCW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = m_vertexA + 1.7f * m_lengthAxisB * m_axisB - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexB, m_vertexC, m_vertexD, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayQuadTest, RayShootOverAC_CW)
    {
        Vector3 rayOrigin = m_vertexA + 2.0f * m_normal;
        Vector3 rayDir = m_vertexA + 1.3f * (m_vertexD - m_vertexA) - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayQuad(rayOrigin, rayDir, m_vertexA, m_vertexD, m_vertexC, m_vertexB, t);
        EXPECT_EQ(hit, 0);
    }

    class MATH_IntersectRayBoxTest
        : public AllocatorsFixture
    {
    protected:

        void SetUp() override
        {
            m_boxCenter = Vector3(1.234f, 2.345f, 9.824f);
            m_boxAxis1 = Vector3(1.0f, 2.0f, 3.0f);
            m_boxAxis2 = Vector3(-3.0f, 2.0f, -1.0f);
            m_boxAxis3 = m_boxAxis1.Cross(m_boxAxis2);
            m_boxAxis1.NormalizeExact();
            m_boxAxis2.NormalizeExact();
            m_boxAxis3.NormalizeExact();
            m_boxHalfExtentAxis1 = 4.775f;
            m_boxHalfExtentAxis2 = 8.035f;
            m_boxHalfExtentAxis3 = 14.007f;
        }

        void TearDown() override
        {

        }

        Vector3 m_boxCenter;
        Vector3 m_boxAxis1;
        Vector3 m_boxAxis2;
        Vector3 m_boxAxis3;
        float m_boxHalfExtentAxis1;
        float m_boxHalfExtentAxis2;
        float m_boxHalfExtentAxis3;
    };

    TEST_F(MATH_IntersectRayBoxTest, RayOriginOutside_HitBoxAxis1Side)
    {
        Vector3 rayOrigin = m_boxCenter + 2.0f * m_boxHalfExtentAxis1 * m_boxAxis1 + 2.0f * m_boxHalfExtentAxis2 * m_boxAxis2;
        Vector3 rayDir = m_boxCenter + m_boxHalfExtentAxis1 * m_boxAxis1 - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 1);
    }

    TEST_F(MATH_IntersectRayBoxTest, RayOriginOutside_ShootAwayFromBox)
    {
        Vector3 rayOrigin = m_boxCenter + 2.0f * m_boxHalfExtentAxis3 * m_boxAxis3 + 2.0f * m_boxHalfExtentAxis2 * m_boxAxis2;
        Vector3 rayDir = rayOrigin - m_boxCenter + m_boxHalfExtentAxis3 * m_boxAxis3;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayBoxTest, RayOriginOutside_RayParallelToAxis2MissBox)
    {
        Vector3 rayOrigin = m_boxCenter + 2.0f * m_boxHalfExtentAxis3 * m_boxAxis3 + 2.0f * m_boxHalfExtentAxis2 * m_boxAxis2;
        Vector3 rayDir = m_boxAxis2;
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 0);
    }

    TEST_F(MATH_IntersectRayBoxTest, RayOriginInside_ShootToConner)
    {
        Vector3 rayOrigin = m_boxCenter + 0.5f * m_boxHalfExtentAxis1 * m_boxAxis1 +
            0.5f * m_boxHalfExtentAxis2 * m_boxAxis2 + 0.5f * m_boxHalfExtentAxis3 * m_boxAxis3;
        Vector3 rayDir = (m_boxCenter - m_boxHalfExtentAxis1 * m_boxAxis1 -
            m_boxHalfExtentAxis2 * m_boxAxis2 - m_boxHalfExtentAxis3 * m_boxAxis3) - rayOrigin;
        rayDir.NormalizeExact();
        float t = 0.0f;
        int hit = Intersect::IntersectRayBox(rayOrigin, rayDir, m_boxCenter, m_boxAxis1, m_boxAxis2, m_boxAxis3,
            m_boxHalfExtentAxis1, m_boxHalfExtentAxis2, m_boxHalfExtentAxis3, t);
        EXPECT_EQ(hit, 1);
    }

    class MATH_IntersectRayPolyhedronTest
        : public AllocatorsFixture
    {
    protected:
        void SetUp() override
        {
            // base of cube
            m_vertices[0] = Vector3(0.0f, 0.0f, 0.0f);
            m_vertices[1] = Vector3(10.0f, 0.0f, 0.0f);
            m_vertices[2] = Vector3(10.0f, 10.0f, 0.0f);
            m_vertices[3] = Vector3(0.0f, 10.0f, 0.0f);

            // setup planes
            for (size_t i = 0; i < 4; ++i)
            {
                const Vector3 start = m_vertices[i];
                const Vector3 end = m_vertices[(i + 1) % 4];
                const Vector3 top = start + Vector3::CreateAxisZ();

                const Vector3 normal = (end - start).Cross(top - start).GetNormalizedSafe();

                m_planes[i] = Plane::CreateFromNormalAndPoint(normal, start);
            }

            const Vector3 normalTop = 
                (m_vertices[2] - m_vertices[0]).Cross(m_vertices[0] - m_vertices[1]).GetNormalizedSafe();
            const Vector3 normalBottom = -normalTop;

            const float height = 10.0f;
            m_planes[4] = Plane::CreateFromNormalAndPoint(normalTop, m_vertices[0] + Vector3::CreateAxisZ(height));
            m_planes[5] = Plane::CreateFromNormalAndPoint(normalBottom, m_vertices[0]);
        }

        void TearDown() override
        {
        }

        Vector3 m_vertices[4];
        Plane m_planes[6];
    };

    TEST_F(MATH_IntersectRayPolyhedronTest, RayParallelHit)
    {
        const Vector3 src = Vector3(0.0f, -1.0f, 1.0f);
        const Vector3 dir = Vector3(0.0f, 1.0f, 0.0f);
        const Vector3 end = (src + dir * VectorFloat(100.0f)) - src;

        VectorFloat f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 1);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayAboveMiss)
    {
        const Vector3 src = Vector3(5.0f, 11.0f, 11.0f);
        const Vector3 dir = Vector3(0.0f, -1.0f, 0.0f);
        const Vector3 end = (src + dir * VectorFloat(100.0f)) - src;

        VectorFloat f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 0);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayDiagonalDownHit)
    {
        const Vector3 src = Vector3(5.0f, -1.0f, 11.0f);
        const Vector3 end = Vector3(5.0f, 11.0f, -11.0f);

        VectorFloat f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 1);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayDiagonalAcrossHit)
    {
        const Vector3 src = Vector3(-5.0f, -5.0f, 5.0f);
        const Vector3 end = Vector3(15.0f, 15.0f, 5.0f);

        VectorFloat f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 1);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayDiagonalAcrossMiss)
    {
        const Vector3 src = Vector3(-5.0f, -15.0f, 5.0f);
        const Vector3 end = Vector3(15.0f, 5.0f, 5.0f);

        VectorFloat f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 0);
    }

    TEST_F(MATH_IntersectRayPolyhedronTest, RayStartInside)
    {
        const Vector3 src = Vector3(5.0f, 5.0f, 5.0f);
        const Vector3 end = Vector3(5.0f, 5.0f, 5.0f);

        VectorFloat f, l;
        int firstPlane, lastPlane;
        const int intersections = Intersect::IntersectSegmentPolyhedron(src, end, m_planes, 6, f, l, firstPlane, lastPlane);

        EXPECT_EQ(intersections, 0);
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
#if defined(AZ_PLATFORM_APPLE_OSX)
        // This test blocks the execution on Mac, forcing to fail until it gets properly fixed.
        EXPECT_TRUE(false);
        return;
#endif

        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
            {
                for (int i = 0; i < 10000; ++i)
                {
                    sfmt.Rand32();
                }
            };
        AZStd::thread threads[8];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
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
#if defined(AZ_PLATFORM_APPLE_OSX)
        // This test blocks the execution on Mac, forcing to fail until it gets properly fixed.
        EXPECT_TRUE(false);
        return;
#endif

        Sfmt sfmt;
        auto threadFunc = [&sfmt]()
            {
                for (int i = 0; i < 10000; ++i)
                {
                    sfmt.Rand64();
                }
            };
        AZStd::thread threads[8];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
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
#if defined(AZ_PLATFORM_APPLE_OSX)
        // This test blocks the execution on Mac, forcing to fail until it gets properly fixed.
        EXPECT_TRUE(false);
        return;
#endif

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
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(threads); ++threadIdx)
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

    TEST(MATH_Color, Construction)
    {
        // Default constructor
        Color colorDefault;
        colorDefault.SetR(1.0f);
        EXPECT_EQ(colorDefault.GetR(), VectorFloat(1.0f));

        // Vector4 constructor
        const Vector4 vectorColor(0.5f, 0.6f, 0.7f, 1.0f);
        const Color colorFromVector(vectorColor);
        EXPECT_TRUE(colorFromVector.GetAsVector4().IsClose(vectorColor));

        // Single VectorFloat constructor
        const VectorFloat vfOne = 1.0f;
        const Color colorFromVectorFloat(vfOne);
        EXPECT_TRUE(colorFromVectorFloat.GetAsVector4().IsClose(Vector4(1.0f, 1.0f, 1.0f, 1.0f)));

        // Four individual VectorFloats constructor
        const VectorFloat vfHalf = 0.5f;
        const Color colorFrom4VectorFloats(vfHalf, vfHalf, vfHalf, vfOne);
        EXPECT_TRUE(colorFrom4VectorFloats.GetAsVector4().IsClose(Vector4(0.5f, 0.5f, 0.5f, 1.0f)));

        // Four individual floats constructor
        const Color colorFromFloats(0.5f, 0.6f, 0.7f, 1.0f);
        EXPECT_TRUE(colorFromFloats.GetAsVector4().IsClose(vectorColor));

        // Four individual uint8s constructor
        const Color colorFronUints((u8)0x7F, (u8)0x9F, (u8)0xBF, (u8)0xFF);
        EXPECT_EQ(colorFronUints.ToU32(), 0xFFBF9F7F);

    }

    TEST(MATH_Color, StaticConstruction)
    {
        // Zero color
        const Color colorZero = Color::CreateZero();
        EXPECT_EQ(colorZero.GetAsVector4(), Vector4(0.0f, 0.0f, 0.0f, 0.0f));

        // OneColor
        const Color colorOne = Color::CreateOne();
        EXPECT_EQ(colorOne.GetAsVector4(), Vector4(1.0f, 1.0f, 1.0f, 1.0f));

        // From Float4
        const float float4Color[4] = { 0.3f, 0.4f, 0.5f, 0.8f };
        const Color colorFrom4Floats = Color::CreateFromFloat4(float4Color);
        EXPECT_EQ(colorFrom4Floats.GetAsVector4(), Vector4(0.3f, 0.4f, 0.5f, 0.8f));

        // From Vector3
        const Vector3 vector3Color(0.6f, 0.7f, 0.8f);
        const Color colorFromVector3 = Color::CreateFromVector3(vector3Color);
        EXPECT_EQ(colorFromVector3.GetAsVector4(), Vector4(0.6f, 0.7f, 0.8f, 1.0f));

        // From Vector 3 and float
        const Color colorFromVector3AndFloat = Color::CreateFromVector3AndFloat(vector3Color, 0.5f);
        EXPECT_EQ(colorFromVector3AndFloat.GetAsVector4(), Vector4(0.6f, 0.7f, 0.8f, 0.5f));
    }

    TEST(MATH_Color, Store)
    {
        // Store color values to float array.
        float dest[4];
        const Color colorOne = Color::CreateOne();
        colorOne.StoreToFloat4(dest);
        EXPECT_EQ(dest[0], 1.0f);
        EXPECT_EQ(dest[1], 1.0f);
        EXPECT_EQ(dest[2], 1.0f);
        EXPECT_EQ(dest[3], 1.0f);
    }

    TEST(MATH_Color, ComponentAccess)
    {
        Color testColor;

        // Float Get / Set
        testColor.SetR(0.4f);
        EXPECT_EQ(testColor.GetR(), VectorFloat(0.4f));
        testColor.SetG(0.3f);
        EXPECT_EQ(testColor.GetG(), VectorFloat(0.3f));
        testColor.SetB(0.2f);
        EXPECT_EQ(testColor.GetB(), VectorFloat(0.2f));
        testColor.SetA(0.1f);
        EXPECT_EQ(testColor.GetA(), VectorFloat(0.1f));

        // u8 Get / Set
        testColor.SetR8((u8)0x12);
        EXPECT_EQ(testColor.GetR8(), 0x12);
        testColor.SetG8((u8)0x34);
        EXPECT_EQ(testColor.GetG8(), 0x34);
        testColor.SetB8((u8)0x56);
        EXPECT_EQ(testColor.GetB8(), 0x56);
        testColor.SetA8((u8)0x78);
        EXPECT_EQ(testColor.GetA8(), 0x78);

        // Index-based element access
        testColor = Color(0.3f, 0.4f, 0.5f, 0.7f);
        EXPECT_EQ(testColor.GetElement(0), VectorFloat(0.3f));
        EXPECT_EQ(testColor.GetElement(1), VectorFloat(0.4f));
        EXPECT_EQ(testColor.GetElement(2), VectorFloat(0.5f));
        EXPECT_EQ(testColor.GetElement(3), VectorFloat(0.7f));

        testColor.SetElement(0, 0.7f);
        EXPECT_EQ(testColor.GetR(), VectorFloat(0.7f));
        testColor.SetElement(1, 0.5f);
        EXPECT_EQ(testColor.GetG(), VectorFloat(0.5f));
        testColor.SetElement(2, 0.4f);
        EXPECT_EQ(testColor.GetB(), VectorFloat(0.4f));
        testColor.SetElement(3, 0.3f);
        EXPECT_EQ(testColor.GetA(), VectorFloat(0.3f));
    }

    TEST(MATH_Color, SettersGetters)
    {
        // Vector3 getter
        const Color vectorColor = Color(0.3f, 0.4f, 0.5f, 0.7f);
        EXPECT_EQ(vectorColor.GetAsVector3(), Vector3(0.3f, 0.4f, 0.5f));

        // Vector 4 getter
        EXPECT_EQ(vectorColor.GetAsVector4(), Vector4(0.3f, 0.4f, 0.5f, 0.7f));

        // Set from single VectorFloat
        Color singleValue;
        singleValue.Set(0.75f);
        EXPECT_EQ(singleValue.GetAsVector4(), Vector4(0.75f, 0.75f, 0.75f, 0.75f));

        // Set from 4 VectorFloats
        Color separateValues;
        separateValues.Set(0.23f, 0.45f, 0.67f, 0.89f);
        EXPECT_EQ(separateValues.GetAsVector4(), Vector4(0.23f, 0.45f, 0.67f, 0.89f));

        // Set from a float[4]
        float floatArray[4] = { 0.87f, 0.65f, 0.43f, 0.21f };
        Color floatArrayColor;
        floatArrayColor.Set(floatArray);
        EXPECT_EQ(floatArrayColor.GetAsVector4(), Vector4(0.87f, 0.65f, 0.43f, 0.21f));

        // Set from Vector3, Alpha should be set to 1.0f
        Color vector3Color;
        vector3Color.Set(Vector3(0.2f, 0.4f, 0.6f));
        EXPECT_EQ(vector3Color.GetAsVector4(), Vector4(0.2f, 0.4f, 0.6f, 1.0f));

        // Set from Vector3 +_ alpha
        Color vector4Color;
        vector4Color.Set(Vector3(0.1f, 0.3f, 0.5f), 0.7f);
        EXPECT_EQ(vector4Color.GetAsVector4(), Vector4(0.1f, 0.3f, 0.5f, 0.7f));
        
        // Oddly lacking a Set() from Vector4...

    }

    TEST(MATH_Color, HueSaturationValue)
    {
        Color fromHSV(0.0f, 0.0f, 0.0f, 1.0f);

        // Check first sexant (0-60 degrees) with 0 hue, full saturation and value = red.
        fromHSV.SetFromHSVRadians(0.0f, 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(1.0f, 0.0f, 0.0f, 1.0f)));
        
        // Check the second sexant (60-120 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(72.0f), 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.8f, 1.0f, 0.0f, 1.0f)));

        // Check the third sexant (120-180 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(144.0f), 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.0f, 1.0f, 0.4f, 1.0f)));

        // Check the fourth sexant (180-240 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(216.0f), 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.0f, 0.4f, 1.0f, 1.0f)));

        // Check the fifth sexant (240-300 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(252.0f), 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.2f, 0.0f, 1.0f, 1.0f)));

        // Check the sixth sexant (300-360 degrees)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check the upper limit of the hue
        fromHSV.SetFromHSVRadians(AZ::Constants::TwoPi, 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(1.0f, 0.0f, 0.0f, 1.0f)));
        
        // Check that zero saturation causes RGB to all be value.
        fromHSV.SetFromHSVRadians(AZ::DegToRad(90.0f), 0.0f, 0.75f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.75f, 0.75f, 0.75f, 1.0f)));

        // Check that zero value causes the color to be black.
        fromHSV.SetFromHSVRadians(AZ::DegToRad(180.0f), 1.0f, 0.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.0f, 0.0f, 0.0f, 1.0f)));

        // Check a non-zero, non-one saturation
        fromHSV.SetFromHSVRadians(AZ::DegToRad(252.0f), 0.5f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.6f, 0.5f, 1.0f, 1.0f)));

        // Check a non-zero, non-one value
        fromHSV.SetFromHSVRadians(AZ::DegToRad(216.0f), 1.0f, 0.5f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.0f, 0.2f, 0.5f, 1.0f)));

        // Check a non-zero, non-one value and saturation
        fromHSV.SetFromHSVRadians(AZ::DegToRad(144.0f), 0.25f, 0.75f);
        EXPECT_TRUE(fromHSV.IsClose(Color(143.44f/255.0f, 191.25f/255.0f, 162.56f/255.0f, 1.0f)));

        // Check that negative hue is handled correctly (only fractional value, +1 to be positive)
        fromHSV.SetFromHSVRadians(AZ::DegToRad(-396.0f), 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check that negative saturation is clamped to 0
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), -1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(1.0f, 1.0f, 1.0f, 1.0f)));

        // Check that negative value is clamped to 0
        fromHSV.SetFromHSVRadians(AZ::Constants::Pi, 1.0f, -1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.0f, 0.0f, 0.0f, 1.0f)));

        // Check that > 1 saturation is clamped to 1
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), 2.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check that > 1 value is clamped to 1
        fromHSV.SetFromHSVRadians(AZ::DegToRad(324.0f), 1.0f, 2.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(1.0f, 0.0f, 0.6f, 1.0f)));

        // Check a large hue.
        fromHSV.SetFromHSVRadians(AZ::DegToRad(3744.0f), 1.0f, 1.0f);
        EXPECT_TRUE(fromHSV.IsClose(Color(0.0f, 1.0f, 0.4f, 1.0f)));
    }

    TEST(MATH_Color, EqualityComparisons)
    {
        // Equality within tolerance
        Color color1(0.1f, 0.2f, 0.3f, 0.4f);
        Color color2(0.1f, 0.2f, 0.3f, 0.4f);
        Color color3(0.12f, 0.22f, 0.32f, 0.42f);

        EXPECT_TRUE(color1.IsClose(color2));
        EXPECT_FALSE(color1.IsClose(color3));
        EXPECT_TRUE(color1.IsClose(color3, 0.03f));
        EXPECT_FALSE(color1.IsClose(color3, 0.01f));

        // Zero check within tolerance
        Color zeroColor = Color::CreateZero();
        EXPECT_TRUE(zeroColor.IsZero());
        Color almostZeroColor(0.001f, 0.001f, 0.001f, 0.001f);
        EXPECT_FALSE(almostZeroColor.IsZero());
        EXPECT_TRUE(almostZeroColor.IsZero(0.01f));

        // Strict equality
        EXPECT_TRUE(color1 == color2);
        EXPECT_FALSE(color1 == color3);

        // Strict inequality
        EXPECT_FALSE(color1 != color2);
        EXPECT_TRUE(color1 != color3);
    }

    TEST(MATH_Color, LessThanComparisons)
    {
        Color color1(0.3f, 0.6f, 0.8f, 1.0f);
        Color color2(0.2f, 0.5f, 0.7f, 1.0f);
        Color color3(0.2f, 0.5f, 0.7f, 0.9f);
        Color color4(0.8f, 0.4f, 0.3f, 0.8f);

        // color 1 and two have an equal component so should not be strictly less than each other
        EXPECT_FALSE(color1.IsLessThan(color2));
        EXPECT_FALSE(color2.IsLessThan(color1));

        // color 3 should be strictly less than color 1, but not color 2
        EXPECT_TRUE(color3.IsLessThan(color1));
        EXPECT_FALSE(color3.IsLessThan(color2));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsLessThan(color1));
        EXPECT_FALSE(color4.IsLessThan(color2));
        EXPECT_FALSE(color4.IsLessThan(color3));
        EXPECT_FALSE(color1.IsLessThan(color4));
        EXPECT_FALSE(color2.IsLessThan(color4));
        EXPECT_FALSE(color3.IsLessThan(color4));

        // color 1 and two have an equal component but otherwise color 2 is less than color 1
        EXPECT_FALSE(color1.IsLessEqualThan(color2));
        EXPECT_TRUE(color2.IsLessEqualThan(color1));

        // color 3 should be less than or equal to both color 1 and color 2
        EXPECT_TRUE(color3.IsLessEqualThan(color1));
        EXPECT_TRUE(color3.IsLessEqualThan(color2));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsLessEqualThan(color1));
        EXPECT_FALSE(color4.IsLessEqualThan(color2));
        EXPECT_FALSE(color4.IsLessEqualThan(color3));
        EXPECT_FALSE(color1.IsLessEqualThan(color4));
        EXPECT_FALSE(color2.IsLessEqualThan(color4));
        EXPECT_FALSE(color3.IsLessEqualThan(color4));
    }

    TEST(MATH_Color, GreaterThanComparisons)
    {
        Color color1(0.3f, 0.6f, 0.8f, 1.0f);
        Color color2(0.2f, 0.5f, 0.7f, 1.0f);
        Color color3(0.2f, 0.5f, 0.7f, 0.9f);
        Color color4(0.8f, 0.4f, 0.3f, 0.8f);

        // color 1 and two have an equal component so should not be strictly greater than each other
        EXPECT_FALSE(color1.IsGreaterThan(color2));
        EXPECT_FALSE(color2.IsGreaterThan(color1));

        // color 1 should be strictly greater than color 3, but color 2 shouldn't
        EXPECT_TRUE(color1.IsGreaterThan(color3));
        EXPECT_FALSE(color2.IsGreaterThan(color3));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsGreaterThan(color1));
        EXPECT_FALSE(color4.IsGreaterThan(color2));
        EXPECT_FALSE(color4.IsGreaterThan(color3));
        EXPECT_FALSE(color1.IsGreaterThan(color4));
        EXPECT_FALSE(color2.IsGreaterThan(color4));
        EXPECT_FALSE(color3.IsGreaterThan(color4));

        // color 1 and two have an equal component but otherwise color 2 is less than color 1
        EXPECT_TRUE(color1.IsGreaterEqualThan(color2));
        EXPECT_FALSE(color2.IsGreaterEqualThan(color1));

        // color 1 and 2 should both be greater than or equal to color 3
        EXPECT_TRUE(color1.IsGreaterEqualThan(color3));
        EXPECT_TRUE(color2.IsGreaterEqualThan(color3));

        // color 4 has values higher and lower than other colors so it should always fail
        EXPECT_FALSE(color4.IsGreaterEqualThan(color1));
        EXPECT_FALSE(color4.IsGreaterEqualThan(color2));
        EXPECT_FALSE(color4.IsGreaterEqualThan(color3));
        EXPECT_FALSE(color1.IsGreaterEqualThan(color4));
        EXPECT_FALSE(color2.IsGreaterEqualThan(color4));
        EXPECT_FALSE(color3.IsGreaterEqualThan(color4));
    }

    TEST(MATH_Color, VectorConversions)
    {
        Vector3 vec3FromColor(Color(0.4f, 0.6f, 0.8f, 1.0f));
        EXPECT_EQ(vec3FromColor, Vector3(0.4f, 0.6f, 0.8f));

        Vector4 vec4FromColor(Color(0.4f, 0.6f, 0.8f, 1.0f));
        EXPECT_EQ(vec4FromColor, Vector4(0.4f, 0.6f, 0.8f, 1.0f));

        Color ColorfromVec3;
        ColorfromVec3 = Vector3(0.3f, 0.4f, 0.5f);
        EXPECT_EQ(ColorfromVec3.GetAsVector4(), Vector4(0.3f, 0.4f, 0.5f, 1.0f));
    }

    TEST(MATH_Color, LinearToGamma)
    {
        // Very dark values (these are converted differently)
        Color reallyDarkLinear(0.001234f, 0.000123f, 0.001010f, 0.5f);
        Color reallyDarkGamma = reallyDarkLinear.LinearToGamma();
        Color reallyDarkGammaCheck(0.01594328f, 0.00158916f, 0.0130492f, 0.5f);
        EXPECT_TRUE(reallyDarkGamma.IsClose(reallyDarkGammaCheck, 0.00001f));

        // Normal values
        Color normalLinear(0.123456f, 0.345678f, 0.567890f, 0.8f);
        Color normalGamma = normalLinear.LinearToGamma();
        Color normalGammaCheck(0.386281658744f, 0.622691988496f, 0.77841737783f, 0.8f);
        EXPECT_TRUE(normalGamma.IsClose(normalGammaCheck, 0.00001f));

        // Bright values
        Color brightLinear(1.234567f, 3.456789f, 5.678901f, 1.0f);
        Color brightGamma = brightLinear.LinearToGamma();
        Color brightGammaCheck(1.09681722689f, 1.71388455271f, 2.12035054203f, 1.0f);
        EXPECT_TRUE(brightGamma.IsClose(brightGammaCheck, 0.00001f));

        // Zero should stay the same
        Color zeroColor = Color::CreateZero();
        Color zeroColorGamma = zeroColor.LinearToGamma();
        EXPECT_TRUE(zeroColorGamma.IsClose(zeroColor, 0.00001f));

        // One should stay the same
        Color oneColor = Color::CreateOne();
        Color oneColorGamma = oneColor.LinearToGamma();
        EXPECT_TRUE(oneColorGamma.IsClose(oneColor, 0.00001f));
    }

    TEST(MATH_Color, GammaToLinear)
    {
        // Very dark values (these are converted differently)
        Color reallyDarkGamma(0.001234f, 0.000123f, 0.001010f, 0.5f);
        Color reallyDarkLinear = reallyDarkGamma.GammaToLinear();
        Color reallyDarkLinearCheck(0.0000955108359133f, 0.00000952012383901f, 0.000078173374613f, 0.5f);
        EXPECT_TRUE(reallyDarkLinear.IsClose(reallyDarkLinearCheck, 0.00001f));

        // Normal values
        Color normalGamma(0.123456f, 0.345678f, 0.567890f, 0.8f);
        Color normalLinear = normalGamma.GammaToLinear();
        Color normalLinearCheck(0.0140562303977f, 0.097927189487f, 0.282345816828f, 0.8f);
        EXPECT_TRUE(normalLinear.IsClose(normalLinearCheck, 0.00001f));

        // Bright values
        Color brightGamma(1.234567f, 3.456789f, 5.678901f, 1.0f);
        Color brightLinear = brightGamma.GammaToLinear();
        Color brightLinearCheck(1.61904710087f, 17.9251290437f, 58.1399365547f, 1.0f);
        EXPECT_TRUE(brightLinear.IsClose(brightLinearCheck, 0.00001f));

        // Zero should stay the same
        Color zeroColor = Color::CreateZero();
        Color zeroColorLinear = zeroColor.GammaToLinear();
        EXPECT_TRUE(zeroColorLinear.IsClose(zeroColor, 0.00001f));

        // One should stay the same
        Color oneColor = Color::CreateOne();
        Color oneColorLinear = oneColor.GammaToLinear();
        EXPECT_TRUE(oneColorLinear.IsClose(oneColor, 0.00001f));
    }

    TEST(MATH_Color, UintConversions)
    {
        // Convert to u32, floats expected to floor.
        Color colorToU32(0.23f, 0.55f, 0.88f, 1.0f);
        EXPECT_EQ(colorToU32.ToU32(), 0xFFE08C3A);

        // Convert from u32
        Color colorFromU32;
        colorFromU32.FromU32(0xFFE08C3A);
        EXPECT_TRUE(colorFromU32.IsClose(Color(0.22745098039f, 0.549019607843f, 0.87843137254f, 1.0f), 0.00001f));

        // Convert to u32 and change to gamma space at the same time.
        Color colorToU32Gamma(0.23f, 0.55f, 0.88f, 0.5f);
        EXPECT_EQ(colorToU32Gamma.ToU32LinearToGamma(), 0x7FF1C383);

        // Convert from u32 and change to linear space at the same time.
        Color colorFromU32Gamma;
        colorFromU32Gamma.FromU32GammaToLinear(0x7FF1C383);
        EXPECT_TRUE(colorFromU32Gamma.IsClose(Color(0.22696587351f, 0.54572446137f, 0.879622396888f, 0.498039215686f), 0.00001f));

    }

    TEST(MATH_Color, DotProduct)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color color3(2.5f, 1.7f, 5.3f, 2.8f);

        EXPECT_NEAR(color1.Dot(color2), 0.75f, 0.0001f);
        EXPECT_NEAR(color1.Dot(color3), 4.05f, 0.0001f);
        EXPECT_NEAR(color2.Dot(color3), 6.27f, 0.0001f);

        EXPECT_NEAR(color1.Dot3(color2), 0.67f, 0.0001f);
        EXPECT_NEAR(color1.Dot3(color3), 3.77f, 0.0001f);
        EXPECT_NEAR(color2.Dot3(color3), 4.03f, 0.0001f);
    }

    TEST(MATH_Color, Addition)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color colorSum(1.1f, 1.1f, 0.6f, 0.9f);

        EXPECT_TRUE(colorSum.IsClose(color1 + color2, 0.0001f));

        color1 += color2;
        EXPECT_TRUE(colorSum.IsClose(color1, 0.0001f));
    }

    TEST(MATH_Color, Subtraction)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color colorDiff(0.1f, -0.3f, 0.0f, -0.7f);

        EXPECT_TRUE(colorDiff.IsClose(color1 - color2, 0.0001f));

        color1 -= color2;
        EXPECT_TRUE(colorDiff.IsClose(color1, 0.0001f));
    }

    TEST(MATH_Color, Multiplication)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.7f, 0.3f, 0.8f);
        Color colorProduct(0.3f, 0.28f, 0.09f, 0.08f);
        Color color2Double(1.0f, 1.4f, 0.6f, 1.6f);

        // Product of two colors
        EXPECT_TRUE(colorProduct.IsClose(color1 * color2, 0.0001f));

        // Multiply-assignment
        color1 *= color2;
        EXPECT_TRUE(colorProduct.IsClose(color1, 0.0001f));

        // Product of color and float
        EXPECT_TRUE(color2Double.IsClose(color2 * 2.0f));

        // Multiply-assignment with single float
        color2 *= 2.0f;
        EXPECT_TRUE(color2Double.IsClose(color2));
    }

    TEST(MATH_Color, Division)
    {
        Color color1(0.6f, 0.4f, 0.3f, 0.1f);
        Color color2(0.5f, 0.8f, 0.3f, 0.8f);
        Color colorQuotient(1.2f, 0.5f, 1.0f, 0.125f);
        Color color2Half(0.25f, 0.4f, 0.15f, 0.4f);

        // Product of two colors
        EXPECT_TRUE(colorQuotient.IsClose(color1 / color2, 0.0001f));

        // Multiply-assignment
        color1 /= color2;
        EXPECT_TRUE(colorQuotient.IsClose(color1, 0.0001f));

        // Product of color and float
        EXPECT_TRUE(color2Half.IsClose(color2 / 2.0f));

        // Multiply-assignment with single float
        color2 /= 2.0f;
        EXPECT_TRUE(color2Half.IsClose(color2));
    }


    TEST(MATH_Lerp, Test)
    {
        // Float
        EXPECT_EQ(2.5f, AZ::Lerp(2.0f, 4.0f, 0.25f));
        EXPECT_EQ(6.0f, AZ::Lerp(2.0f, 4.0f, 2.0f));

        // Double
        EXPECT_EQ(3.5, AZ::Lerp(2.0, 4.0, 0.75));
        EXPECT_EQ(0.0, AZ::Lerp(2.0, 4.0, -1.0));
    }

    TEST(MATH_LerpInverse, Test)
    {
        // Float
        EXPECT_NEAR(0.25f, AZ::LerpInverse(2.0f, 4.0f, 2.5f), 0.0001f);
        EXPECT_NEAR(2.0f, AZ::LerpInverse(2.0f, 4.0f, 6.0f), 0.0001f);

        // Double
        EXPECT_NEAR(0.75, AZ::LerpInverse(2.0, 4.0, 3.5), 0.0001);
        EXPECT_NEAR(-1.0, AZ::LerpInverse(2.0, 4.0, 0.0), 0.0001);

        // min/max need to be substantially different to return a useful t value
        
        // Float
        const float epsilonF = std::numeric_limits<float>::epsilon();
        const float doesntMatterF = std::numeric_limits<float>::signaling_NaN();
        // volatile keyword required here to prevent spurious vs2013 compile error about division by 0
        volatile float lowerF = 2.3f, upperF = 2.3f;
        EXPECT_EQ(0.0f, AZ::LerpInverse(lowerF, upperF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 0.5f * epsilonF, doesntMatterF));
        EXPECT_EQ(0.0f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 0.0f));
        EXPECT_NEAR(0.4f, AZ::LerpInverse(0.0f, 5.0f * epsilonF, 2.0f * epsilonF), epsilonF);
        EXPECT_NEAR(0.6f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 3.0f * epsilonF), epsilonF);
        EXPECT_NEAR(1.0f, AZ::LerpInverse(1.0f, 1.0f + 5.0f * epsilonF, 1.0f + 5.0f * epsilonF), epsilonF);

        // Double
        const double epsilonD = std::numeric_limits<double>::epsilon();
        const double doesntMatterD = std::numeric_limits<double>::signaling_NaN();
        // volatile keyword required here to prevent spurious vs2013 compile error about division by 0
        volatile double lowerD = 2.3, upperD = 2.3;
        EXPECT_EQ(0.0, AZ::LerpInverse(lowerD, upperD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 0.5 * epsilonD, doesntMatterD));
        EXPECT_EQ(0.0, AZ::LerpInverse(0.0, 5.0 * epsilonD, 0.0));
        EXPECT_NEAR(0.4, AZ::LerpInverse(0.0, 5.0 * epsilonD, 2.0 * epsilonD), epsilonD);
        EXPECT_NEAR(0.6, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 3.0 * epsilonD), epsilonD);
        EXPECT_NEAR(1.0, AZ::LerpInverse(1.0, 1.0 + 5.0 * epsilonD, 1.0 + 5.0 * epsilonD), epsilonD);
    }
}