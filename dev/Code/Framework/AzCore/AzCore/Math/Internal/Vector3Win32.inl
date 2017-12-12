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

namespace AZ
{
    AZ_MATH_FORCE_INLINE Vector3::Vector3(const VectorFloat& x)                                             { Set(x); }
    AZ_MATH_FORCE_INLINE Vector3::Vector3(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z) { Set(x, y, z); }
    AZ_MATH_FORCE_INLINE Vector3::Vector3(SimdVectorType value)
        : m_value(value)                                { }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateZero()            { return Vector3(_mm_setzero_ps()); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateOne()         { return Vector3(*(SimdVectorType*)&Internal::g_simd1111); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateFromFloat3(const float values[])
    {
        Vector3 result;
        result.Set(values);
        return result;
    }

    AZ_MATH_FORCE_INLINE void Vector3::StoreToFloat3(float* values) const
    {
        _mm_store_ss(&values[0], m_value);
        _mm_store_ss(&values[1], _mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(1, 1, 1, 1)));
        _mm_store_ss(&values[2], _mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(2, 2, 2, 2)));
    }
    AZ_MATH_FORCE_INLINE void Vector3::StoreToFloat4(float* values) const
    {
        _mm_storeu_ps(values, m_value);
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetX() const        { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(0, 0, 0, 0))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetY() const        { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(1, 1, 1, 1))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetZ() const        { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(2, 2, 2, 2))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetElement(int index) const
    {
        AZ_Assert((index >= 0) && (index < 3), "Invalid index for component access!\n");
        if (index == 0)
        {
            return GetX();
        }
        else if (index == 1)
        {
            return GetY();
        }
        else
        {
            return GetZ();
        }
    }

    AZ_MATH_FORCE_INLINE void Vector3::SetX(const VectorFloat& x)       { m_value = _mm_shuffle_ps(_mm_unpacklo_ps(m_value, x.m_value), m_value, _MM_SHUFFLE(2, 2, 2, 1)); }
    AZ_MATH_FORCE_INLINE void Vector3::SetY(const VectorFloat& y)       { m_value = _mm_shuffle_ps(_mm_unpacklo_ps(m_value, y.m_value), m_value, _MM_SHUFFLE(2, 2, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Vector3::SetZ(const VectorFloat& z)       { m_value = _mm_shuffle_ps(m_value, z.m_value, _MM_SHUFFLE(0, 0, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Vector3::SetElement(int index, const VectorFloat& v)
    {
        AZ_Assert((index >= 0) && (index < 3), "Invalid index for component access!\n");
        if (index == 0)
        {
            SetX(v);
        }
        else if (index == 1)
        {
            SetY(v);
        }
        else
        {
            SetZ(v);
        }
    }
    AZ_MATH_FORCE_INLINE void Vector3::Set(const VectorFloat& x)        { m_value = x.m_value; }
    AZ_MATH_FORCE_INLINE void Vector3::Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z)
    {
        m_value = _mm_unpacklo_ps(_mm_unpacklo_ps(x.m_value, z.m_value), y.m_value);
    }
    AZ_MATH_FORCE_INLINE void Vector3::Set(const float values[])        { m_value = _mm_loadu_ps(values); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLength() const       { return GetLengthSq().GetSqrt(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthApprox() const { return GetLengthSq().GetSqrtApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthExact() const  { return GetLengthSq().GetSqrtExact(); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthReciprocal() const     { return GetLengthSq().GetSqrtReciprocal(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthReciprocalApprox() const { return GetLengthSq().GetSqrtReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthReciprocalExact() const  { return GetLengthSq().GetSqrtReciprocalExact(); }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalized() const       { return (*this) * GetLengthReciprocal(); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedApprox() const { return (*this) * GetLengthReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedExact() const  { return (*this) / GetLengthExact(); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeWithLength()
    {
        VectorFloat length = GetLength();
        (*this) /= length;
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeWithLengthApprox()
    {
        VectorFloat length = GetLengthApprox();
        (*this) /= length;
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeWithLengthExact()
    {
        VectorFloat length = GetLengthExact();
        (*this) /= length;
        return length;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedSafe(const VectorFloat& tolerance) const
    {
        VectorFloat length = GetLength();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            return Vector3(*(SimdVectorType*)&Internal::g_simd1000);
        }
        else
        {
            return (*this) / length;
        }
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedSafeApprox(const VectorFloat& tolerance) const
    {
        VectorFloat length = GetLengthApprox();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            return Vector3(*(SimdVectorType*)&Internal::g_simd1000);
        }
        else
        {
            return (*this) / length;
        }
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedSafeExact(const VectorFloat& tolerance) const
    {
        VectorFloat length = GetLengthExact();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            return Vector3(*(SimdVectorType*)&Internal::g_simd1000);
        }
        else
        {
            return (*this) / length;
        }
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeSafeWithLength(const VectorFloat& tolerance)
    {
        VectorFloat length = GetLength();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            m_value = *(SimdVectorType*)&Internal::g_simd1000;
        }
        else
        {
            (*this) /= length;
        }
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeSafeWithLengthApprox(const VectorFloat& tolerance)
    {
        VectorFloat length = GetLengthApprox();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            m_value = *(SimdVectorType*)&Internal::g_simd1000;
        }
        else
        {
            (*this) /= length;
        }
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeSafeWithLengthExact(const VectorFloat& tolerance)
    {
        VectorFloat length = GetLengthExact();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            m_value = *(SimdVectorType*)&Internal::g_simd1000;
        }
        else
        {
            (*this) /= length;
        }
        return length;
    }

    AZ_MATH_FORCE_INLINE bool Vector3::IsNormalized(const VectorFloat& tolerance) const
    {
        SimdVectorType one = *reinterpret_cast<const SimdVectorType*>(&Internal::g_simd1111);
        SimdVectorType diff = _mm_sub_ss(GetLengthSq().m_value, one);
        SimdVectorType absDiff = _mm_and_ps(diff, *(const SimdVectorType*)&Internal::g_simdAbsMask);
        return ((_mm_movemask_ps(_mm_cmple_ss(absDiff, tolerance.m_value)) & 0x01) != 0);
    }

    AZ_MATH_FORCE_INLINE void Vector3::SetLength(const VectorFloat& length)
    {
        VectorFloat scale(_mm_mul_ps(length.m_value, GetLengthReciprocal().m_value));
        (*this) *= scale;
    }
    AZ_MATH_FORCE_INLINE void Vector3::SetLengthApprox(const VectorFloat& length)
    {
        VectorFloat scale(_mm_mul_ps(length.m_value, GetLengthReciprocalApprox().m_value));
        (*this) *= scale;
    }
    AZ_MATH_FORCE_INLINE void Vector3::SetLengthExact(const VectorFloat& length)
    {
        VectorFloat scale(_mm_mul_ps(length.m_value, GetLengthReciprocalExact().m_value));
        (*this) *= scale;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::Lerp(const Vector3& dest, const VectorFloat& t) const
    {
        return (*this) + (dest - (*this)) * t;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::Dot(const Vector3& rhs) const
    {
        SimdVectorType x2 = _mm_mul_ps(m_value, rhs.m_value);
        SimdVectorType xy = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 1, 1, 1)), x2);
        SimdVectorType xyz = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 2, 2, 2)), xy);
        return VectorFloat(_mm_shuffle_ps(xyz, xyz, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::Cross(const Vector3& rhs) const
    {
        SimdVectorType cross1 = _mm_mul_ps(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(3, 0, 2, 1)),
                _mm_shuffle_ps(rhs.m_value, rhs.m_value, _MM_SHUFFLE(3, 1, 0, 2)));
        SimdVectorType cross2 = _mm_mul_ps(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(3, 1, 0, 2)),
                _mm_shuffle_ps(rhs.m_value, rhs.m_value, _MM_SHUFFLE(3, 0, 2, 1)));
        return Vector3(_mm_sub_ps(cross1, cross2));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CrossXAxis() const
    {
        SimdVectorType maskedW = _mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdWMask);
        SimdVectorType v0Z0 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 2, 3));
        SimdVectorType v00Y = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 1, 3, 3));
        return Vector3(_mm_sub_ps(v0Z0, v00Y));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CrossYAxis() const
    {
        SimdVectorType maskedW = _mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdWMask);
        SimdVectorType v00X = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 0, 3, 3));
        SimdVectorType vZ00 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 3, 2));
        return Vector3(_mm_sub_ps(v00X, vZ00));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CrossZAxis() const
    {
        SimdVectorType maskedW = _mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdWMask);
        SimdVectorType vY00 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 3, 1));
        SimdVectorType v0X0 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 0, 3));
        return Vector3(_mm_sub_ps(vY00, v0X0));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::XAxisCross() const
    {
        SimdVectorType maskedW = _mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdWMask);
        SimdVectorType v00Y = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 1, 3, 3));
        SimdVectorType v0Z0 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 2, 3));
        return Vector3(_mm_sub_ps(v00Y, v0Z0));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::YAxisCross() const
    {
        SimdVectorType maskedW = _mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdWMask);
        SimdVectorType vZ00 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 3, 2));
        SimdVectorType v00X = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 0, 3, 3));
        return Vector3(_mm_sub_ps(vZ00, v00X));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::ZAxisCross() const
    {
        SimdVectorType maskedW = _mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdWMask);
        SimdVectorType v0X0 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 0, 3));
        SimdVectorType vY00 = _mm_shuffle_ps(maskedW, maskedW, _MM_SHUFFLE(3, 3, 3, 1));
        return Vector3(_mm_sub_ps(v0X0, vY00));
    }

    AZ_MATH_FORCE_INLINE bool Vector3::IsClose(const Vector3& v, const VectorFloat& tolerance) const
    {
        Vector3 dist = (v - (*this)).GetAbs();
        return dist.IsLessEqualThan(Vector3(tolerance.m_value));
    }

    AZ_MATH_FORCE_INLINE bool Vector3::operator==(const Vector3& rhs) const
    {
        //we could perhaps remove the &0x07 by making the w component consistent (set equal to z perhaps),
        // but it optimizes to the same amount of instructions either way.
        return ((_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) & 0x07) == 0);
    }

    AZ_MATH_FORCE_INLINE bool Vector3::operator!=(const Vector3& rhs) const
    {
        //we could perhaps remove the &0x07 by making the w component consistent (set equal to z perhaps),
        // but it optimizes to the same amount of instructions either way.
        return ((_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) & 0x07) != 0);
    }

    AZ_MATH_FORCE_INLINE bool Vector3::IsLessThan(const Vector3& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpge_ps(m_value, rhs.m_value)) & 0x07) == 0);
    }
    AZ_MATH_FORCE_INLINE bool Vector3::IsLessEqualThan(const Vector3& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpgt_ps(m_value, rhs.m_value)) & 0x07) == 0);
    }
    AZ_MATH_FORCE_INLINE bool Vector3::IsGreaterThan(const Vector3& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmple_ps(m_value, rhs.m_value)) & 0x07) == 0);
    }
    AZ_MATH_FORCE_INLINE bool Vector3::IsGreaterEqualThan(const Vector3& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmplt_ps(m_value, rhs.m_value)) & 0x07) == 0);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetMin(const Vector3& v) const
    {
        return Vector3(_mm_min_ps(m_value, v.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetMax(const Vector3& v) const
    {
        return Vector3(_mm_max_ps(m_value, v.m_value));
    }

    // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateSelectCmpEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
        SimdVectorType mask = _mm_cmpeq_ps(cmp1.m_value, cmp2.m_value);
        return Vector3(_mm_or_ps(_mm_and_ps(mask, vA.m_value), _mm_andnot_ps(mask, vB.m_value)));
    }

    // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateSelectCmpGreaterEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
        SimdVectorType mask = _mm_cmpge_ps(cmp1.m_value, cmp2.m_value);
        return Vector3(_mm_or_ps(_mm_and_ps(mask, vA.m_value), _mm_andnot_ps(mask, vB.m_value)));
    }

    // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateSelectCmpGreater(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
        SimdVectorType mask = _mm_cmpgt_ps(cmp1.m_value, cmp2.m_value);
        return Vector3(_mm_or_ps(_mm_and_ps(mask, vA.m_value), _mm_andnot_ps(mask, vB.m_value)));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetAngleMod() const
    {
        //converting to float, don't have sse instructions that can do this...
        float x = GetX();
        float y = GetY();
        float z = GetZ();
        return Vector3((x >= 0.0f) ? (fmodf(x + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(x - Constants::Pi, Constants::TwoPi) + Constants::Pi),
            (y >= 0.0f) ? (fmodf(y + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(y - Constants::Pi, Constants::TwoPi) + Constants::Pi),
            (z >= 0.0f) ? (fmodf(z + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(z - Constants::Pi, Constants::TwoPi) + Constants::Pi));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetSin() const
    {
        SimdVectorType x = GetAngleMod().m_value;
        SimdVectorType x2 = _mm_mul_ps(x, x);
        SimdVectorType x3 = _mm_mul_ps(x, x2);
        SimdVectorType x5 = _mm_mul_ps(x3, x2);
        SimdVectorType x7 = _mm_mul_ps(x5, x2);
        SimdVectorType coefficients = *(const SimdVectorType*)&Internal::g_sinEstimateCoefficients;
        SimdVectorType c1 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(1, 1, 1, 1));
        SimdVectorType c2 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(2, 2, 2, 2));
        SimdVectorType c3 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(3, 3, 3, 3));
        return Vector3(_mm_add_ps(_mm_mul_ps(c3, x7), _mm_add_ps(_mm_mul_ps(c2, x5), _mm_add_ps(_mm_mul_ps(c1, x3), x))));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetCos() const
    {
        SimdVectorType x = GetAngleMod().m_value;
        SimdVectorType x2 = _mm_mul_ps(x, x);
        SimdVectorType x4 = _mm_mul_ps(x2, x2);
        SimdVectorType x6 = _mm_mul_ps(x4, x2);
        SimdVectorType coefficients = *(const SimdVectorType*)&Internal::g_cosEstimateCoefficients;
        SimdVectorType c0 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(0, 0, 0, 0));
        SimdVectorType c1 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(1, 1, 1, 1));
        SimdVectorType c2 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(2, 2, 2, 2));
        SimdVectorType c3 = _mm_shuffle_ps(coefficients, coefficients, _MM_SHUFFLE(3, 3, 3, 3));
        return Vector3(_mm_add_ps(_mm_mul_ps(c3, x6), _mm_add_ps(_mm_mul_ps(c2, x4), _mm_add_ps(_mm_mul_ps(c1, x2), c0))));
    }

    AZ_MATH_FORCE_INLINE void Vector3::GetSinCos(Vector3& sin, Vector3& cos) const
    {
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
        sin = Vector3(_mm_add_ps(_mm_mul_ps(s3, x7), _mm_add_ps(_mm_mul_ps(s2, x5), _mm_add_ps(_mm_mul_ps(s1, x3), x))));
        cos = Vector3(_mm_add_ps(_mm_mul_ps(c3, x6), _mm_add_ps(_mm_mul_ps(c2, x4), _mm_add_ps(_mm_mul_ps(c1, x2), c0))));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetAbs() const
    {
        return Vector3(_mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdAbsMask));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetReciprocal() const       { return GetReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetReciprocalApprox() const { return Vector3(_mm_rcp_ps(m_value)); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetReciprocalExact() const  { return Vector3::CreateOne() / (*this); }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator-() const
    {
        return Vector3(_mm_xor_ps(m_value, *(const SimdVectorType*)&Internal::g_simdNegateMask));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator+(const Vector3& rhs) const
    {
        return Vector3(_mm_add_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator-(const Vector3& rhs) const
    {
        return Vector3(_mm_sub_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator*(const Vector3& rhs) const
    {
        return Vector3(_mm_mul_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator/(const Vector3& rhs) const
    {
        return Vector3(_mm_div_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator*(const VectorFloat& multiplier) const
    {
        return Vector3(_mm_mul_ps(m_value, multiplier.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator/(const VectorFloat& divisor) const
    {
        return Vector3(_mm_div_ps(m_value, divisor.m_value));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetMadd(const Vector3& mul, const Vector3& add)
    {
        return Vector3(_mm_add_ps(_mm_mul_ps(m_value, mul.m_value), add.m_value));
    }

    AZ_MATH_FORCE_INLINE bool Vector3::IsPerpendicular(const Vector3& v, const VectorFloat& tolerance) const
    {
        SimdVectorType absDot = _mm_and_ps(Dot(v).m_value, *(const SimdVectorType*)&Internal::g_simdAbsMask);
        return ((_mm_movemask_ps(_mm_cmple_ss(absDot, tolerance.m_value)) & 0x01) != 0);
    }

    AZ_MATH_FORCE_INLINE const Vector3 operator*(const VectorFloat& multiplier, const Vector3& rhs)
    {
        return rhs * multiplier;
    }
}
