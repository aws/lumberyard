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
#ifndef AZCORE_MATH_VECTOR4_H
#define AZCORE_MATH_VECTOR4_H 1

#include <AzCore/Math/Vector3.h>

namespace AZ
{
    /**
     * A vector class with 4 components, you probably only want to use this if you're
     * dealing with projection matrices.
     * To convert back to a Vector3, call the GetHomogenized function.
     */
    class Vector4
    {
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Vector4);
        AZ_TYPE_INFO(Vector4, "{0CE9FA36-1E3A-4C06-9254-B7C73A732053}")

        //===============================================================
        // Constructors
        //===============================================================

        ///Default constructor, components are uninitialized
        Vector4()   { }

        ///Constructs vector with all components set to the same specified value
        explicit Vector4(const VectorFloat& x);

        explicit Vector4(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w);

        ///For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Vector4(SimdVectorType value);

        ///Creates a vector with all components set to zero, more efficient than calling Vector3(0.0f)
        static const Vector4 CreateZero();

        ///Creates a vector with all components set to one, more efficient than calling Vector3(1.0f)
        static const Vector4 CreateOne();

        static AZ_MATH_FORCE_INLINE const Vector4 CreateAxisX(const VectorFloat& length = VectorFloat::CreateOne()) { return Vector4(length,                    VectorFloat::CreateZero(), VectorFloat::CreateZero(), VectorFloat::CreateZero()); }
        static AZ_MATH_FORCE_INLINE const Vector4 CreateAxisY(const VectorFloat& length = VectorFloat::CreateOne()) { return Vector4(VectorFloat::CreateZero(), length,                    VectorFloat::CreateZero(), VectorFloat::CreateZero()); }
        static AZ_MATH_FORCE_INLINE const Vector4 CreateAxisZ(const VectorFloat& length = VectorFloat::CreateOne()) { return Vector4(VectorFloat::CreateZero(), VectorFloat::CreateZero(), length,                    VectorFloat::CreateZero()); }
        static AZ_MATH_FORCE_INLINE const Vector4 CreateAxisW(const VectorFloat& length = VectorFloat::CreateOne()) { return Vector4(VectorFloat::CreateZero(), VectorFloat::CreateZero(), VectorFloat::CreateZero(), length); }

        ///Sets components from an array of 4 floats, stored in xyzw order
        static const Vector4 CreateFromFloat4(const float* values);

        ///Copies x,y,z components from a Vector3, sets w to 1.0
        static const Vector4 CreateFromVector3(const Vector3& v);

        ///Copies x,y,z components from a Vector3, specify w separately
        static const Vector4 CreateFromVector3AndFloat(const Vector3& v, const VectorFloat& w);

        //===============================================================
        // Store
        //===============================================================

        ///Stores the vector to an array of 4 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT
        ///required.
        void StoreToFloat4(float* values) const;

        //===============================================================
        // Component access
        //===============================================================

        const VectorFloat GetX() const;
        const VectorFloat GetY() const;
        const VectorFloat GetZ() const;
        const VectorFloat GetW() const;
        void SetX(const VectorFloat& x);
        void SetY(const VectorFloat& y);
        void SetZ(const VectorFloat& z);
        void SetW(const VectorFloat& w);

        // We recommend using GetX,Y,Z,W. GetElement can be slower.
        const VectorFloat GetElement(int index) const;
        // We recommend using SetX,Y,Z,W. SetElement can be slower.
        void SetElement(int index, const VectorFloat& v);

        AZ_MATH_FORCE_INLINE Vector3 GetAsVector3() const
        {
#ifdef AZ_SIMD
            return Vector3(m_value);
#else
            return Vector3(m_x, m_y, m_z);
#endif
        }

        ///Sets all components to the same specified value
        void Set(const VectorFloat& x);

