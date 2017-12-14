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
#ifndef AZCORE_MATH_QUATERNION_H
#define AZCORE_MATH_QUATERNION_H 1

#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class Quaternion
    {
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Quaternion);
        AZ_TYPE_INFO(Quaternion, "{73103120-3DD3-4873-BAB3-9713FA2804FB}")

        //===============================================================
        // Constructors
        //===============================================================

        ///Default constructor, components are uninitialized
        Quaternion()    { }

        ///Constructs quaternion with all components set to the same specified value
        explicit Quaternion(const VectorFloat& x);

        explicit Quaternion(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w);

        ///For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Quaternion(SimdVectorType value);

        static AZ_MATH_FORCE_INLINE const Quaternion CreateIdentity()               { float xyzw[] = {0.0f, 0.0f, 0.0f, 1.0f}; return CreateFromFloat4(xyzw); }
        static const Quaternion CreateZero();

        ///Sets components from an array of 4 floats, stored in xyzw order
        static const Quaternion CreateFromFloat4(const float* values);

        ///Sets components using a Vector3 for the imaginary part and setting the real part to zero
        static AZ_MATH_FORCE_INLINE const Quaternion CreateFromVector3(const Vector3& v) { Quaternion result; result.Set(v, VectorFloat::CreateZero()); return result; }

        ///Sets components using a Vector3 for the imaginary part and a VectorFloat for the real part
        static const Quaternion CreateFromVector3AndValue(const Vector3& v, const VectorFloat& w);

        ///Sets the quaternion to be a rotation around a specified axis.
        /*@{*/
        static const Quaternion CreateRotationX(float angle);
        static const Quaternion CreateRotationY(float angle);
        static const Quaternion CreateRotationZ(float angle);
        /*@}*/

        /**
         * Creates a quaternion using the rotation part of a Transform matrix
         * \note If the transform has a scale other than (1,1,1) be sure to extract the scale first with AZ::Transform::ExtractScale or ::ExtractScaleExact.
         */
        static const Quaternion CreateFromTransform(const class Transform& t);

        ///Creates a quaternion from a Matrix3x3
        static const Quaternion CreateFromMatrix3x3(const class Matrix3x3& m);

        ///Creates a quaternion using the rotation part of a Matrix4x4
        static const Quaternion CreateFromMatrix4x4(const class Matrix4x4& m);

        static const Quaternion CreateFromAxisAngle(const Vector3& axis, const VectorFloat& angle);

        static const Quaternion CreateShortestArc(const Vector3& v1, const Vector3& v2);

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

        /**
         * Indexed access using operator(). It's just for convenience.
         * \note This can be slower than using GetX,GetY, etc.
         */
        /*@{*/
        AZ_MATH_FORCE_INLINE float operator()(int index) const  { return GetElement(index); }
        /*@}*/

        void Set(const VectorFloat& x);
        void Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z, const VectorFloat& w);
        void Set(const Vector3& v, const VectorFloat& w);
        void Set(const float values[]);

        //===============================================================
        // Inverse/conjugate
        //===============================================================

        ///The conjugate of a quaternion is (-x, -y, -z, w)
        const Quaternion GetConjugate() const;

        /**
         * For a unit quaternion, the inverse is just the conjugate. This function assumes
         * a unit quaternion.
         */
        /*@{*/
        AZ_MATH_FORCE_INLINE const Quaternion GetInverseFast() const { return GetConjugate(); }
        void InvertFast() { *this = GetInverseFast(); }
        /*@}*/

        /**
         * This is the inverse for any quaternion, not just unit quaternions.
         */
        /*@{*/
        AZ_MATH_FORCE_INLINE const Quaternion GetInverseFull() const { return GetConjugate() / GetLengthSq(); }
        void InvertFull() { *this = GetInverseFull(); }
        /*@}*/

        //===============================================================
        // Dot product
        //===============================================================

        const VectorFloat Dot(const Quaternion& q) const;

        //===============================================================
        // Length/normalizing
        //===============================================================

        AZ_MATH_FORCE_INLINE const VectorFloat GetLengthSq() const      { return Dot(*this); }

        ///Returns length of the quaternion, medium speed and medium accuracy, typically uses a refined estimate
        const VectorFloat GetLength() const;
        ///Returns length of the quaternion, fast but low accuracy, uses raw estimate instructions
        const VectorFloat GetLengthApprox() const;
        ///Returns length of the quaternion, full accuracy
        const VectorFloat GetLengthExact() const;

        ///Returns 1/length, medium speed and medium accuracy, typically uses a refined estimate
        const VectorFloat GetLengthReciprocal() const;
        ///Returns 1/length of the quaternion, fast but low accuracy, uses raw estimate instructions
        const VectorFloat GetLengthReciprocalApprox() const;
        ///Returns 1/length of the quaternion, full accuracy
        const VectorFloat GetLengthReciprocalExact() const;

        ///Returns normalized quaternion, medium speed and medium accuracy, typically uses a refined estimate
        const Quaternion GetNormalized() const;
        ///Returns normalized quaternion, fast but low accuracy, uses raw estimate instructions
        const Quaternion GetNormalizedApprox() const;
        ///Returns normalized quaternion, full accuracy
        const Quaternion GetNormalizedExact() const;

        ///Normalizes the quaternion in-place, medium speed and medium accuracy, typically uses a refined estimate
        AZ_MATH_FORCE_INLINE void Normalize()       { *this = GetNormalized(); }
        ///Normalizes the quaternion in-place, fast but low accuracy, uses raw estimate instructions
        AZ_MATH_FORCE_INLINE void NormalizeApprox() { *this = GetNormalizedApprox(); }
        ///Normalizes the quaternion in-place, full accuracy
        AZ_MATH_FORCE_INLINE void NormalizeExact()  { *this = GetNormalizedExact(); }

        ///Normalizes the vector in-place and returns the previous length. This takes a few more instructions than calling
        ///just Normalize().
        /*@{*/
        const VectorFloat NormalizeWithLength();
        const VectorFloat NormalizeWithLengthApprox();
        const VectorFloat NormalizeWithLengthExact();
        /*@}*/

        //===============================================================
        // Interpolation
        //===============================================================

        /**
         * Linearly interpolate towards a destination quaternion.
         */
        const Quaternion Lerp(const Quaternion& dest, const VectorFloat& t) const;

        /**
         * Spherical linear interpolation. Result is NOT normalized.
         */
        const Quaternion Slerp(const Quaternion& dest, float t) const;

        /**
         * Spherical linear interpolation, but with in/out tangent quaternions.
         */
        const Quaternion Squad(const Quaternion& dest, const Quaternion& in, const Quaternion& out, float t) const;

        //===============================================================
        // Comparison
        //===============================================================

        /**
         * Checks if the quaternion is close to another quaternion with a given floating point tolerance
         */
        bool IsClose(const Quaternion& q, const VectorFloat& tolerance = g_simdTolerance) const;

        bool IsIdentity(const VectorFloat& tolerance = g_simdTolerance) const;

        AZ_MATH_FORCE_INLINE bool IsZero(const VectorFloat& tolerance = g_fltEps) const { return IsClose(CreateZero(), tolerance); }

        //===============================================================
        // Standard operators
        //===============================================================
