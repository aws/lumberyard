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
#ifndef AZCORE_MATH_VECTOR2_H
#define AZCORE_MATH_VECTOR2_H 1

#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <math.h>

namespace AZ
{
    /**
    * 2-dimensional vector class
    */
    class Vector2
    {
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Vector2);
        AZ_TYPE_INFO(Vector2, "{3D80F623-C85C-4741-90D0-E4E66164E6BF}")

        Vector2()                                                                   { }
        explicit Vector2(float x)
            : m_x(x)
            , m_y(x)                                  { }
        explicit Vector2(float x, float y)
            : m_x(x)
            , m_y(y)                         { }

        static AZ_MATH_FORCE_INLINE const Vector2 CreateZero()          { return Vector2(0.0f); }
        static AZ_MATH_FORCE_INLINE const Vector2 CreateOne()           { return Vector2(1.0f); }

        ///Sets components from an array of 2 floats, stored in xy order
        static AZ_MATH_FORCE_INLINE const Vector2 CreateFromFloat2(const float* values) { return Vector2(values[0], values[1]); }

        static AZ_MATH_FORCE_INLINE const Vector2 CreateAxisX(float length = 1.0f) { return Vector2(length, 0.0f); }
        static AZ_MATH_FORCE_INLINE const Vector2 CreateAxisY(float length = 1.0f) { return Vector2(0.0f, length); }

        ///operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        static AZ_MATH_FORCE_INLINE const Vector2 CreateSelectCmpEqual(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB)
        {
            return Vector2((cmp1.m_x == cmp2.m_x) ? vA.m_x : vB.m_x,
                (cmp1.m_y == cmp2.m_y) ? vA.m_y : vB.m_y);
        }

        ///operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        static AZ_MATH_FORCE_INLINE const Vector2 CreateSelectCmpGreaterEqual(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB)
        {
            return Vector2((cmp1.m_x >= cmp2.m_x) ? vA.m_x : vB.m_x,
                (cmp1.m_y >= cmp2.m_y) ? vA.m_y : vB.m_y);
        }

        ///operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        static AZ_MATH_FORCE_INLINE const Vector2 CreateSelectCmpGreater(const Vector2& cmp1, const Vector2& cmp2, const Vector2& vA, const Vector2& vB)
        {
            return Vector2((cmp1.m_x > cmp2.m_x) ? vA.m_x : vB.m_x,
                (cmp1.m_y > cmp2.m_y) ? vA.m_y : vB.m_y);
        }

        //===============================================================
        // Store
        //===============================================================

        ///Stores the vector to an array of 2 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT equired.
        AZ_MATH_FORCE_INLINE void StoreToFloat2(float* values) const        { values[0] = m_x; values[1] = m_y; }

        AZ_MATH_FORCE_INLINE float GetX() const { return m_x; }
        AZ_MATH_FORCE_INLINE float GetY() const { return m_y; }
        AZ_MATH_FORCE_INLINE float GetElement(int index) const
        {
            AZ_Assert((index >= 0) && (index < 2), "Invalid index for component access!\n");
            return reinterpret_cast<const float*>(this)[index];
        }
        AZ_MATH_FORCE_INLINE void SetX(float x) { m_x = x; }
        AZ_MATH_FORCE_INLINE void SetY(float y) { m_y = y; }
        AZ_MATH_FORCE_INLINE void Set(float x)  { m_x = x; m_y = x; }
        AZ_MATH_FORCE_INLINE void Set(float x, float y)     { m_x = x; m_y = y; }
        AZ_MATH_FORCE_INLINE void SetElement(int index, float value)
        {
            AZ_Assert((index >= 0) && (index < 2), "Invalid index for component access!\n");
            float* farr = reinterpret_cast<float*>(this);
            farr[index] = value;
        }

        /** @{ */
        /**
        * Indexed access using operator()
        */
        AZ_MATH_FORCE_INLINE float operator()(int index) const { return GetElement(index); }
        /** @} */

        AZ_MATH_FORCE_INLINE float GetLengthSq() const  { return (m_x * m_x + m_y * m_y); }
        AZ_MATH_FORCE_INLINE float GetLength() const        { return sqrtf(GetLengthSq()); }

        AZ_MATH_FORCE_INLINE const Vector2 GetNormalized() const
        {
            float lengthRecip = 1.0f / GetLength();
            return (*this) * lengthRecip;
        }
        AZ_MATH_FORCE_INLINE const Vector2 GetNormalizedSafe(float tolerance = 0.0001f) const
        {
            float length = GetLength();
            if (length <= tolerance)
            {
                return Vector2(1.0f, 0.0f);
            }
            else
            {
                return (*this) / length;
            }
        }
        AZ_MATH_FORCE_INLINE void Normalize() { NormalizeWithLength(); }
        AZ_MATH_FORCE_INLINE float NormalizeWithLength()
        {
            float length = GetLength();
            (*this) *= (1.0f / length);
            return length;
        }

