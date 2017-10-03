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
    AZ_MATH_FORCE_INLINE Vector4::Vector4(const VectorFloat& x)                                                                         { Set(x); }
    AZ_MATH_FORCE_INLINE Vector4::Vector4(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)       { Set(x, y, z, w); }
    AZ_MATH_FORCE_INLINE Vector4::Vector4(SimdVectorType value)
        : m_value(value)    { }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::CreateZero()            { return Vector4(_mm_setzero_ps()); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::CreateOne()         { return Vector4(*(SimdVectorType*)&Internal::g_simd1111); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::CreateFromFloat4(const float values[])
    {
        Vector4 result;
        result.Set(values);
        return result;
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::CreateFromVector3(const Vector3& v)
    {
        Vector4 result;
        result.Set(v);
        return result;
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::CreateFromVector3AndFloat(const Vector3& v, const VectorFloat& w)
    {
        Vector4 result;
        result.Set(v, w);
        return result;
    }
    AZ_MATH_FORCE_INLINE void Vector4::StoreToFloat4(float* values) const
    {
        _mm_storeu_ps(values, m_value);
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetX() const    { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(0, 0, 0, 0))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetY() const    { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(1, 1, 1, 1))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetZ() const    { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(2, 2, 2, 2))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetW() const    { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(3, 3, 3, 3))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetElement(int index) const
    {
        AZ_Assert((index >= 0) && (index < 4), "Invalid index for component access!\n");
        if (index == 0)
        {
            return GetX();
        }
        else if (index == 1)
        {
            return GetY();
        }
        else if (index == 2)
        {
            return GetZ();
        }
        else
        {
            return GetW();
        }
    }
    AZ_MATH_FORCE_INLINE void Vector4::SetX(const VectorFloat& x)   { m_value = _mm_shuffle_ps(_mm_unpacklo_ps(m_value, x.m_value), m_value, _MM_SHUFFLE(3, 2, 2, 1)); }
    AZ_MATH_FORCE_INLINE void Vector4::SetY(const VectorFloat& y)   { m_value = _mm_shuffle_ps(_mm_unpacklo_ps(m_value, y.m_value), m_value, _MM_SHUFFLE(3, 2, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Vector4::SetZ(const VectorFloat& z)   { m_value = _mm_shuffle_ps(m_value, _mm_unpackhi_ps(m_value, z.m_value), _MM_SHUFFLE(2, 1, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Vector4::SetW(const VectorFloat& w)   { m_value = _mm_shuffle_ps(m_value, _mm_unpackhi_ps(m_value, w.m_value), _MM_SHUFFLE(1, 0, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const VectorFloat& x)    { m_value = x.m_value; }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)
    {
        m_value = _mm_unpacklo_ps(_mm_unpacklo_ps(x.m_value, z.m_value), _mm_unpacklo_ps(y.m_value, w.m_value));
    }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const float values[])    { m_value = _mm_loadu_ps(values); }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const Vector3& v)
    {
        m_value = _mm_shuffle_ps(v.m_value, _mm_unpackhi_ps(v.m_value, *(SimdVectorType*)&Internal::g_simd1111), _MM_SHUFFLE(3, 0, 1, 0));
    }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const Vector3& v, const VectorFloat& w)
    {
        m_value = _mm_shuffle_ps(v.m_value, _mm_shuffle_ps(v.m_value, w.m_value, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 0, 1, 0));
    }
    AZ_MATH_FORCE_INLINE void Vector4::SetElement(int index, const VectorFloat& v)
    {
        AZ_Assert((index >= 0) && (index < 4), "Invalid index for component access!\n");
        if (index == 0)
        {
            SetX(v);
        }
        else if (index == 1)
        {
            SetY(v);
        }
        else if (index == 2)
        {
            SetZ(v);
        }
        else
        {
            SetW(v);
        }
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLength() const       { return GetLengthSq().GetSqrt(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthApprox() const { return GetLengthSq().GetSqrtApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthExact() const  { return GetLengthSq().GetSqrtExact(); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthReciprocal() const     { return GetLengthSq().GetSqrtReciprocal(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthReciprocalApprox() const { return GetLengthSq().GetSqrtReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthReciprocalExact() const  { return GetLengthSq().GetSqrtReciprocalExact(); }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalized() const       { return (*this) * GetLengthReciprocal(); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedApprox() const { return (*this) * GetLengthReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedExact() const  { return (*this) / GetLengthExact(); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeWithLength()
    {
        VectorFloat length = GetLength();
        (*this) /= length;
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeWithLengthApprox()
    {
        VectorFloat length = GetLengthApprox();
        (*this) /= length;
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeWithLengthExact()
    {
        VectorFloat length = GetLengthExact();
        (*this) /= length;
        return length;
    }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedSafe(const VectorFloat& tolerance) const
    {
        VectorFloat length = GetLength();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            return Vector4(*(SimdVectorType*)&Internal::g_simd1000);
        }
        else
        {
            return (*this) / length;
        }
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedSafeApprox(const VectorFloat& tolerance) const
    {
        VectorFloat length = GetLengthApprox();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            return Vector4(*(SimdVectorType*)&Internal::g_simd1000);
        }
        else
        {
            return (*this) / length;
        }
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedSafeExact(const VectorFloat& tolerance) const
    {
        VectorFloat length = GetLengthExact();
        if ((_mm_movemask_ps(_mm_cmple_ss(length.m_value, tolerance.m_value)) & 0x01) != 0)
        {
            return Vector4(*(SimdVectorType*)&Internal::g_simd1000);
        }
        else
        {
            return (*this) / length;
        }
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeSafeWithLength(const VectorFloat& tolerance)
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
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeSafeWithLengthApprox(const VectorFloat& tolerance)
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
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeSafeWithLengthExact(const VectorFloat& tolerance)
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

    AZ_MATH_FORCE_INLINE bool Vector4::IsNormalized(const VectorFloat& tolerance) const
    {
        SimdVectorType one = *reinterpret_cast<const SimdVectorType*>(&Internal::g_simd1111);
        SimdVectorType diff = _mm_sub_ss(GetLengthSq().m_value, one);
        SimdVectorType absDiff = _mm_and_ps(diff, *(const SimdVectorType*)&Internal::g_simdAbsMask);
        return ((_mm_movemask_ps(_mm_cmple_ss(absDiff, tolerance.m_value)) & 0x01) != 0);
    }
    AZ_MATH_FORCE_INLINE bool Vector4::IsClose(const Vector4& v, const VectorFloat& tolerance) const
    {
        SimdVectorType diff = _mm_sub_ps(v.m_value, m_value);
        SimdVectorType absDiff = _mm_and_ps(diff, *(const SimdVectorType*)&Internal::g_simdAbsMask);
        return (_mm_movemask_ps(_mm_cmpgt_ps(absDiff, tolerance.m_value)) == 0);
    }

    AZ_MATH_FORCE_INLINE bool Vector4::operator==(const Vector4& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) == 0);
    }

    AZ_MATH_FORCE_INLINE bool Vector4::operator!=(const Vector4& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) != 0);
    }

    AZ_MATH_FORCE_INLINE bool Vector4::IsLessThan(const Vector4& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpge_ps(m_value, rhs.m_value)) & 0x0f) == 0);
    }
    AZ_MATH_FORCE_INLINE bool Vector4::IsLessEqualThan(const Vector4& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmpgt_ps(m_value, rhs.m_value)) & 0x0f) == 0);
    }
    AZ_MATH_FORCE_INLINE bool Vector4::IsGreaterThan(const Vector4& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmple_ps(m_value, rhs.m_value)) & 0x0f) == 0);
    }
    AZ_MATH_FORCE_INLINE bool Vector4::IsGreaterEqualThan(const Vector4& rhs) const
    {
        return ((_mm_movemask_ps(_mm_cmplt_ps(m_value, rhs.m_value)) & 0x0f) == 0);
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::Dot(const Vector4& rhs) const
    {
        SimdVectorType x2 = _mm_mul_ps(m_value, rhs.m_value);
        SimdVectorType tmp = _mm_add_ps(x2, _mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 3, 0, 1)));
        return VectorFloat(_mm_add_ps(tmp, _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(1, 0, 2, 3))));
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::Dot3(const Vector3& rhs) const
    {
        SimdVectorType x2 = _mm_mul_ps(m_value, rhs.m_value);
        SimdVectorType xy = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 1, 1, 1)), x2);
        SimdVectorType xyz = _mm_add_ss(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(2, 2, 2, 2)), xy);
        return VectorFloat(_mm_shuffle_ps(xyz, xyz, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    AZ_MATH_FORCE_INLINE void Vector4::Homogenize()
    {
        m_value = _mm_div_ps(m_value, _mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(3, 3, 3, 3)));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector4::GetHomogenized() const
    {
        return Vector3(_mm_div_ps(m_value, _mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(3, 3, 3, 3))));
    }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator-() const
    {
        return Vector4(_mm_xor_ps(m_value, *(const SimdVectorType*)&Internal::g_simdNegateMask));
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator+(const Vector4& rhs) const
    {
        return Vector4(_mm_add_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator-(const Vector4& rhs) const
    {
        return Vector4(_mm_sub_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator*(const Vector4& rhs) const
    {
        return Vector4(_mm_mul_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator/(const Vector4& rhs) const
    {
        return Vector4(_mm_div_ps(m_value, rhs.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator*(const VectorFloat& multiplier) const
    {
        return Vector4(_mm_mul_ps(m_value, multiplier.m_value));
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator/(const VectorFloat& divisor) const
    {
        return Vector4(_mm_div_ps(m_value, divisor.m_value));
    }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetAbs() const
    {
        return Vector4(_mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdAbsMask));
    }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetReciprocal() const { return Vector4(_mm_rcp_ps(m_value)); }

    AZ_MATH_FORCE_INLINE const Vector4 operator*(const VectorFloat& multiplier, const Vector4& rhs)
    {
        return rhs * multiplier;
    }
}