        void Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w);

        ///Sets components from an array of 4 floats, stored in xyzw order
        void Set(const float values[]);

        ///Sets x,y,z components from a Vector3, sets w to 1.0
        void Set(const Vector3& v);

        ///Sets x,y,z components from a Vector3, specify w separately
        void Set(const Vector3& v, const VectorFloat& w);

        /**
         * Indexed access using operator(). It's just for convenience.
         * \note This can be slower than using GetX,GetY, etc.
         */
        AZ_MATH_FORCE_INLINE const VectorFloat operator()(int index) const      { return GetElement(index); }

        //===============================================================
        // Length/normalizing
        //===============================================================

        ///Returns squared length of the vector
        AZ_MATH_FORCE_INLINE const VectorFloat GetLengthSq() const { return Dot(*this); }

        ///Returns length of the vector, medium speed and medium accuracy, typically uses a refined estimate
        const VectorFloat GetLength() const;
        ///Returns length of the vector, fast but low accuracy, uses raw estimate instructions
        const VectorFloat GetLengthApprox() const;
        ///Returns length of the vector, full accuracy
        const VectorFloat GetLengthExact() const;

        ///Returns 1/length, medium speed and medium accuracy, typically uses a refined estimate
        const VectorFloat GetLengthReciprocal() const;
        ///Returns 1/length of the vector, fast but low accuracy, uses raw estimate instructions
        const VectorFloat GetLengthReciprocalApprox() const;
        ///Returns 1/length of the vector, full accuracy
        const VectorFloat GetLengthReciprocalExact() const;

        ///Returns normalized vector, medium speed and medium accuracy, typically uses a refined estimate
        const Vector4 GetNormalized() const;
        ///Returns normalized vector, fast but low accuracy, uses raw estimate instructions
        const Vector4 GetNormalizedApprox() const;
        ///Returns normalized vector, full accuracy
        const Vector4 GetNormalizedExact() const;

        ///Normalizes the vector in-place, medium speed and medium accuracy, typically uses a refined estimate
        AZ_MATH_FORCE_INLINE void Normalize()       { *this = GetNormalized(); }
        ///Normalizes the vector in-place, fast but low accuracy, uses raw estimate instructions
        AZ_MATH_FORCE_INLINE void NormalizeApprox() { *this = GetNormalizedApprox(); }
        ///Normalizes the vector in-place, full accuracy
        AZ_MATH_FORCE_INLINE void NormalizeExact()  { *this = GetNormalizedExact(); }

        ///Normalizes the vector in-place and returns the previous length. This takes a few more instructions than calling
        ///just Normalize().
        /*@{*/
        const VectorFloat NormalizeWithLength();
        const VectorFloat NormalizeWithLengthApprox();
        const VectorFloat NormalizeWithLengthExact();
        /*@}*/

        ///Safe normalization functions work the same as their non-safe counterparts, but will return (1,0,0,0) if the length
        ///of the vector is too small.
        /*@{*/
        const Vector4 GetNormalizedSafe(const VectorFloat& tolerance = g_simdTolerance) const;
        const Vector4 GetNormalizedSafeApprox(const VectorFloat& tolerance = g_simdTolerance) const;
        const Vector4 GetNormalizedSafeExact(const VectorFloat& tolerance = g_simdTolerance) const;
        AZ_MATH_FORCE_INLINE void NormalizeSafe(const VectorFloat& tolerance = g_simdTolerance)       { *this = GetNormalizedSafe(tolerance); }
        AZ_MATH_FORCE_INLINE void NormalizeSafeApprox(const VectorFloat& tolerance = g_simdTolerance) { *this = GetNormalizedSafeApprox(tolerance); }
        AZ_MATH_FORCE_INLINE void NormalizeSafeExact(const VectorFloat& tolerance = g_simdTolerance)  { *this = GetNormalizedSafeExact(tolerance); }
        const VectorFloat NormalizeSafeWithLength(const VectorFloat& tolerance = g_simdTolerance);
        const VectorFloat NormalizeSafeWithLengthApprox(const VectorFloat& tolerance = g_simdTolerance);
        const VectorFloat NormalizeSafeWithLengthExact(const VectorFloat& tolerance = g_simdTolerance);
        /*@}*/

        bool IsNormalized(const VectorFloat& tolerance = g_simdTolerance) const;

        //===============================================================
        // Comparisons
        //===============================================================

        ///Checks the vector is equal to another within a floating point tolerance
        bool IsClose(const Vector4& v, const VectorFloat& tolerance = g_simdTolerance) const;

        AZ_MATH_FORCE_INLINE bool IsZero(const VectorFloat& tolerance = g_fltEps) const { return IsClose(CreateZero(), tolerance); }

        bool operator==(const Vector4& rhs) const;
        bool operator!=(const Vector4& rhs) const;

        /**
        * Comparison functions, not implemented as operators since that would probably be a little dangerous. These
        * functions return true only if all components pass the comparison test.
        */
        /*@{*/
        bool IsLessThan(const Vector4& rhs) const;
        bool IsLessEqualThan(const Vector4& rhs) const;
        bool IsGreaterThan(const Vector4& rhs) const;
        bool IsGreaterEqualThan(const Vector4& rhs) const;
        /*@}*/

        //===============================================================
        // Dot products
        //===============================================================

        ///Dot product of two vectors, uses all 4 components.
        const VectorFloat Dot(const Vector4& rhs) const;

        //Dot product of two vectors, using only the x,y,z components.
        const VectorFloat Dot3(const Vector3& rhs) const;

        //===============================================================
        // Homogenize
        //===============================================================

        ///Homogenizes the vector, i.e. divides all components by w.
        void Homogenize();

        ///Returns the homogenized vector, i.e. divides all compnents by w, return value is a Vector3.
        const Vector3 GetHomogenized() const;

        //===============================================================
        // Standard operators
        //===============================================================
