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
    AZ_MATH_FORCE_INLINE Quaternion::Quaternion(const VectorFloat& x) { Set(x); }
    AZ_MATH_FORCE_INLINE Quaternion::Quaternion(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w) { Set(x, y, z, w); }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::CreateZero()      { return Quaternion(0.0f); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::CreateFromFloat4(const float* values)
    {
        return Quaternion(values[0], values[1], values[2], values[3]);
    }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::CreateFromVector3AndValue(const Vector3& v, const VectorFloat& w)
    {
        Quaternion result;
        result.Set(v, w);
        return result;
    }
    AZ_MATH_FORCE_INLINE void Quaternion::StoreToFloat4(float* values) const
    {
        values[0] = m_x;
        values[1] = m_y;
        values[2] = m_z;
        values[3] = m_w;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetX() const { return m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetY() const { return m_y; }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetZ() const { return m_z; }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetW() const { return m_w; }
    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::GetElement(int index) const
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
    AZ_MATH_FORCE_INLINE void Quaternion::SetX(const VectorFloat& x)    { m_x = x; }
    AZ_MATH_FORCE_INLINE void Quaternion::SetY(const VectorFloat& y)    { m_y = y; }
    AZ_MATH_FORCE_INLINE void Quaternion::SetZ(const VectorFloat& z)    { m_z = z; }
    AZ_MATH_FORCE_INLINE void Quaternion::SetW(const VectorFloat& w)    { m_w = w; }
    AZ_MATH_FORCE_INLINE void Quaternion::SetElement(int index, const VectorFloat& v)
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
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const VectorFloat& x) { m_x = x; m_y = x; m_z = x; m_w = x; }
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w)       { m_x = x; m_y = y; m_z = z; m_w = w; }
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const Vector3& v, const VectorFloat& w)       { m_x = v.GetX(); m_y = v.GetY(); m_z = v.GetZ(); m_w = w; }
    AZ_MATH_FORCE_INLINE void Quaternion::Set(const float values[]) { m_x = values[0]; m_y = values[1]; m_z = values[2]; m_w = values[3]; }

    AZ_MATH_FORCE_INLINE bool Quaternion::IsIdentity(const VectorFloat& tolerance) const
    {
        return (fabsf(m_x) <= tolerance) && (fabsf(m_y) <= tolerance) && (fabsf(m_z) <= tolerance) && (fabsf(m_w) >= (1.0f - tolerance));
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::GetConjugate() const
    {
        return Quaternion(-m_x, -m_y, -m_z, m_w);
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Quaternion::Dot(const Quaternion& q) const
    {
        return m_x * q.m_x + m_y * q.m_y + m_z * q.m_z + m_w * q.m_w;
    }

    inline const VectorFloat Quaternion::GetLength() const                  { return sqrtf(Dot(*this)); }
    inline const VectorFloat Quaternion::GetLengthApprox() const            { return sqrtf(Dot(*this)); }
    inline const VectorFloat Quaternion::GetLengthExact() const             { return sqrtf(Dot(*this)); }

    inline const VectorFloat Quaternion::GetLengthReciprocal() const        { return 1.0f / GetLength(); }
    inline const VectorFloat Quaternion::GetLengthReciprocalApprox() const  { return 1.0f / GetLength(); }
    inline const VectorFloat Quaternion::GetLengthReciprocalExact() const   { return 1.0f / GetLength(); }

    inline const Quaternion Quaternion::GetNormalized() const               { return (*this) / GetLength(); }
    inline const Quaternion Quaternion::GetNormalizedApprox() const         { return (*this) / GetLength(); }
    inline const Quaternion Quaternion::GetNormalizedExact() const          { return (*this) / GetLength(); }

    inline const VectorFloat Quaternion::NormalizeWithLength()
    {
        VectorFloat length = GetLength();
        *this /= length;
        return length;
    }
    inline const VectorFloat Quaternion::NormalizeWithLengthApprox()
    {
        VectorFloat length = GetLengthApprox();
        (*this) /= length;
        return length;
    }
    inline const VectorFloat Quaternion::NormalizeWithLengthExact()
    {
        VectorFloat length = GetLengthExact();
        (*this) /= length;
        return length;
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::Lerp(const Quaternion& dest, const VectorFloat& t) const
    {
        return (*this) * (1.0f - t) + dest * t;
    }

    AZ_MATH_FORCE_INLINE bool Quaternion::IsClose(const Quaternion& q, const VectorFloat& tolerance) const
    {
        return ((fabsf(q.m_x - m_x) <= tolerance) && (fabsf(q.m_y - m_y) <= tolerance) && (fabsf(q.m_z - m_z) <= tolerance) && (fabsf(q.m_w - m_w) <= tolerance));
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator-() const { return Quaternion(-m_x, -m_y, -m_z, -m_w); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator+(const Quaternion& q) const  { return Quaternion(m_x + q.m_x, m_y + q.m_y, m_z + q.m_z, m_w + q.m_w); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator-(const Quaternion& q) const  { return Quaternion(m_x - q.m_x, m_y - q.m_y, m_z - q.m_z, m_w - q.m_w); }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator*(const Quaternion& q) const
    {
        const Vector3 v1 = Vector3(m_x, m_y, m_z);
        const Vector3 v2 = Vector3(q.m_x, q.m_y, q.m_z);
        Vector3 cross = v1.Cross(v2);
        return Quaternion(m_w * q.m_x + q.m_w * m_x + cross.GetX(),
            m_w * q.m_y + q.m_w * m_y + cross.GetY(),
            m_w * q.m_z + q.m_w * m_z + cross.GetZ(),
            m_w * q.m_w - v1.Dot(v2));
    }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator*(const VectorFloat& multiplier) const
    {
        return Quaternion(m_x * multiplier, m_y * multiplier, m_z * multiplier, m_w * multiplier);
    }
    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::operator/(const VectorFloat& divisor) const
    {
        return Quaternion(m_x / divisor, m_y / divisor, m_z / divisor, m_w / divisor);
    }

    AZ_MATH_FORCE_INLINE bool Quaternion::operator==(const Quaternion& rhs) const
    {
        return ((m_x == rhs.m_x) && (m_y == rhs.m_y) && (m_z == rhs.m_z) && (m_w == rhs.m_w));
    }
    AZ_MATH_FORCE_INLINE bool Quaternion::operator!=(const Quaternion& rhs) const
    {
        return ((m_x != rhs.m_x) || (m_y != rhs.m_y) || (m_z != rhs.m_z) || (m_w != rhs.m_w));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Quaternion::operator*(const Vector3& v) const
    {
        //inverse must come last, this ensures associativity, (q1*q2)*v = q1*(q2*v)
        Quaternion result = (*this) * Quaternion::CreateFromVector3(v) * GetInverseFast();
        return Vector3(result.GetX(), result.GetY(), result.GetZ());
    }

    /**
     * Allows pre-multiplying by a float
     */
    AZ_MATH_FORCE_INLINE const Quaternion operator*(const VectorFloat& multiplier, const Quaternion& rhs)
    {
        return Quaternion(rhs.GetX() * multiplier, rhs.GetY() * multiplier, rhs.GetZ() * multiplier, rhs.GetW() * multiplier);
    }

    AZ_MATH_FORCE_INLINE const Quaternion Quaternion::GetAbs() const
    {
        return Quaternion(fabsf(m_x), fabsf(m_y), fabsf(m_z), fabsf(m_z));
    }
}