#if defined(AZ_SIMD)
        AZ_MATH_FORCE_INLINE Quaternion(const Quaternion& q)
            : m_value(q.m_value)   {}
        AZ_MATH_FORCE_INLINE Quaternion& operator= (const Quaternion& rhs)          { m_value = rhs.m_value; return *this;  }
#endif

        const Quaternion operator-() const;
        const Quaternion operator+(const Quaternion& q) const;
        const Quaternion operator-(const Quaternion& q) const;
        const Quaternion operator*(const Quaternion& q) const;
        const Quaternion operator*(const VectorFloat& multiplier) const;
        const Quaternion operator/(const VectorFloat& divisor) const;

        AZ_MATH_FORCE_INLINE Quaternion& operator+=(const Quaternion& q)        { *this = *this + q; return *this; }
        AZ_MATH_FORCE_INLINE Quaternion& operator-=(const Quaternion& q)        { *this = *this - q; return *this; }
        AZ_MATH_FORCE_INLINE Quaternion& operator*=(const Quaternion& q)        { *this = *this * q; return *this; }
        AZ_MATH_FORCE_INLINE Quaternion& operator*=(const VectorFloat& multiplier)  { *this = *this * multiplier; return *this; }
        AZ_MATH_FORCE_INLINE Quaternion& operator/=(const VectorFloat& divisor)     { *this = *this / divisor; return *this; }

        bool operator==(const Quaternion& rhs) const;
        bool operator!=(const Quaternion& rhs) const;

        /**
         * Transforms a vector. The multiplication order is defined to be q*v, which matches the matrix multiplication order.
         */
        const Vector3 operator*(const Vector3& v) const;

        //===============================================================
        // Miscellaneous
        //===============================================================

        //TODO: write proper simd version
        AZ_MATH_FORCE_INLINE const Vector3 GetImaginary() const     { return Vector3(GetX(), GetY(), GetZ()); }

        /**
         * Return angle in radians.
         */
        VectorFloat GetAngle() const;

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return IsFiniteFloat(GetX()) && IsFiniteFloat(GetY()) && IsFiniteFloat(GetZ()) && IsFiniteFloat(GetW()); }

    private:

        ///Takes the absolute value of each component of
        const Quaternion GetAbs() const;

        friend const Quaternion operator*(const VectorFloat& multiplier, const Quaternion& rhs);

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
AZSTD_DECLARE_POD_TYPE(AZ::Quaternion);
#endif

#if defined(AZ_SIMD_WINDOWS) || defined(AZ_SIMD_XBONE) || defined(AZ_SIMD_PS4) || defined(AZ_SIMD_LINUX) || defined(AZ_SIMD_APPLE_OSX)
    #include <AzCore/Math/Internal/QuaternionWin32.inl>
#elif defined(AZ_SIMD_WII)
    #include <AzCore/Math/Internal/QuaternionWii.inl>
#else
    #include <AzCore/Math/Internal/QuaternionFpu.inl>
#endif

#endif
#pragma once