#if defined(AZ_SIMD)
        AZ_MATH_FORCE_INLINE Vector4(const Vector4& v)
            : m_value(v.m_value)                {}
        AZ_MATH_FORCE_INLINE Vector4& operator= (const Vector4& rhs)            { m_value = rhs.m_value; return *this; }
#endif

        const Vector4 operator-() const;
        const Vector4 operator+(const Vector4& rhs) const;
        const Vector4 operator-(const Vector4& rhs) const;
        const Vector4 operator*(const Vector4& rhs) const;
        const Vector4 operator/(const Vector4& rhs) const;
        const Vector4 operator*(const VectorFloat& multiplier) const;
        const Vector4 operator/(const VectorFloat& divisor) const;

        AZ_MATH_FORCE_INLINE Vector4& operator+=(const Vector4& rhs)
        {
            *this = (*this) + rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector4& operator-=(const Vector4& rhs)
        {
            *this = (*this) - rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector4& operator*=(const Vector4& rhs)
        {
            *this = (*this) * rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector4& operator/=(const Vector4& rhs)
        {
            *this = (*this) / rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector4& operator*=(const VectorFloat& multiplier)
        {
            *this = (*this) * multiplier;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector4& operator/=(const VectorFloat& divisor)
        {
            *this = (*this) / divisor;
            return *this;
        }

        //===============================================================
        // Miscellaneous
        //===============================================================

        ///Takes the absolute value of each component of the vector
        const Vector4 GetAbs() const;

        ///Returns the reciprocal of each component of the vector, medium speed and medium accuracy, typically uses a refined estimate
        const Vector4 GetReciprocal() const;

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return IsFiniteFloat(GetX()) && IsFiniteFloat(GetY()) && IsFiniteFloat(GetZ()) && IsFiniteFloat(GetW()); }

    protected:
        friend class Transform;
        friend class Matrix4x4;
        friend class Matrix3x3;
        friend const Vector4 operator*(const Vector4& lhs, const Transform& rhs);
        friend const Vector4 operator*(const Vector4& lhs, const Matrix4x4& rhs);

        #if defined(AZ_SIMD)
        SimdVectorType m_value;
        #else
        AZ_ALIGN(float m_x, 16);        //align just to be consistent with simd implementations
        float m_y, m_z, m_w;
        #endif
    };
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Vector4);
#endif

#if defined(AZ_SIMD_WINDOWS) || defined(AZ_SIMD_XBONE) || defined(AZ_SIMD_PS4) || defined(AZ_SIMD_LINUX) || defined(AZ_SIMD_APPLE_OSX)
    #include <AzCore/Math/Internal/Vector4Win32.inl>
#elif defined(AZ_SIMD_WII)
    #include <AzCore/Math/Internal/Vector4Wii.inl>
#else
    #include <AzCore/Math/Internal/Vector4Fpu.inl>
#endif

#endif
#pragma once