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
#ifndef AZCORE_MATH_VECTOR3_H
#define AZCORE_MATH_VECTOR3_H 1

#include <AzCore/Math/Internal/MathTypes.h>
#include <AzCore/Math/VectorFloat.h>
namespace AZ
{
    class Vector4;
    class Transform;
    class Quaternion;
    class Matrix4x4;
    class Matrix3x3;

    /**
     * 3-dimensional vector class
     */
    class Vector3
    {
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Vector3);
        AZ_TYPE_INFO(Vector3, "{8379EB7D-01FA-4538-B64B-A6543B4BE73D}")

        //===============================================================
        // Constructors
        //===============================================================

        ///Default constructor, components are uninitialized
        Vector3()   { }

        ///Constructs vector with all components set to the same specified value
        explicit Vector3(const VectorFloat& x);

        explicit Vector3(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z);

        ///For internal use only, arrangement of values in SIMD type is not guaranteed.
        explicit Vector3(SimdVectorType value);

        ///Creates a vector with all components set to zero, more efficient than calling Vector3(0.0f)
        static const Vector3 CreateZero();

        ///Creates a vector with all components set to one, more efficient than calling Vector3(1.0f)
        static const Vector3 CreateOne();

        static AZ_MATH_FORCE_INLINE const Vector3 CreateAxisX(const VectorFloat& length = VectorFloat::CreateOne()) { return Vector3(length,                    VectorFloat::CreateZero(), VectorFloat::CreateZero()); }
        static AZ_MATH_FORCE_INLINE const Vector3 CreateAxisY(const VectorFloat& length = VectorFloat::CreateOne()) { return Vector3(VectorFloat::CreateZero(), length,                    VectorFloat::CreateZero()); }
        static AZ_MATH_FORCE_INLINE const Vector3 CreateAxisZ(const VectorFloat& length = VectorFloat::CreateOne()) { return Vector3(VectorFloat::CreateZero(), VectorFloat::CreateZero(), length); }

        ///Sets components from an array of 3 floats, stored in xyz order
        static const Vector3 CreateFromFloat3(const float* values);