        /************************************************************************
        *
        ************************************************************************/
        AZ_MATH_FORCE_INLINE void NormalizeSafe(float tolerance = 0.0001f) { NormalizeSafeWithLength(tolerance); }
        AZ_MATH_FORCE_INLINE float NormalizeSafeWithLength(float tolerance = 0.0001f)
        {
            float length = GetLength();
            if (length <= tolerance)
            {
                Set(1.0f, 0.0f);
            }
            else
            {
                (*this) *= (1.0f / length);
            }

            return length;
        }

        AZ_MATH_FORCE_INLINE void SetLength(float length)
        {
            float scale = length / GetLength();
            m_x *= scale;
            m_y *= scale;
        }
        AZ_MATH_FORCE_INLINE bool IsNormalized(float tolerance = 0.01f) const
        {
            return (fabsf(GetLengthSq() - 1.0f) <= tolerance);
        }

        /**
        * Returns squared distance to another Vector2
        */
        AZ_MATH_FORCE_INLINE float GetDistanceSq(const Vector2& v) const
        {
            return ((*this) - v).GetLengthSq();
        }

        /**
        * Returns distance to another Vector2
        */
        AZ_MATH_FORCE_INLINE float GetDistance(const Vector2& v) const
        {
            return ((*this) - v).GetLength();
        }

        /**
        * Linear interpolation between this vector and a destination.
        * @return (*this)*(1-t) + dest*t
        */
        AZ_MATH_FORCE_INLINE const Vector2 Lerp(const Vector2& dest, float t) const
        {
            return (*this) * (1.0f - t) + dest * t;
        }

        /**
        * Spherical linear interpolation between normalized vectors.
        * Interpolates along the great circle connecting the two vectors.
        */
        const Vector2 Slerp(const Vector2& dest, float t) const;

        /**
         * Gets perpendicular vector, i.e. rotates through 90 degrees. The positive rotation direction is defined such that
         * the x-axis is rotated into the y-axis.
         */
        AZ_MATH_FORCE_INLINE const Vector2 GetPerpendicular() const
        {
            return Vector2(-m_y, m_x);
        }

        /**
        * Checks the vector is equal to another within a floating point tolerance
        */
        AZ_MATH_FORCE_INLINE bool IsClose(const Vector2& v, float tolerance = 0.001f) const
        {
            return ((fabsf(v.m_x - m_x) <= tolerance) && (fabsf(v.m_y - m_y) <= tolerance));
        }

        AZ_MATH_FORCE_INLINE bool IsZero(float tolerance = AZ_FLT_EPSILON) const { return IsClose(Vector2::CreateZero(), tolerance); }

        AZ_MATH_FORCE_INLINE bool operator==(const Vector2& rhs) const
        {
            return ((m_x == rhs.m_x) && (m_y == rhs.m_y));
        }

        AZ_MATH_FORCE_INLINE bool operator!=(const Vector2& rhs) const
        {
            return ((m_x != rhs.m_x) || (m_y != rhs.m_y));
        }

        /**
        * Comparison functions, not implemented as operators since that would probably be a little dangerous. These
        * functions return true only if all components pass the comparison test.
        */
        /*@{*/
        AZ_MATH_FORCE_INLINE bool IsLessThan(const Vector2& v) const
        {
            return (m_x < v.m_x) && (m_y < v.m_y);
        }
        AZ_MATH_FORCE_INLINE bool IsLessEqualThan(const Vector2& v) const
        {
            return (m_x <= v.m_x) && (m_y <= v.m_y);
        }
        AZ_MATH_FORCE_INLINE bool IsGreaterThan(const Vector2& v) const
        {
            return (m_x > v.m_x) && (m_y > v.m_y);
        }
        AZ_MATH_FORCE_INLINE bool IsGreaterEqualThan(const Vector2& v) const
        {
            return (m_x >= v.m_x) && (m_y >= v.m_y);
        }
        /*@}*/

        /**
        * Min/Max functions, operate on each component individually, result will be a new Vector2
        */
        /*@{*/
        AZ_MATH_FORCE_INLINE const Vector2 GetMin(const Vector2& v) const
        {
            return Vector2(AZ::GetMin(m_x, v.m_x), AZ::GetMin(m_y, v.m_y));
        }
        AZ_MATH_FORCE_INLINE const Vector2 GetMax(const Vector2& v) const
        {
            return Vector2(AZ::GetMax(m_x, v.m_x), AZ::GetMax(m_y, v.m_y));
        }
        AZ_MATH_FORCE_INLINE const Vector2 GetClamp(const Vector2& min, const Vector2& max) const
        {
            return Vector2(AZ::GetClamp(m_x, min.m_x, max.m_x), AZ::GetClamp(m_y, min.m_y, max.m_y));
        }
        /*@}*/

        /************************************************************************
        * Vector compare
        ************************************************************************/

        /**
        * vector select operation (vR = (vCmp==0) ? vA : vB) this is a special case
        * (from CmpEqual) and it's a lot faster
        */
        AZ_MATH_FORCE_INLINE const Vector2 GetSelect(const Vector2& vCmp, const Vector2& vB)
        {
            return Vector2((vCmp.m_x == 0.0f) ? m_x : vB.m_x,
                (vCmp.m_y == 0.0f) ? m_y : vB.m_y);
        }

