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
#include <AzCore/Math/MathUtils.h>

namespace AZ
{
    AZ_MATH_FORCE_INLINE Vector3::Vector3(const VectorFloat& x)
        : m_x(x)
        , m_y(x)
        , m_z(x)                                        { }
    AZ_MATH_FORCE_INLINE Vector3::Vector3(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z)
        : m_x(x)
        , m_y(y)
        , m_z(z)                        { }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateZero()            { return Vector3(0.0f); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateOne()         { return Vector3(1.0f); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateFromFloat3(const float values[])
    {
        Vector3 result;
        result.Set(values);
        return result;
    }

    AZ_MATH_FORCE_INLINE void Vector3::StoreToFloat3(float* values) const
    {
        values[0] = m_x;
        values[1] = m_y;
        values[2] = m_z;
    }
    AZ_MATH_FORCE_INLINE void Vector3::StoreToFloat4(float* values) const
    {
        values[0] = m_x;
        values[1] = m_y;
        values[2] = m_z;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetX() const    { return m_x; }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetY() const    { return m_y; }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetZ() const    { return m_z; }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetElement(int index) const
    {
        AZ_Assert((index >= 0) && (index < 3), "Invalid index for component access!\n");
        if (index == 0)
        {
            return m_x;
        }
        else if (index == 1)
        {
            return m_y;
        }
        else
        {
            return m_z;
        }
    }
    AZ_MATH_FORCE_INLINE void Vector3::SetX(const VectorFloat& x)   { m_x = x; }
    AZ_MATH_FORCE_INLINE void Vector3::SetY(const VectorFloat& y)   { m_y = y; }
    AZ_MATH_FORCE_INLINE void Vector3::SetZ(const VectorFloat& z)   { m_z = z; }
    AZ_MATH_FORCE_INLINE void Vector3::Set(const VectorFloat& x)    { m_x = x; m_y = x; m_z = x; }
    AZ_MATH_FORCE_INLINE void Vector3::SetElement(int index, const VectorFloat& v)
    {
        AZ_Assert((index >= 0) && (index < 3), "Invalid index for component access!\n");
        if (index == 0)
        {
            m_x = v;
        }
        else if (index == 1)
        {
            m_y = v;
        }
        else
        {
            m_z = v;
        }
    }
    AZ_MATH_FORCE_INLINE void Vector3::Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z)        { m_x = x; m_y = y; m_z = z; }
    AZ_MATH_FORCE_INLINE void Vector3::Set(const float values[])    { m_x = values[0]; m_y = values[1]; m_z = values[2]; }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLength() const       { return GetLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthApprox() const { return GetLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthExact() const  { return sqrtf(GetLengthSq()); }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthReciprocal() const       { return GetLengthReciprocalExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthReciprocalApprox() const { return GetLengthReciprocalExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::GetLengthReciprocalExact() const  { return 1.0f / GetLength(); }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalized() const       { return GetNormalizedExact(); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedApprox() const { return GetNormalizedExact(); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedExact() const  { return (*this) / GetLength(); }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedSafe(const VectorFloat& tolerance) const { return GetNormalizedSafeExact(tolerance); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedSafeApprox(const VectorFloat& tolerance) const { return GetNormalizedSafeExact(tolerance); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetNormalizedSafeExact(const VectorFloat& tolerance) const
    {
        float length = GetLength();
        if (length <= tolerance)
        {
            return Vector3(1.0f, 0.0f, 0.0f);
        }
        else
        {
            return (*this) / length;
        }
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeWithLength()       { return NormalizeWithLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeWithLengthApprox() { return NormalizeWithLengthExact(); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeWithLengthExact()
    {
        float length = GetLength();
        (*this) *= (1.0f / length);
        return length;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeSafeWithLength(const VectorFloat& tolerance) { return NormalizeSafeWithLengthExact(tolerance); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeSafeWithLengthApprox(const VectorFloat& tolerance) { return NormalizeSafeWithLengthExact(tolerance); }
    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::NormalizeSafeWithLengthExact(const VectorFloat& tolerance)
    {
        float length = GetLength();
        if (length <= tolerance)
        {
            Set(1.0f, 0.0f, 0.0f);
        }
        else
        {
            (*this) *= (1.0f / length);
        }

        return length;
    }

    AZ_MATH_FORCE_INLINE bool Vector3::IsNormalized(const VectorFloat& tolerance) const
    {
        return (fabsf(GetLengthSq() - 1.0f) <= tolerance);
    }

    AZ_MATH_FORCE_INLINE void Vector3::SetLength(const VectorFloat& length)       { SetLengthExact(length); }
    AZ_MATH_FORCE_INLINE void Vector3::SetLengthApprox(const VectorFloat& length) { SetLengthExact(length); }
    AZ_MATH_FORCE_INLINE void Vector3::SetLengthExact(const VectorFloat& length)
    {
        float scale = length / GetLength();
        m_x *= scale;
        m_y *= scale;
        m_z *= scale;
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::Lerp(const Vector3& dest, const VectorFloat& t) const
    {
        return (*this) * (1.0f - t) + dest * t;
    }

    AZ_MATH_FORCE_INLINE const VectorFloat Vector3::Dot(const Vector3& rhs) const
    {
        return (m_x * rhs.m_x + m_y * rhs.m_y + m_z * rhs.m_z);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::Cross(const Vector3& rhs) const
    {
        return Vector3(m_y * rhs.m_z - m_z * rhs.m_y, m_z * rhs.m_x - m_x * rhs.m_z, m_x * rhs.m_y - m_y * rhs.m_x);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CrossXAxis() const  { return Vector3(0.0f, m_z, -m_y); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CrossYAxis() const  { return Vector3(-m_z, 0.0f, m_x); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CrossZAxis() const  { return Vector3(m_y, -m_x, 0.0f); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::XAxisCross() const  { return Vector3(0.0f, -m_z, m_y); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::YAxisCross() const  { return Vector3(m_z, 0.0f, -m_x); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::ZAxisCross() const  { return Vector3(-m_y, m_x, 0.0f); }

    AZ_MATH_FORCE_INLINE bool Vector3::IsClose(const Vector3& v, const VectorFloat& tolerance) const
    {
        Vector3 dist = (v - (*this)).GetAbs();
        return dist.IsLessEqualThan(Vector3(tolerance));
    }

    AZ_MATH_FORCE_INLINE bool Vector3::operator==(const Vector3& rhs) const
    {
        return ((m_x == rhs.m_x) && (m_y == rhs.m_y) && (m_z == rhs.m_z));
    }

    AZ_MATH_FORCE_INLINE bool Vector3::operator!=(const Vector3& rhs) const
    {
        return ((m_x != rhs.m_x) || (m_y != rhs.m_y) || (m_z != rhs.m_z));
    }

    AZ_MATH_FORCE_INLINE bool Vector3::IsLessThan(const Vector3& rhs) const         { return (GetX() < rhs.GetX()) && (GetY() < rhs.GetY()) && (GetZ() < rhs.GetZ()); }
    AZ_MATH_FORCE_INLINE bool Vector3::IsLessEqualThan(const Vector3& rhs) const        { return (GetX() <= rhs.GetX()) && (GetY() <= rhs.GetY()) && (GetZ() <= rhs.GetZ()); }
    AZ_MATH_FORCE_INLINE bool Vector3::IsGreaterThan(const Vector3& rhs) const      { return (GetX() > rhs.GetX()) && (GetY() > rhs.GetY()) && (GetZ() > rhs.GetZ()); }
    AZ_MATH_FORCE_INLINE bool Vector3::IsGreaterEqualThan(const Vector3& rhs) const { return (GetX() >= rhs.GetX()) && (GetY() >= rhs.GetY()) && (GetZ() >= rhs.GetZ()); }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetMin(const Vector3& v) const
    {
        return Vector3(AZ::GetMin(m_x, v.m_x), AZ::GetMin(m_y, v.m_y), AZ::GetMin(m_z, v.m_z));
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetMax(const Vector3& v) const
    {
        return Vector3(AZ::GetMax(m_x, v.m_x), AZ::GetMax(m_y, v.m_y), AZ::GetMax(m_z, v.m_z));
    }

    // operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateSelectCmpEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
        return Vector3((cmp1.m_x == cmp2.m_x) ? vA.m_x : vB.m_x,
            (cmp1.m_y == cmp2.m_y) ? vA.m_y : vB.m_y,
            (cmp1.m_z == cmp2.m_z) ? vA.m_z : vB.m_z);
    }

    // operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateSelectCmpGreaterEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
        return Vector3((cmp1.m_x >= cmp2.m_x) ? vA.m_x : vB.m_x,
            (cmp1.m_y >= cmp2.m_y) ? vA.m_y : vB.m_y,
            (cmp1.m_z >= cmp2.m_z) ? vA.m_z : vB.m_z);
    }

    // operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::CreateSelectCmpGreater(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB)
    {
        return Vector3((cmp1.m_x > cmp2.m_x) ? vA.m_x : vB.m_x,
            (cmp1.m_y > cmp2.m_y) ? vA.m_y : vB.m_y,
            (cmp1.m_z > cmp2.m_z) ? vA.m_z : vB.m_z);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetAngleMod() const
    {
        return Vector3((m_x >= 0.0f) ? (fmodf(m_x + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(m_x - Constants::Pi, Constants::TwoPi) + Constants::Pi),
            (m_y >= 0.0f) ? (fmodf(m_y + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(m_y - Constants::Pi, Constants::TwoPi) + Constants::Pi),
            (m_z >= 0.0f) ? (fmodf(m_z + Constants::Pi, Constants::TwoPi) - Constants::Pi) : (fmodf(m_z - Constants::Pi, Constants::TwoPi) + Constants::Pi));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetSin() const  { return Vector3(sinf(m_x), sinf(m_y), sinf(m_z)); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetCos() const  { return Vector3(cosf(m_x), cosf(m_y), cosf(m_z)); }

    AZ_MATH_FORCE_INLINE void Vector3::GetSinCos(Vector3& sin, Vector3& cos) const
    {
        sin.Set(sinf(m_x), sinf(m_y), sinf(m_z));
        cos.Set(cosf(m_x), cosf(m_y), cosf(m_z));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetAbs() const
    {
        return Vector3(fabsf(m_x), fabsf(m_y), fabsf(m_z));
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetReciprocal() const       { return GetReciprocalExact(); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetReciprocalApprox() const { return GetReciprocalExact(); }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetReciprocalExact() const  { return Vector3(1.0f) / *this; }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator-() const
    {
        return Vector3(-m_x, -m_y, -m_z);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator+(const Vector3& rhs) const
    {
        return Vector3(m_x + rhs.m_x, m_y + rhs.m_y, m_z + rhs.m_z);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator-(const Vector3& rhs) const
    {
        return Vector3(m_x - rhs.m_x, m_y - rhs.m_y, m_z - rhs.m_z);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator*(const Vector3& rhs) const
    {
        return Vector3(m_x * rhs.m_x, m_y * rhs.m_y, m_z * rhs.m_z);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator/(const Vector3& rhs) const
    {
        return Vector3(m_x / rhs.m_x, m_y / rhs.m_y, m_z / rhs.m_z);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator*(const VectorFloat& multiplier) const
    {
        return Vector3(m_x * multiplier, m_y * multiplier, m_z * multiplier);
    }
    AZ_MATH_FORCE_INLINE const Vector3 Vector3::operator/(const VectorFloat& divisor) const
    {
        return Vector3(m_x / divisor, m_y / divisor, m_z / divisor);
    }

    AZ_MATH_FORCE_INLINE const Vector3 Vector3::GetMadd(const Vector3& mul, const Vector3& add)
    {
        return Vector3(m_x * mul.m_x + add.m_x,
            m_y * mul.m_y + add.m_y,
            m_z * mul.m_z + add.m_z);
    }

    AZ_MATH_FORCE_INLINE bool Vector3::IsPerpendicular(const Vector3& v, const VectorFloat& tolerance) const
    {
        return (fabsf(Dot(v)) <= tolerance);
    }

    AZ_MATH_FORCE_INLINE const Vector3 operator*(const VectorFloat& multiplier, const Vector3& rhs)
    {
        return Vector3(rhs.GetX() * multiplier, rhs.GetY() * multiplier, rhs.GetZ() * multiplier);
    }
}