        ///operation r.x = (cmp1.x == cmp2.x) ? vA.x : vB.x per component
        static const Vector3 CreateSelectCmpEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB);

        ///operation ( r.x = (cmp1.x >= cmp2.x) ? vA.x : vB.x ) per component
        static const Vector3 CreateSelectCmpGreaterEqual(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB);

        ///operation ( r.x = (cmp1.x > cmp2.x) ? vA.x : vB.x ) per component
        static const Vector3 CreateSelectCmpGreater(const Vector3& cmp1, const Vector3& cmp2, const Vector3& vA, const Vector3& vB);

        //===============================================================
        // Store
        //===============================================================

        ///Stores the vector to an array of 3 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT
        ///required. Note that StoreToFloat4 is usually faster, as the extra padding allows more efficient instructions
        ///to be used.
        void StoreToFloat3(float* values) const;

        ///Stores the vector to an array of 4 floats. The floats need only be 4 byte aligned, 16 byte alignment is NOT
        ///required. The 4th float is generally set to garbage, but its presence allows more efficient instructions to
        ///be used.
        void StoreToFloat4(float* values) const;

        //===============================================================
        // Component access
        //===============================================================

        const VectorFloat GetX() const;
        const VectorFloat GetY() const;
        const VectorFloat GetZ() const;
        void SetX(const VectorFloat& x);
        void SetY(const VectorFloat& y);
        void SetZ(const VectorFloat& z);

        ///We recommend using GetX,Y,Z. GetElement can be slower.
        const VectorFloat GetElement(int index) const;
        ///We recommend using SetX,Y,Z. SetElement can be slower.
        void SetElement(int index, const VectorFloat& v);

        ///Sets all components to the same specified value
        void Set(const VectorFloat& x);

        void Set(const VectorFloat& x, const VectorFloat& y, const VectorFloat& z);

        ///Sets components from an array of 3 floats in xyz order.
        void Set(const float values[]);

        /**
         * Indexed access using operator(). It's just for convenience.
         * \note This can be slower than using GetX,GetY, etc.
         */
        AZ_MATH_FORCE_INLINE const VectorFloat operator()(int index) const      { return GetElement(index); }

        //===============================================================
        // Length/normalizing
        //===============================================================

        ///Returns squared length of the vector
        AZ_MATH_FORCE_INLINE const VectorFloat GetLengthSq() const              { return Dot(*this); }

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
        const Vector3 GetNormalized() const;
        ///Returns normalized vector, fast but low accuracy, uses raw estimate instructions
        const Vector3 GetNormalizedApprox() const;
        ///Returns normalized vector, full accuracy
        const Vector3 GetNormalizedExact() const;

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

        ///Safe normalization functions work the same as their non-safe counterparts, but will return (1,0,0) if the length
        ///of the vector is too small.
        /*@{*/
        const Vector3 GetNormalizedSafe(const VectorFloat& tolerance = g_simdTolerance) const;
        const Vector3 GetNormalizedSafeApprox(const VectorFloat& tolerance = g_simdTolerance) const;
        const Vector3 GetNormalizedSafeExact(const VectorFloat& tolerance = g_simdTolerance) const;
        AZ_MATH_FORCE_INLINE void NormalizeSafe(const VectorFloat& tolerance = g_simdTolerance)       { *this = GetNormalizedSafe(tolerance); }
        AZ_MATH_FORCE_INLINE void NormalizeSafeApprox(const VectorFloat& tolerance = g_simdTolerance) { *this = GetNormalizedSafeApprox(tolerance); }
        AZ_MATH_FORCE_INLINE void NormalizeSafeExact(const VectorFloat& tolerance = g_simdTolerance)  { *this = GetNormalizedSafeExact(tolerance); }
        const VectorFloat NormalizeSafeWithLength(const VectorFloat& tolerance = g_simdTolerance);
        const VectorFloat NormalizeSafeWithLengthApprox(const VectorFloat& tolerance = g_simdTolerance);
        const VectorFloat NormalizeSafeWithLengthExact(const VectorFloat& tolerance = g_simdTolerance);
        /*@}*/

        bool IsNormalized(const VectorFloat& tolerance = g_simdTolerance) const;

        ///Scales the vector to have the specified length, medium speed and medium accuracy, typically uses a refined estimate
        void SetLength(const VectorFloat& length);
        ///Scales the vector to have the specified length, fast but low accuracy, uses raw estimate instructions
        void SetLengthApprox(const VectorFloat& length);
        ///Scales the vector to have the specified length, full accuracy
        void SetLengthExact(const VectorFloat& length);

        ///Returns squared distance to another Vector3
        AZ_MATH_FORCE_INLINE const VectorFloat GetDistanceSq(const Vector3& v) const     { return ((*this) - v).GetLengthSq(); }

        ///Returns distance to another Vector3
        /*@{*/
        AZ_MATH_FORCE_INLINE const VectorFloat GetDistance(const Vector3& v) const       { return ((*this) - v).GetLength(); }
        AZ_MATH_FORCE_INLINE const VectorFloat GetDistanceApprox(const Vector3& v) const { return ((*this) - v).GetLengthApprox(); }
        AZ_MATH_FORCE_INLINE const VectorFloat GetDistanceExact(const Vector3& v) const  { return ((*this) - v).GetLengthExact(); }
        /*@}*/

        //===============================================================
        // Interpolation
        //===============================================================

        /**
         * Linear interpolation between this vector and a destination.
         * @return (*this)*(1-t) + dest*t
         */
        const Vector3 Lerp(const Vector3& dest, const VectorFloat& t) const;

        /**
         * Spherical linear interpolation between normalized vectors.
         * Interpolates along the great circle connecting the two vectors.
         */
        const Vector3 Slerp(const Vector3& dest, const VectorFloat& t) const;

        //===============================================================
        // Dot/cross products
        //===============================================================

        ///Dot product of two vectors
        const VectorFloat Dot(const Vector3& rhs) const;

        ///Cross product of two vectors
        const Vector3 Cross(const Vector3& rhs) const;

        ///Cross product with specified axis.
        /*@{*/
        const Vector3 CrossXAxis() const;
        const Vector3 CrossYAxis() const;
        const Vector3 CrossZAxis() const;
        const Vector3 XAxisCross() const;
        const Vector3 YAxisCross() const;
        const Vector3 ZAxisCross() const;
        /*@}*/

        //===============================================================
        // Comparison
        //===============================================================

        ///Checks the vector is equal to another within a floating point tolerance
        bool IsClose(const Vector3& v, const VectorFloat& tolerance = g_simdTolerance) const;

        AZ_MATH_FORCE_INLINE bool IsZero(const VectorFloat& tolerance = g_fltEps) const { return IsClose(CreateZero(), tolerance); }

        bool operator==(const Vector3& rhs) const;
        bool operator!=(const Vector3& rhs) const;

        /**
         * Comparison functions, not implemented as operators since that would probably be a little dangerous. These
         * functions return true only if all components pass the comparison test.
         */
        /*@{*/
        bool IsLessThan(const Vector3& rhs) const;
        bool IsLessEqualThan(const Vector3& rhs) const;
        bool IsGreaterThan(const Vector3& rhs) const;
        bool IsGreaterEqualThan(const Vector3& rhs) const;
        /*@}*/

        //===============================================================
        // Min/Max
        //===============================================================

        ///Min/Max functions, operate on each component individually, result will be a new Vector3
        /*@{*/
        const Vector3 GetMin(const Vector3& v) const;
        const Vector3 GetMax(const Vector3& v) const;
        AZ_MATH_FORCE_INLINE const Vector3 GetClamp(const Vector3& min, const Vector3& max) const   { return GetMin(max).GetMax(min); }
        /*@}*/

        //===============================================================
        // Standard operators
        //===============================================================