        AZ_MATH_FORCE_INLINE void Select(const Vector2& vCmp, const Vector2& vB)
        {
            *this = GetSelect(vCmp, vB);
        }

        AZ_MATH_FORCE_INLINE const Vector2 GetAbs() const
        {
            return Vector2(fabsf(m_x), fabsf(m_y));
        }

        AZ_MATH_FORCE_INLINE Vector2& operator+=(const Vector2& rhs)
        {
            m_x += rhs.m_x;
            m_y += rhs.m_y;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector2& operator-=(const Vector2& rhs)
        {
            m_x -= rhs.m_x;
            m_y -= rhs.m_y;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector2& operator*=(const Vector2& rhs)
        {
            m_x *= rhs.m_x;
            m_y *= rhs.m_y;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector2& operator/=(const Vector2& rhs)
        {
            m_x /= rhs.m_x;
            m_y /= rhs.m_y;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector2& operator*=(float multiplier)
        {
            m_x *= multiplier;
            m_y *= multiplier;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector2& operator/=(float divisor)
        {
            m_x /= divisor;
            m_y /= divisor;
            return *this;
        }

        AZ_MATH_FORCE_INLINE const Vector2 operator-() const
        {
            return Vector2(-m_x, -m_y);
        }

        AZ_MATH_FORCE_INLINE const Vector2 operator+(const Vector2& rhs) const
        {
            return Vector2(m_x + rhs.m_x, m_y + rhs.m_y);
        }
        AZ_MATH_FORCE_INLINE const Vector2 operator-(const Vector2& rhs) const
        {
            return Vector2(m_x - rhs.m_x, m_y - rhs.m_y);
        }
        AZ_MATH_FORCE_INLINE const Vector2 operator*(const Vector2& rhs) const
        {
            return Vector2(m_x * rhs.m_x, m_y * rhs.m_y);
        }
        AZ_MATH_FORCE_INLINE const Vector2 operator*(float multiplier) const
        {
            return Vector2(m_x * multiplier, m_y * multiplier);
        }
        AZ_MATH_FORCE_INLINE const Vector2 operator/(float divisor) const
        {
            return Vector2(m_x / divisor, m_y / divisor);
        }
        AZ_MATH_FORCE_INLINE const Vector2 operator/(const Vector2& rhs) const
        {
            return Vector2(m_x / rhs.m_x, m_y / rhs.m_y);
        }

        /**
        * Dot product of two vectors
        */
        AZ_MATH_FORCE_INLINE float Dot(const Vector2& rhs) const
        {
            return (m_x * rhs.m_x + m_y * rhs.m_y);
        }

        //////////////////////////////////////////////////////////////////////////
        // perform multiply-add operator (this = this * mul + add)
        AZ_MATH_FORCE_INLINE const Vector2 GetMadd(const Vector2& mul, const Vector2& add)
        {
            return Vector2(m_x * mul.m_x + add.m_x, m_y * mul.m_y + add.m_y);
        }

        AZ_MATH_FORCE_INLINE void Madd(const Vector2& mul, const Vector2& add)
        {
            *this = GetMadd(mul, add);
        }
        //////////////////////////////////////////////////////////////////////////

        /// Project vector onto another. P = (a.Dot(b) / b.Dot(b)) * b
        AZ_MATH_FORCE_INLINE void           Project(const Vector2& rhs)                 { *this = rhs * (Dot(rhs) / rhs.Dot(rhs)); }
        /// Project vector onto a normal (faster function). P = (v.Dot(Normal) * normal)
        AZ_MATH_FORCE_INLINE void           ProjectOnNormal(const Vector2& normal)      { AZ_Assert(normal.IsNormalized(), "This normal is not a normalized!"); *this = normal * Dot(normal); }

        /// Project this vector onto another and return the resulting projection. P = (a.Dot(b) / b.Dot(b)) * b
        AZ_MATH_FORCE_INLINE Vector2        GetProjected(const Vector2& rhs) const      { return rhs * (Dot(rhs) / rhs.Dot(rhs)); }
        /// Project this vector onto a normal and return the resulting projection. P = (v.Dot(Normal) * normal)
        AZ_MATH_FORCE_INLINE Vector2        GetProjectedOnNormal(const Vector2& normal) {  AZ_Assert(normal.IsNormalized(), "This normal is not a normalized!"); return normal * Dot(normal); }

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return IsFiniteFloat(GetX()) && IsFiniteFloat(GetY()); }

    private:
        float m_x, m_y;
    };

    /**
    * Allows pre-multiplying by a float
    */
    AZ_MATH_FORCE_INLINE const Vector2 operator*(float multiplier, const Vector2& rhs)
    {
        return Vector2(rhs.GetX() * multiplier, rhs.GetY() * multiplier);
    }
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Vector2);
#endif

#endif
#pragma once