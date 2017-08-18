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

#include <math.h>
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    AZ_MATH_FORCE_INLINE VectorFloat::VectorFloat(float value)
        : m_value(_mm_set_ps1(value))    { }
    AZ_MATH_FORCE_INLINE VectorFloat::VectorFloat(const SimdVectorType& value)
        : m_value(value) { }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateZero()            { return VectorFloat(_mm_setzero_ps()); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateOne()         { return VectorFloat(*(SimdVectorType*)&Internal::g_simd1111); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateFromFloat(const float* ptr)
    {
        return VectorFloat(_mm_load_ps1(ptr));
    }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateSelectCmpEqual(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB)
    {
        SimdVectorType mask = _mm_cmpeq_ps(cmp1.m_value, cmp2.m_value);
        return VectorFloat(_mm_or_ps(_mm_and_ps(mask, vA.m_value), _mm_andnot_ps(mask, vB.m_value)));
    }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateSelectCmpGreaterEqual(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB)
    {
        SimdVectorType mask = _mm_cmpge_ps(cmp1.m_value, cmp2.m_value);
        return VectorFloat(_mm_or_ps(_mm_and_ps(mask, vA.m_value), _mm_andnot_ps(mask, vB.m_value)));
    }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::CreateSelectCmpGreater(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB)
    {
        SimdVectorType mask = _mm_cmpgt_ps(cmp1.m_value, cmp2.m_value);
        return VectorFloat(_mm_or_ps(_mm_and_ps(mask, vA.m_value), _mm_andnot_ps(mask, vB.m_value)));
    }
    AZ_MATH_FORCE_INLINE VectorFloat::operator float() const                    { return *reinterpret_cast<const float*>(&m_value); }

    AZ_MATH_FORCE_INLINE void VectorFloat::StoreToFloat(float* ptr) const
    {
        _mm_store_ss(ptr, m_value);
    }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator-() const                       { return VectorFloat(_mm_sub_ps(_mm_setzero_ps(), m_value)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator+(const VectorFloat& rhs) const { return VectorFloat(_mm_add_ps(m_value, rhs.m_value)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator-(const VectorFloat& rhs) const { return VectorFloat(_mm_sub_ps(m_value, rhs.m_value)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator*(const VectorFloat& rhs) const { return VectorFloat(_mm_mul_ps(m_value, rhs.m_value)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::operator/(const VectorFloat& rhs) const { return VectorFloat(_mm_div_ps(m_value, rhs.m_value)); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetMin(const VectorFloat& v) const      { return VectorFloat(_mm_min_ps(m_value, v.m_value)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetMax(const VectorFloat& v) const      { return VectorFloat(_mm_max_ps(m_value, v.m_value)); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetAngleMod() const
    {
        //converting to float, don't have sse instructions that can do this...
        float x = *this;
        float result = (x >= 0.0f) ? (fmodf(x + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(x - Constants::Pi, Constants::TwoPi) + Constants::Pi);
        return VectorFloat(result);
    }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSin() const
    {
        //can probably improve this... calculating all 4 components, we only need 1...
        SimdVectorType x = GetAngleMod().m_value;
        SimdVectorType x2 = _mm_mul_ps(x, x);
        SimdVectorType x3 = _mm_mul_ps(x, x2);
        SimdVectorType x5 = _mm_mul_ps(x3, x2);
        SimdVectorType x7 = _mm_mul_ps(x5, x2);
        SimdVectorType coefficients = *(const SimdVectorType*)&Internal::g_sinEstimateCoefficients;
        SimdVectorType c1 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(1, 1, 1, 1));
        SimdVectorType c2 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(2, 2, 2, 2));
        SimdVectorType c3 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(3, 3, 3, 3));
        return VectorFloat(_mm_add_ps(_mm_mul_ps(c3, x7), _mm_add_ps(_mm_mul_ps(c2, x5), _mm_add_ps(_mm_mul_ps(c1, x3), x))));
    }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetCos() const
    {
        //can probably improve this... calculating all 4 components, we only need 1...
        SimdVectorType x = GetAngleMod().m_value;
        SimdVectorType x2 = _mm_mul_ps(x, x);
        SimdVectorType x4 = _mm_mul_ps(x2, x2);
        SimdVectorType x6 = _mm_mul_ps(x4, x2);
        SimdVectorType coefficients = *(const SimdVectorType*)&Internal::g_cosEstimateCoefficients;
        SimdVectorType c0 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(0, 0, 0, 0));
        SimdVectorType c1 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(1, 1, 1, 1));
        SimdVectorType c2 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(2, 2, 2, 2));
        SimdVectorType c3 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(3, 3, 3, 3));
        return VectorFloat(_mm_add_ps(_mm_mul_ps(c3, x6), _mm_add_ps(_mm_mul_ps(c2, x4), _mm_add_ps(_mm_mul_ps(c1, x2), c0))));
    }

    AZ_MATH_FORCE_INLINE void VectorFloat::GetSinCos(VectorFloat& sin, VectorFloat& cos) const
    {
        //can probably improve this... calculating all 4 components, we only need 1...
        SimdVectorType x = GetAngleMod().m_value;
        SimdVectorType x2 = _mm_mul_ps(x, x);
        SimdVectorType x3 = _mm_mul_ps(x, x2);
        SimdVectorType x4 = _mm_mul_ps(x2, x2);
        SimdVectorType x5 = _mm_mul_ps(x3, x2);
        SimdVectorType x6 = _mm_mul_ps(x3, x3);
        SimdVectorType x7 = _mm_mul_ps(x4, x3);
        SimdVectorType sinCoefficients = *(const SimdVectorType*)&Internal::g_sinEstimateCoefficients;
        SimdVectorType cosCoefficients = *(const SimdVectorType*)&Internal::g_cosEstimateCoefficients;
        SimdVectorType s1 = _mm_shuffle_ps(sinCoefficients, sinCoefficients, _MM_SHUFFLE(1, 1, 1, 1));
        SimdVectorType s2 = _mm_shuffle_ps(sinCoefficients, sinCoefficients, _MM_SHUFFLE(2, 2, 2, 2));
        SimdVectorType s3 = _mm_shuffle_ps(sinCoefficients, sinCoefficients, _MM_SHUFFLE(3, 3, 3, 3));
        SimdVectorType c0 = _mm_shuffle_ps(cosCoefficients, cosCoefficients, _MM_SHUFFLE(0, 0, 0, 0));
        SimdVectorType c1 = _mm_shuffle_ps(cosCoefficients, cosCoefficients, _MM_SHUFFLE(1, 1, 1, 1));
        SimdVectorType c2 = _mm_shuffle_ps(cosCoefficients, cosCoefficients, _MM_SHUFFLE(2, 2, 2, 2));
        SimdVectorType c3 = _mm_shuffle_ps(cosCoefficients, cosCoefficients, _MM_SHUFFLE(3, 3, 3, 3));
        sin = VectorFloat(_mm_add_ps(_mm_mul_ps(s3, x7), _mm_add_ps(_mm_mul_ps(s2, x5), _mm_add_ps(_mm_mul_ps(s1, x3), x))));
        cos = VectorFloat(_mm_add_ps(_mm_mul_ps(c3, x6), _mm_add_ps(_mm_mul_ps(c2, x4), _mm_add_ps(_mm_mul_ps(c1, x2), c0))));
    }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetAbs() const
    {
        return VectorFloat(_mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdAbsMask));
    }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetReciprocal() const       { return GetReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetReciprocalApprox() const { return VectorFloat(_mm_rcp_ps(m_value)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetReciprocalExact() const  { return VectorFloat::CreateOne() / (*this); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrt() const             { return GetSqrtExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtApprox() const
    {
        return VectorFloat(_mm_rcp_ps(_mm_rsqrt_ps(m_value)));
        //uses sqrt(x) = x / sqrt(x), more accurate than using _mm_rcp_ps(_mm_rsqrt_ps(x))
        //SimdVectorType root = _mm_mul_ps(m_value, _mm_rsqrt_ps(m_value));
        //SimdVectorType cmpMask = _mm_cmpeq_ps(m_value, _mm_setzero_ps()); //handle 0.0
        //return VectorFloat(_mm_or_ps(_mm_and_ps(cmpMask, m_value), _mm_andnot_ps(cmpMask, root)));
    }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtExact() const            { return VectorFloat(_mm_sqrt_ps(m_value)); }

    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtReciprocal() const       { return GetSqrtReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtReciprocalApprox() const { return VectorFloat(_mm_rsqrt_ps(m_value)); }
    AZ_MATH_FORCE_INLINE const VectorFloat VectorFloat::GetSqrtReciprocalExact() const  { return GetSqrtExact().GetReciprocalExact(); }

    AZ_MATH_FORCE_INLINE bool VectorFloat::operator==(const VectorFloat& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) & 0x01) == 0);
    }

    AZ_MATH_FORCE_INLINE bool VectorFloat::operator!=(const VectorFloat& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) & 0x01) != 0);
    }

    AZ_MATH_FORCE_INLINE bool VectorFloat::IsLessThan(const VectorFloat& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpge_ps(m_value, rhs.m_value)) & 0x01) == 0);
    }
    AZ_MATH_FORCE_INLINE bool VectorFloat::IsLessEqualThan(const VectorFloat& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpgt_ps(m_value, rhs.m_value)) & 0x01) == 0);
    }
    AZ_MATH_FORCE_INLINE bool VectorFloat::IsGreaterThan(const VectorFloat& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmple_ps(m_value, rhs.m_value)) & 0x01) == 0);
    }
    AZ_MATH_FORCE_INLINE bool VectorFloat::IsGreaterEqualThan(const VectorFloat& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmplt_ps(m_value, rhs.m_value)) & 0x01) == 0);
    }
}