#if defined(AZ_SIMD)
        AZ_MATH_FORCE_INLINE Vector3(const Vector3& v)
            : m_value(v.m_value)            {}
        AZ_MATH_FORCE_INLINE Vector3& operator= (const Vector3& rhs)        { m_value = rhs.m_value; return *this;  }
#endif

        const Vector3 operator-() const;
        const Vector3 operator+(const Vector3& rhs) const;
        const Vector3 operator-(const Vector3& rhs) const;
        const Vector3 operator*(const Vector3& rhs) const;
        const Vector3 operator/(const Vector3& rhs) const;
        const Vector3 operator*(const VectorFloat& multiplier) const;
        const Vector3 operator/(const VectorFloat& divisor) const;

        AZ_MATH_FORCE_INLINE Vector3& operator+=(const Vector3& rhs)
        {
            *this = (*this) + rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector3& operator-=(const Vector3& rhs)
        {
            *this = (*this) - rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector3& operator*=(const Vector3& rhs)
        {
            *this = (*this) * rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector3& operator/=(const Vector3& rhs)
        {
            *this = (*this) / rhs;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector3& operator*=(const VectorFloat& multiplier)
        {
            *this = (*this) * multiplier;
            return *this;
        }
        AZ_MATH_FORCE_INLINE Vector3& operator/=(const VectorFloat& divisor)
        {
            *this = (*this) / divisor;
            return *this;
        }

        //===============================================================
        // Trigonometry
        //===============================================================

        ///Gets the sine of each component.
        const Vector3 GetSin() const;

        ///Gets the cosine of each component.
        const Vector3 GetCos() const;

        ///Gets the sine and cosine of each component, quicker than calling GetSin and GetCos separately.
        void GetSinCos(Vector3& sin, Vector3& cos) const;

        ///wraps the angle in each component into the [-pi,pi] range
        const Vector3 GetAngleMod() const;

        //===============================================================
        // Miscellaneous
        //===============================================================

        ///Takes the absolute value of each component of the vector
        const Vector3 GetAbs() const;

        ///Returns the reciprocal of each component of the vector, medium speed and medium accuracy, typically uses a refined estimate
        const Vector3 GetReciprocal() const;

        ///Returns the reciprocal of each component of the vector, fast but low accuracy, uses raw estimate instructions
        const Vector3 GetReciprocalApprox() const;

        ///Returns the reciprocal of each component of the vector, full accuracy
        const Vector3 GetReciprocalExact() const;

        ///Builds a tangent basis with this vector as the normal.
        void BuildTangentBasis(Vector3& tangent, Vector3& bitangent) const;

        ///Perform multiply-add operator, returns this*mul + add
        const Vector3 GetMadd(const Vector3& mul, const Vector3& add);

        ///Perform multiply-add operator (this = this * mul + add)
        AZ_MATH_FORCE_INLINE void Madd(const Vector3& mul, const Vector3& add)      { *this = GetMadd(mul, add); }

        bool IsPerpendicular(const Vector3& v, const VectorFloat& tolerance = g_simdTolerance) const;

        /// Project vector onto another. P = (a.Dot(b) / b.Dot(b)) * b
        AZ_MATH_FORCE_INLINE void           Project(const Vector3& rhs)                 { *this = rhs * (Dot(rhs) / rhs.Dot(rhs)); }
        /// Project vector onto a normal (faster function). P = (v.Dot(Normal) * normal)
        AZ_MATH_FORCE_INLINE void           ProjectOnNormal(const Vector3& normal)      { AZ_Assert(normal.IsNormalized(), "This normal is not a normalized!"); *this = normal * Dot(normal); }

        /// Project this vector onto another and return the resulting projection. P = (a.Dot(b) / b.Dot(b)) * b
        AZ_MATH_FORCE_INLINE Vector3        GetProjected(const Vector3& rhs) const      { return rhs * (Dot(rhs) / rhs.Dot(rhs)); }
        /// Project this vector onto a normal and return the resulting projection. P = v - (v.Dot(Normal) * normal)
        AZ_MATH_FORCE_INLINE Vector3        GetProjectedOnNormal(const Vector3& normal) {  AZ_Assert(normal.IsNormalized(), "This normal is not a normalized!"); return normal * Dot(normal); }

        AZ_MATH_FORCE_INLINE bool           IsFinite() const { return IsFiniteFloat(GetX()) && IsFiniteFloat(GetY()) && IsFiniteFloat(GetZ()); }

    private:
        friend class Vector4;
        friend class Transform;
        friend class Matrix4x4;
        friend class Matrix3x3;
        friend class Quaternion;
        friend const Vector3 operator*(const Vector3& lhs, const Transform& rhs);
        friend const Vector3 operator*(const Vector3& lhs, const Matrix4x4& rhs);
        friend const Vector3 operator*(const Vector3& lhs, const Matrix3x3& rhs);

        #if defined(AZ_SIMD)
        SimdVectorType m_value;
        #else
        AZ_ALIGN(float m_x, 16);        //align just to be consistent with simd implementations
        float m_y, m_z;
        float m_pad;                    //pad to 16 bytes, also for consistency with simd implementations
        #endif
    };
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Vector3);
#endif

#if defined(AZ_SIMD_WINDOWS) || defined(AZ_SIMD_XBONE) || defined(AZ_SIMD_PS4) || defined(AZ_SIMD_LINUX) || defined(AZ_SIMD_APPLE_OSX)
    #include <AzCore/Math/Internal/Vector3Win32.inl>
#elif defined(AZ_SIMD_WII)
    #include <AzCore/Math/Internal/Vector3Wii.inl>
#else
    #include <AzCore/Math/Internal/Vector3Fpu.inl>
#endif

#endif
#pragma once