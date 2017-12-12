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
    AZ_MATH_FORCE_INLINE Quaternion::Quaternion(const VectorFloat& x)                               { Set(x); }
    AZ_MATH_FORCE_INLINE Quaternion::Quaternion(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)                 { Set(x, y, z, w); }
    AZ_MATH_FORCE_INLINE Quaternion::Quaternion(SimdVectorType value)
        : m_value(value)          { }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::CreateZero()      { return Quaternion(_mm_setzero_ps()); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::CreateFromFloat4(const float* values)
    {
        return Quaternion(_mm_setr_ps(values[0], values[1], values[2], values[3]));
    }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::CreateFromVector3AndValue(const Vector3& v, const VectorFloat& w)
    {
        Quaternion result;
        result.Set(v, w);
        return result;
    }
    AZ_MATH_FORCE_INLINE void Quaternion::StoreToFloat4(float* values) const
    {
        _mm_storeu_ps(values, m_value);
    }


    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetX() const { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(0, 0, 0, 0))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetY() const { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(1, 1, 1, 1))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetZ() const { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(2, 2, 2, 2))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetW() const { return VectorFloat(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(3, 3, 3, 3))); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetElement(int index) const
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
    AZ_MATH_FORCE_INLINE void Quaternion::SetX(const VectorFloat& x)    { m_value = _mm_shuffle_ps(_mm_unpacklo_ps(m_value, x.m_value), m_value, _MM_SHUFFLE(3, 2, 2, 1)); }
    AZ_MATH_FORCE_INLINE void Quaternion::SetY(const VectorFloat& y)    { m_value = _mm_shuffle_ps(_mm_unpacklo_ps(m_value, y.m_value), m_value, _MM_SHUFFLE(3, 2, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Quaternion::SetZ(const VectorFloat& z)    { m_value = _mm_shuffle_ps(m_value, _mm_unpackhi_ps(m_value, z.m_value), _MM_SHUFFLE(2, 1, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Quaternion::SetW(const VectorFloat& w)    { m_value = _mm_shuffle_ps(m_value, _mm_unpackhi_ps(m_value, w.m_value), _MM_SHUFFLE(1, 0, 1, 0)); }
    AZ_MATH_FORCE_INLINE void Quaternion::SetElement(int index, const VectorFloat& v)
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
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const VectorFloat& x) { m_value = x.m_value; }
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)
    {
        m_value = _mm_unpacklo_ps(_mm_unpacklo_ps(x.m_value, z.m_value), _mm_unpacklo_ps(y.m_value, w.m_value));
    }
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const Vector3& v, const VectorFloat& w)
    {
        m_value = _mm_shuffle_ps(v.m_value, _mm_shuffle_ps(v.m_value, w.m_value, _MM_SHUFFLE(0, 0, 2, 2)), _MM_SHUFFLE(3, 0, 1, 0));
    }
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const float values[]) { m_value = _mm_loadu_ps(values); }

    AZ_MATH_FORCE_INLINE bool Quaternion::IsIdentity(const VectorFloat& tolerance) const
    {
        return IsClose(Quaternion::CreateIdentity(), tolerance);
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::GetConjugate() const
    {
        return Quaternion(_mm_xor_ps(m_value, *(SimdVectorType*)&Internal::g_simdNegateXYZMask));
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::Dot(const Quaternion& q) const
    {
        SimdVectorType x2 = _mm_mul_ps(m_value, q.m_value);
        SimdVectorType xyAndYz = _mm_add_ps(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(3, 3, 1, 1)), x2);
        SimdVectorType xyzw = _mm_add_ss(_mm_shuffle_ps(xyAndYz, xyAndYz, _MM_SHUFFLE(2, 2, 2, 2)), xyAndYz);
        return VectorFloat(_mm_shuffle_ps(xyzw, xyzw, _MM_SHUFFLE(0, 0, 0, 0)));
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetLength() const                    { return GetLengthSq().GetSqrt(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetLengthApprox() const              { return GetLengthSq().GetSqrtApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetLengthExact() const               { return GetLengthSq().GetSqrtExact(); }

    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetLengthReciprocal() const          { return GetLengthSq().GetSqrtReciprocal(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetLengthReciprocalApprox() const    { return GetLengthSq().GetSqrtReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetLengthReciprocalExact() const     { return GetLengthSq().GetSqrtReciprocalExact(); }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::GetNormalized() const                 { return (*this) * GetLengthReciprocal(); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::GetNormalizedApprox() const           { return (*this) * GetLengthReciprocalApprox(); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::GetNormalizedExact() const            { return (*this) * GetLengthReciprocalExact(); }

    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::NormalizeWithLength()
    {
        VectorFloat length = GetLength();
        *this /= length;
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::NormalizeWithLengthApprox()
    {
        VectorFloat length = GetLengthApprox();
        (*this) /= length;
        return length;
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::NormalizeWithLengthExact()
    {
        VectorFloat length = GetLengthExact();
        (*this) /= length;
        return length;
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::Lerp(const Quaternion& dest, const VectorFloat& t) const
    {
        return (*this) + (dest - (*this)) * t;
    }

    AZ_MATH_FORCE_INLINE bool Quaternion::IsClose(const Quaternion& q, const VectorFloat& tolerance) const
    {
        SimdVectorType diff = _mm_sub_ps(q.m_value, m_value);
        SimdVectorType absDiff = _mm_and_ps(diff, *(const SimdVectorType*)&Internal::g_simdAbsMask);
        return (_mm_movemask_ps(_mm_cmpgt_ps(absDiff, tolerance.m_value)) == 0);
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator-() const { return Quaternion(_mm_xor_ps(m_value, *(SimdVectorType*)&Internal::g_simdNegateMask)); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator+(const Quaternion& q) const  { return Quaternion(_mm_add_ps(m_value, q.m_value)); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator-(const Quaternion& q) const  { return Quaternion(_mm_sub_ps(m_value, q.m_value)); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator*(const Quaternion& q) const
    {
        //Quaternion multiplication in terms of components.
        // Maybe can optimize this better with more thought... currently is 16 ops
        //  q_x = w1*x2 + x1*w2 + y1*z2 - z1*y2
        //  q_y = w1*y2 + y1*w2 + z1*x2 - x1*z2
        //  q_z = w1*z2 + z1*w2 + x1*y2 - y1*x2
        //  q_w = w1*w2 - x1*x2 - y1*y2 - z1*z2
        SimdVectorType xzNegXNegY = _mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(1, 0, 2, 0));
        xzNegXNegY = _mm_xor_ps(xzNegXNegY, *(SimdVectorType*)&Internal::g_simdNegateZWMask);
        SimdVectorType a = _mm_mul_ps(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(3, 3, 3, 3)), q.m_value);
        SimdVectorType b = _mm_mul_ps(_mm_shuffle_ps(m_value, xzNegXNegY, _MM_SHUFFLE(2, 1, 1, 0)),
                _mm_shuffle_ps(q.m_value, q.m_value, _MM_SHUFFLE(0, 3, 3, 3)));
        SimdVectorType c = _mm_mul_ps(_mm_shuffle_ps(m_value, xzNegXNegY, _MM_SHUFFLE(3, 0, 2, 1)),
                _mm_shuffle_ps(q.m_value, q.m_value, _MM_SHUFFLE(1, 1, 0, 2)));
        SimdVectorType d = _mm_mul_ps(_mm_shuffle_ps(m_value, m_value, _MM_SHUFFLE(2, 1, 0, 2)),
                _mm_shuffle_ps(q.m_value, q.m_value, _MM_SHUFFLE(2, 0, 2, 1)));
        return Quaternion(_mm_add_ps(_mm_add_ps(_mm_sub_ps(c, d), b), a));
    }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator*(const VectorFloat& multiplier) const
    {
        return Quaternion(_mm_mul_ps(m_value, multiplier.m_value));
    }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator/(const VectorFloat& divisor) const
    {
        return Quaternion(_mm_div_ps(m_value, divisor.m_value));
    }

    AZ_MATH_FORCE_INLINE bool Quaternion::operator==(const Quaternion& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) == 0);
    }
    AZ_MATH_FORCE_INLINE bool Quaternion::operator!=(const Quaternion& rhs) const
    {
        return (_mm_movemask_ps(_mm_cmpneq_ps(m_value, rhs.m_value)) != 0);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Quaternion::operator*(const Vector3& v) const
    {
        //inverse must come last, this ensures associativity, (q1*q2)*v = q1*(q2*v)
        //TODO: can be optimized a little by inlining one of the multiplies and removing the w1(=0) terms
        Quaternion result = (*this) * Quaternion::CreateFromVector3(v) * GetInverseFast();
        return Vector3(result.m_value);
    }

    /**
    * Allows pre-multiplying by a float
    */
    AZ_MATH_FORCE_INLINE const Quaternion operator*(const VectorFloat& multiplier, const Quaternion& rhs)
    {
        return rhs * multiplier;
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::GetAbs() const
    {
        return Quaternion(_mm_and_ps(m_value, *(const SimdVectorType*)&Internal::g_simdAbsMask));
    }
}
