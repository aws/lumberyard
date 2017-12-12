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

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::CreateZero()            { return Vector4(0.0f); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::CreateOne()         { return Vector4(1.0f); }
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
        values[0] = m_x;
        values[1] = m_y;
        values[2] = m_z;
        values[3] = m_w;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetX() const    { return m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetY() const    { return m_y; }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetZ() const    { return m_z; }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetW() const    { return m_w; }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetElement(int index) const
    {
        AZ_Assert((index >= 0) && (index < 4), "Invalid index for component access!\n");
        if (index == 0)
        {
            return m_x;
        }
        else if (index == 1)
        {
            return m_y;
        }
        else if (index == 2)
        {
            return m_z;
        }
        else
        {
            return m_w;
        }
    }
    AZ_MATH_FORCE_INLINE void Vector4::SetX(const VectorFloat& x)   { m_x = x; }
    AZ_MATH_FORCE_INLINE void Vector4::SetY(const VectorFloat& y)   { m_y = y; }
    AZ_MATH_FORCE_INLINE void Vector4::SetZ(const VectorFloat& z)   { m_z = z; }
    AZ_MATH_FORCE_INLINE void Vector4::SetW(const VectorFloat& w)   { m_w = w; }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const VectorFloat& x)    { m_x = x; m_y = x; m_z = x; m_w = x; }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)  { m_x = x; m_y = y; m_z = z; m_w = w; }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const float values[])    { m_x = values[0]; m_y = values[1]; m_z = values[2]; m_w = values[3]; }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const Vector3& v)        { m_x = v.GetX(); m_y = v.GetY(), m_z = v.GetZ(); m_w = 1.0f; }
    AZ_MATH_FORCE_INLINE void Vector4::Set(const Vector3& v, const VectorFloat& w)  { m_x = v.GetX(); m_y = v.GetY(), m_z = v.GetZ(); m_w = w; }
    AZ_MATH_FORCE_INLINE void Vector4::SetElement(int index, const VectorFloat& v)
    {
        AZ_Assert((index >= 0) && (index < 4), "Invalid index for component access!\n");
        if (index == 0)
        {
            m_x = v;
        }
        else if (index == 1)
        {
            m_y = v;
        }
        else if (index == 2)
        {
            m_z = v;
        }
        else
        {
            m_w = v;
        }
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLength() const       { return GetLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthApprox() const { return GetLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthExact() const  { return sqrtf(GetLengthSq()); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthReciprocal() const       { return GetLengthReciprocalExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthReciprocalApprox() const { return GetLengthReciprocalExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::GetLengthReciprocalExact() const  { return 1.0f / GetLength(); }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalized() const       { return GetNormalizedExact(); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedApprox() const { return GetNormalizedExact(); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedExact() const  { return (*this) / GetLength(); }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedSafe(const VectorFloat& tolerance) const { return GetNormalizedSafeExact(tolerance); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedSafeApprox(const VectorFloat& tolerance) const { return GetNormalizedSafeExact(tolerance); }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetNormalizedSafeExact(const VectorFloat& tolerance) const
    {
        float length = GetLength();
        if (length <= tolerance)
        {
            return Vector4(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            return (*this) / length;
        }
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeWithLength()       { return NormalizeWithLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeWithLengthApprox() { return NormalizeWithLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeWithLengthExact()
    {
        float length = GetLength();
        (*this) *= (1.0f / length);
        return length;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeSafeWithLength(const VectorFloat& tolerance) { return NormalizeSafeWithLengthExact(tolerance); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeSafeWithLengthApprox(const VectorFloat& tolerance) { return NormalizeSafeWithLengthExact(tolerance); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::NormalizeSafeWithLengthExact(const VectorFloat& tolerance)
    {
        float length = GetLength();
        if (length <= tolerance)
        {
            Set(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            (*this) *= (1.0f / length);
        }

        return length;
    }

    AZ_MATH_FORCE_INLINE bool Vector4::IsNormalized(const VectorFloat& tolerance) const
    {
        return (fabsf(GetLengthSq() - 1.0f) <= tolerance);
    }

    AZ_MATH_FORCE_INLINE bool Vector4::IsClose(const Vector4& v, const VectorFloat& tolerance) const
    {
        return ((fabsf(v.m_x - m_x) <= tolerance) && (fabsf(v.m_y - m_y) <= tolerance) && (fabsf(v.m_z - m_z) <= tolerance) && (fabsf(v.m_w - m_w) <= tolerance));
    }

    AZ_MATH_FORCE_INLINE bool Vector4::operator==(const Vector4& rhs) const
    {
        return ((m_x == rhs.m_x) && (m_y == rhs.m_y) && (m_z == rhs.m_z) && (m_w == rhs.m_w));
    }

    AZ_MATH_FORCE_INLINE bool Vector4::operator!=(const Vector4& rhs) const
    {
        return ((m_x != rhs.m_x) || (m_y != rhs.m_y) || (m_z != rhs.m_z) || (m_w != rhs.m_w));
    }

    AZ_MATH_FORCE_INLINE bool Vector4::IsLessThan(const Vector4& rhs) const         { return (GetX() < rhs.GetX()) && (GetY() < rhs.GetY()) && (GetZ() < rhs.GetZ()) && (GetW() < rhs.GetW()); }
    AZ_MATH_FORCE_INLINE bool Vector4::IsLessEqualThan(const Vector4& rhs) const        { return (GetX() <= rhs.GetX()) && (GetY() <= rhs.GetY()) && (GetZ() <= rhs.GetZ()) && (GetW() <= rhs.GetW()); }
    AZ_MATH_FORCE_INLINE bool Vector4::IsGreaterThan(const Vector4& rhs) const      { return (GetX() > rhs.GetX()) && (GetY() > rhs.GetY()) && (GetZ() > rhs.GetZ()) && (GetW() > rhs.GetW()); }
    AZ_MATH_FORCE_INLINE bool Vector4::IsGreaterEqualThan(const Vector4& rhs) const { return (GetX() >= rhs.GetX()) && (GetY() >= rhs.GetY()) && (GetZ() >= rhs.GetZ()) && (GetW() >= rhs.GetW()); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::Dot(const Vector4& rhs) const
    {
        return (m_x * rhs.m_x + m_y * rhs.m_y + m_z * rhs.m_z + m_w * rhs.m_w);
    }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector4::Dot3(const Vector3& rhs) const
    {
        return (m_x * rhs.GetX() + m_y * rhs.GetY() + m_z * rhs.GetZ());
    }

    AZ_MATH_FORCE_INLINE void Vector4::Homogenize()
    {
        m_x /= m_w;
        m_y /= m_w;
        m_z /= m_w;
        m_w = 1.0f;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector4::GetHomogenized() const
    {
        return Vector3(m_x / m_w, m_y / m_w, m_z / m_w);
    }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator-() const
    {
        return Vector4(-m_x, -m_y, -m_z, -m_w);
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator+(const Vector4& rhs) const
    {
        return Vector4(m_x + rhs.m_x, m_y + rhs.m_y, m_z + rhs.m_z, m_w + rhs.m_w);
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator-(const Vector4& rhs) const
    {
        return Vector4(m_x - rhs.m_x, m_y - rhs.m_y, m_z - rhs.m_z, m_w - rhs.m_w);
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator*(const Vector4& rhs) const
    {
        return Vector4(m_x * rhs.m_x, m_y * rhs.m_y, m_z * rhs.m_z, m_w * rhs.m_w);
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator/(const Vector4& rhs) const
    {
        return Vector4(m_x / rhs.m_x, m_y / rhs.m_y, m_z / rhs.m_z, m_w / rhs.m_w);
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator*(const VectorFloat& multiplier) const
    {
        return Vector4(m_x * multiplier, m_y * multiplier, m_z * multiplier, m_w * multiplier);
    }
    AZ_MATH_FORCE_INLINE const Vector4 Vector4::operator/(const VectorFloat& divisor) const
    {
        return Vector4(m_x / divisor, m_y / divisor, m_z / divisor, m_w / divisor);
    }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetAbs() const
    {
        return Vector4(fabsf(m_x), fabsf(m_y), fabsf(m_z), fabsf(m_w));
    }

    AZ_MATH_FORCE_INLINE const Vector4 Vector4::GetReciprocal() const  { return Vector4(1.0f) / *this; }

    AZ_MATH_FORCE_INLINE const Vector4 operator*(const VectorFloat& multiplier, const Vector4& rhs)
    {
        return rhs * multiplier;
    }
}
