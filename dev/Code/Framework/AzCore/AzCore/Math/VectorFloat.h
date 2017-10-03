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
#ifndef AZCORE_MATH_VECTORFLOAT_H
#define AZCORE_MATH_VECTORFLOAT_H 1

#include <AzCore/base.h>
#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class Vector3;
    class Vector4;
    class Quaternion;
    class Transform;
    class Matrix3x3;

    /**
     * Represents a float stored in a vector register, improves performance on some platforms when converting from
     * vector to float is expensive. Implicit conversion to and from float is allowed.
     *
     * Note that the size and alignment of this class is different in SIMD and FPU implementations, so DO NOT store
     * it in data.
     */
    class VectorFloat
    {
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(VectorFloat);
        AZ_TYPE_INFO(VectorFloat, "{EEA8B695-51EE-4717-B818-4070C6DA849D}")

        ///Default constructor, VectorFloat is uninitialized
        VectorFloat() { }

        ///Constructs from a float, implicit conversion is allowed. If you are loading a float from memory then
        ///CreateFromFloat will usually be faster.
        VectorFloat(float value);

        ///Create a VectorFloat initialized to zero, potentially more efficient than converting from 0.0f
        static const VectorFloat CreateZero();

        ///Create a VectorFloat initialized to one, potentially more efficient than converting from 1.0f
        static const VectorFloat CreateOne();

        ///Create a VectorFloat by loading a float from memory. The float must have natural 4 byte alignment (16 byte
        ///alignment is NOT required!)
        static const VectorFloat CreateFromFloat(const float* ptr);

        ///operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x
        static const VectorFloat CreateSelectCmpEqual(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB);

        ///operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x )
        static const VectorFloat CreateSelectCmpGreaterEqual(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB);

        ///operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x )
        static const VectorFloat CreateSelectCmpGreater(const VectorFloat& cmp1, const VectorFloat& cmp2, const VectorFloat& vA, const VectorFloat& vB);

#if defined(AZ_SIMD)
        AZ_MATH_FORCE_INLINE VectorFloat(const VectorFloat& v)
            : m_value(v.m_value)            {}
        AZ_MATH_FORCE_INLINE VectorFloat& operator= (const VectorFloat& rhs)        { m_value = rhs.m_value; return *this;  }
#endif

        ///Implicit conversion to float, prefer StoreToFloat if the result is to be written to memory.
        operator float() const;

        ///Efficient store to float in memory, prefer this to using the automatic conversion if you are storing to
        ///memory.
        void StoreToFloat(float* ptr) const;

        //Standard operators. These must be members, because non-members would cause ambiguous overloads
        const VectorFloat operator-() const;
        const VectorFloat operator+(const VectorFloat& rhs) const;
        const VectorFloat operator-(const VectorFloat& rhs) const;
        const VectorFloat operator*(const VectorFloat& rhs) const;
        const VectorFloat operator/(const VectorFloat& rhs) const;

        AZ_MATH_FORCE_INLINE VectorFloat& operator+=(const VectorFloat& rhs)        { *this = *this + rhs; return *this; }
        AZ_MATH_FORCE_INLINE VectorFloat& operator-=(const VectorFloat& rhs)        { *this = *this - rhs; return *this; }
        AZ_MATH_FORCE_INLINE VectorFloat& operator*=(const VectorFloat& rhs)        { *this = *this * rhs; return *this; }
        AZ_MATH_FORCE_INLINE VectorFloat& operator/=(const VectorFloat& rhs)        { *this = *this / rhs; return *this; }

        //float operators are required to avoid ambiguous overloads
        AZ_MATH_FORCE_INLINE const VectorFloat operator+(float rhs) const   { return (*this) + VectorFloat(rhs); }
        AZ_MATH_FORCE_INLINE const VectorFloat operator-(float rhs) const   { return (*this) - VectorFloat(rhs); }
        AZ_MATH_FORCE_INLINE const VectorFloat operator*(float rhs) const   { return (*this) * VectorFloat(rhs); }
        AZ_MATH_FORCE_INLINE const VectorFloat operator/(float rhs) const   { return (*this) / VectorFloat(rhs); }

        //===============================================================
        // Min/Max
        //===============================================================

        ///Min/Max functions
        /*@{*/
        const VectorFloat GetMin(const VectorFloat& v) const;
        const VectorFloat GetMax(const VectorFloat& v) const;
        AZ_MATH_FORCE_INLINE const VectorFloat GetClamp(const VectorFloat& min, const VectorFloat& max) const   { return GetMin(max).GetMax(min); }
        /*@}*/

        //===============================================================
        // Trigonometry
        //===============================================================

        ///Gets the sine.
        const VectorFloat GetSin() const;

        ///Gets the cosine.
        const VectorFloat GetCos() const;

        ///Gets the sine and cosine, quicker than calling GetSin and GetCos separately.
        void GetSinCos(VectorFloat& sin, VectorFloat& cos) const;

        ///wraps the angle into the [-pi,pi] range
        const VectorFloat GetAngleMod() const;

        //==========================================================================
        // Comparison functions, not implemented as operators since that would probably be a little dangerous. These
        // functions return true only if all components pass the comparison test.
        //==========================================================================
        bool IsLessThan(const VectorFloat& rhs) const;
        bool IsLessEqualThan(const VectorFloat& rhs) const;
        bool IsGreaterThan(const VectorFloat& rhs) const;
        bool IsGreaterEqualThan(const VectorFloat& rhs) const;

        AZ_MATH_FORCE_INLINE bool operator==(float rhs) const { return static_cast<float>(*this) == rhs; }
        AZ_MATH_FORCE_INLINE bool operator!=(float rhs) const { return static_cast<float>(*this) != rhs; }
        bool operator==(const VectorFloat& rhs) const;
        bool operator!=(const VectorFloat& rhs) const;

        bool IsClose(const VectorFloat& v) const;
        AZ_MATH_FORCE_INLINE bool IsClose(const VectorFloat& v, const VectorFloat& tolerance) const { return ((*this) - v).GetAbs().IsLessEqualThan(tolerance); }
        bool IsZero() const;
        AZ_MATH_FORCE_INLINE bool IsZero(const VectorFloat& tolerance) const { return IsClose(CreateZero(), tolerance); }

        //===============================================================
        // Miscellaneous
        //===============================================================

        ///Returns the absolute value
        const VectorFloat GetAbs() const;

        ///Returns the reciprocal of the value, medium speed and medium accuracy, typically uses a refined estimate
        const VectorFloat GetReciprocal() const;
        ///Returns the reciprocal of the value, fast but low accuracy, uses raw estimate instructions
        const VectorFloat GetReciprocalApprox() const;
        ///Returns the reciprocal of the value, full accuracy
        const VectorFloat GetReciprocalExact() const;

        ///Returns the square root of the value, medium speed and medium accuracy, typically uses a refined estimate
        const VectorFloat GetSqrt() const;
        ///Returns the square root of the value, fast but low accuracy, uses raw estimate instructions
        const VectorFloat GetSqrtApprox() const;
        ///Returns the square root of the value, full accuracy
        const VectorFloat GetSqrtExact() const;

        ///Returns the reciprocal square root of the value, medium speed and medium accuracy, typically uses a refined estimate
        const VectorFloat GetSqrtReciprocal() const;
        ///Returns the reciprocal square root of the value, fast but low accuracy, uses raw estimate instructions
        const VectorFloat GetSqrtReciprocalApprox() const;
        ///Returns the reciprocal square root of the value, full accuracy
        const VectorFloat GetSqrtReciprocalExact() const;

        // Linear interpolation between VecFloat and a destination
        AZ_MATH_FORCE_INLINE const VectorFloat Lerp(const VectorFloat dest, const VectorFloat& t) const { VectorFloat one = VectorFloat::CreateOne(); return (*this) * (one - t) + dest * t; }

        //TODO: optimize properly per platform
        ///Returns +1.0 if the value is greater or equal to 0.0, otherwise returns -1.0
        AZ_MATH_FORCE_INLINE const VectorFloat GetSign() const { VectorFloat one = VectorFloat::CreateOne(); return IsGreaterEqualThan(VectorFloat::CreateZero()) ? one : -one; }

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return IsFiniteFloat(*this); }

    private:
        friend class Vector3;
        friend class Vector4;
        friend class Quaternion;
        friend class Transform;
        friend class Matrix4x4;
        friend class Matrix3x3;
        friend const Matrix3x3 operator*(const VectorFloat& lhs, const Matrix3x3& rhs);

        //construct from vector type, used internally only
        explicit VectorFloat(const SimdVectorType& value);

        #if defined(AZ_SIMD)
        SimdVectorType m_value;
        #else
        float m_x;      //Note that we don't have the same alignment or size as the SIMD version
        #endif
    };

    //float operators for when first argument is a float
    AZ_MATH_FORCE_INLINE  const VectorFloat operator+(float lhs, const VectorFloat& rhs)    { return VectorFloat(lhs) + rhs; }
    AZ_MATH_FORCE_INLINE  const VectorFloat operator-(float lhs, const VectorFloat& rhs)    { return VectorFloat(lhs) - rhs; }
    AZ_MATH_FORCE_INLINE  const VectorFloat operator*(float lhs, const VectorFloat& rhs)    { return VectorFloat(lhs) * rhs; }
    AZ_MATH_FORCE_INLINE  const VectorFloat operator/(float lhs, const VectorFloat& rhs)    { return VectorFloat(lhs) / rhs; }

    extern const VectorFloat g_simdTolerance;
    extern const VectorFloat g_fltMax;
    extern const VectorFloat g_fltEps;
    extern const VectorFloat g_fltEpsSq;

    AZ_MATH_FORCE_INLINE bool VectorFloat::IsClose(const VectorFloat& v) const
    {
        return ((*this) - v).GetAbs().IsLessEqualThan(g_simdTolerance);
    }

    AZ_MATH_FORCE_INLINE bool VectorFloat::IsZero() const
    {
        return ((*this) - CreateZero()).GetAbs().IsLessEqualThan(g_fltEps);
    }

    template<typename T>
    AZ_MATH_FORCE_INLINE void GetSinCos(T a, T& sin, T& cos)
    {
        VectorFloat v(a);
        v.GetSinCos(sin, cos);
    }

}


#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::VectorFloat);
#endif

#if defined(AZ_SIMD_WINDOWS) || defined(AZ_SIMD_XBONE) || defined(AZ_SIMD_PS4) || defined(AZ_SIMD_LINUX) || defined(AZ_SIMD_APPLE_OSX)
    #include <AzCore/Math/Internal/VectorFloatWin32.inl>
#elif defined(AZ_SIMD_WII)
    #include <AzCore/Math/Internal/VectorFloatWii.inl>
#else
    #include <AzCore/Math/Internal/VectorFloatFpu.inl>
#endif

#endif
#pragma once