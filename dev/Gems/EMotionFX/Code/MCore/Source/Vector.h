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

#pragma once

// include required header files
#include "StandardHeaders.h"
#include "FastMath.h"


namespace MCore
{
    /**
     * The 3D vector class, where the x, y and z attributes are public.
     */
    class MCORE_API Vector3
    {
    public:
        /**
         * Default constructor. Does not initialize any member attributes. So x, y and z contain unknown values.
         */
        MCORE_INLINE Vector3()                                                                  { }

        /**
         * Constructor which sets the x, y and z.
         * @param vx The value for x.
         * @param vy The value for y.
         * @param vz The value for z.
         */
        MCORE_INLINE Vector3(float vx, float vy, float vz)
            : x(vx)
            , y(vy)
            , z(vz)       { }

        /**
         * Copy constructor.
         * @param v The vector to copy the x, y and z from.
         */
        MCORE_INLINE Vector3(const Vector3& v)
            : x(v.x)
            , y(v.y)
            , z(v.z)    { }

        /**
         * Set the values for x, y and z.
         * @param vx The value for x.
         * @param vy The value for y.
         * @param vz The value for z.
         */
        MCORE_INLINE void Set(float vx, float vy, float vz)                                     { x = vx; y = vy; z = vz; }

        /**
         * Perform a dot product between this vector and the specified one.
         * The dot product works like this: (x*v.x + y*v.y + z*v.z) where v is the other vector.
         * If both vectors are normalized, so have a length of 1 we will get the cosine angle (dotResult=Cos(angle)) between the two vectors. So to get the angle in radians, we do: ACos(dotResult).
         * @param v The vector to perform the dot product with.
         * @result The result of the dotproduct, which is as explained above, the cosine of the angle between the two vectors when both vectors are normalized (have a length of 1).
         */
        MCORE_INLINE float Dot(const Vector3& v) const                                          { return (x * v.x + y * v.y + z * v.z); }

        /**
         * Perform the cross product between this vector and another specified one.
         * The result of the cross product is the vector perpendicular to the other two.
         * An easy way to understand how it works: This vector and the vector passed as parameter make a plane. The result will be the normal of this plane.
         * @param v The other vector to do the cross product with (so the other vector on the plane).
         * @result The result of the cross product (the normal of the plane).
         */
        MCORE_INLINE Vector3 Cross(const Vector3& v) const                                      { return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x); }

        /**
         * Calculates the square length of this vector.
         * @result The square length of this vector (length*length).
         */
        MCORE_INLINE float SquareLength() const                                                 { return (x * x + y * y + z * z); }

        /**
         * Calculates the length of this vector.
         * Zero lengths are not safe here and will generate a NAN!
         * @result The length of this vector.
         */
        MCORE_INLINE float Length() const                                                       { return Math::Sqrt(x * x + y * y + z * z); }

        /**
         * Calculates the length of this vector.
         * Zero lengths are also safe here.
         * @result The length of this vector.
         */
        MCORE_INLINE float SafeLength() const                                                   { return Math::SafeSqrt(x * x + y * y + z * z); }

        /**
         * Normalize this vector (make its length 1).
         * Beware that the current length may not be 0! Because in that case we get a division by zero.
         * @result The same vector, but now normalized.
         */
        MCORE_INLINE Vector3& Normalize()                                                       { const float len = 1.0f / Length();  x *= len; y *= len; z *= len; return *this; }

        /**
         * Returns the normalized version of this vector (the current length may not be 0!).
         * @result The normalized version of this vector.
         */
        MCORE_INLINE Vector3 Normalized() const                                                 { const float len = 1.0f / Length(); return Vector3(x * len, y * len, z * len); }

        /**
         * Normalize this vector (make it unit length).
         * The difference with the Normalize method is that SafeNormalize prevents any division by zero.
         * @result The same vector, but now normalized.
         */
        MCORE_INLINE Vector3& SafeNormalize()
        {
            const float len = SquareLength();
            if (len > 0.0f)
            {
                const float invLen = 1.0f / Math::Sqrt(len);
                x *= invLen;
                y *= invLen;
                z *= invLen;
            }
            return *this;
        }

        /**
         * Returns the normalized version of this vector.
         * The difference with the Normalized method is that SafeNormalized prevents any division by zero.
         * @result The normalized version of this vector.
         */
        MCORE_INLINE Vector3 SafeNormalized() const
        {
            const float sqLen = SquareLength();
            if (sqLen > 0.0f)
            {
                const float invLen = 1.0f / Math::Sqrt(sqLen);
                return Vector3(x * invLen, y * invLen, z * invLen);
            }
            return Vector3(*this);
        }

        /**
         * Approximately normalize this vector (make its length 1).
         * Beware that the current length may not be zero! Because in that case we get a division by zero.
         * @result The same vector, but now approximately normalized.
         */
        MCORE_INLINE Vector3& FastNormalize()                                                   { const float len = Math::FastInvSqrt(x * x + y * y + z * z); x *= len; y *= len; z *= len; return *this; }

        /**
         * Returns the approximated normalized version of this vector (the current length may not be zero!).
         * @result The approximated normalized version of this vector.
         */
        MCORE_INLINE Vector3 FastNormalized() const                                             { const float len = Math::FastInvSqrt(x * x + y * y + z * z); return Vector3(x * len, y * len, z * len); }

        /**
         * Normalize this vector (make it unit length), using approximation.
         * The difference with the Normalize method is that SafeNormalize prevents any division by zero.
         * @result The same vector, but now approximately normalized.
         */
        MCORE_INLINE Vector3& FastSafeNormalize()
        {
            const float len = SquareLength();
            if (len > 0.0f)
            {
                const float invLen = Math::FastInvSqrt(len);
                x *= invLen;
                y *= invLen;
                z *= invLen;
            }
            return *this;
        }

        /**
         * Returns the approximated normalized version of this vector.
         * The difference with the Normalized method is that SafeNormalized prevents any division by zero.
         * @result The approximately normalized version of this vector.
         */
        MCORE_INLINE Vector3 FastSafeNormalized() const
        {
            const float sqLen = SquareLength();
            if (sqLen > 0.0f)
            {
                const float invLen = Math::FastInvSqrt(sqLen);
                return Vector3(x * invLen, y * invLen, z * invLen);
            }
            return Vector3(*this);
        }

        /**
         * Check if the components of the vector are close to equal. An example of a uniform vector would be (1.0f, 1.0f, 1.0f) or (5.34f, 5.34f, 5.34).
         * @result True when the vector is uniform, false if not.
         */
        MCORE_INLINE bool CheckIfIsUniform() const                                              { return ((Math::Abs(x - y) <= Math::epsilon) && (Math::Abs(x - z) < Math::epsilon)); }

        /**
         * Calculate the reflection vector.
         * With reflection vector we mean this vector reflected along the specified normal.
         * <pre>
         * this  N    R
         *  ^    ^    ^
         *   \   |   /
         *    \  |  /
         *     \ | /
         *      \|/
         * --------------
         * </pre>
         * See above for how the reflection is calculated.
         *
         * this = This vector, so pointing away from the surface. <br>
         * N    = The normal of the surface (parameter 'n' in this method). <br>
         * R    = The reflected vector, returned by this method.<br>
         *
         * @param n The normal to use as center of reflection.
         * @result The reflected version of this vector.
         */
        MCORE_INLINE Vector3 Reflect(const Vector3& n) const                                { const float fac = 2 * (n.x * x + n.y * y + n.z * z); return Vector3(fac * n.x - x, fac * n.y - y, fac * n.z - z); }

        /**
         * Mirror a given vector over a plane with a given normal.
         * This is basically the inverted value as returned by Vector3::Reflect.
         * <pre>
         *
         * this  N (plane normal)
         *  ^    ^
         *   \   |
         *    \  |
         *     \ |
         *      \|
         * ------------------ plane surface
         *      /|
         *     / |
         *    /  |
         *   /   |
         *  /
         * R (the returned vector)
         *
         * --------------
         * </pre>
         * See above for how the mirror vector is calculated.
         *
         * this = This vector, so pointing away from the surface. <br>
         * N    = The normal of the plane (parameter 'n' in this method). <br>
         * R    = The reflected vector, returned by this method.<br>
         *
         * @param n The normal of the plane to use as mirror.
         * @result The mirrored version of this vector.
         */
        MCORE_INLINE Vector3 Mirror(const Vector3& n) const                                 { const float fac = 2.0f * (n.x * x + n.y * y + n.z * z); return Vector3(x - fac * n.x, y - fac * n.y, z - fac * n.z); }

        /**
         * Set all vector component values to zero.
         */
        MCORE_INLINE void Zero()                                                            { x = 0.0f; y = 0.0f; z = 0.0f; }

        MCORE_INLINE void ProjectNormalized(const Vector3& projectOnto)                     { *this = projectOnto; *this *= projectOnto.Dot(*this); }
        MCORE_INLINE void Project(const Vector3& projectOnto)
        {
            const float ontoSqLen = projectOnto.SquareLength();
            if (ontoSqLen > 0.0f)
            {
                *this = projectOnto;
            }
            *this *= (projectOnto.Dot(*this) / ontoSqLen);
        }
        MCORE_INLINE Vector3 ProjectedNormalized(const Vector3& projectOnto) const          { Vector3 result = projectOnto; result *= projectOnto.Dot(*this); return result; }
        MCORE_INLINE Vector3 Projected(const Vector3& projectOnto) const
        {
            Vector3 result = projectOnto;
            const float ontoSqLen = projectOnto.SquareLength();
            if (ontoSqLen > 0.0f)
            {
                result *= (projectOnto.Dot(*this) / ontoSqLen);
            }
            return result;
        }

        // operators
        MCORE_INLINE Vector3        operator-() const                                       { return Vector3(-x, -y, -z); }
        MCORE_INLINE const Vector3& operator+=(const Vector3& v)                            { x += v.x; y += v.y; z += v.z; return *this; }
        MCORE_INLINE const Vector3& operator-=(const Vector3& v)                            { x -= v.x; y -= v.y; z -= v.z; return *this; }
        MCORE_INLINE const Vector3& operator*=(float m)                                     { x *= m; y *= m; z *= m; return *this; }
        //      MCORE_INLINE const Vector3& operator*=(double m)                                    { x*=m; y*=m; z*=m; return *this; }
        MCORE_INLINE const Vector3& operator=(const Vector3& v)                             { x = v.x; y = v.y; z = v.z; return *this; }
        MCORE_INLINE const Vector3& operator/=(const Vector3& v)                            { x /= v.x; y /= v.y; z /= v.z; return *this; }
        MCORE_INLINE const Vector3& operator/=(float d)                                     { const float iMul = 1.0f / d; x *= iMul; y *= iMul; z *= iMul; return *this; }
        //      MCORE_INLINE const Vector3& operator/=(double d)                                    { const float iMul=1.0f/d; x*=iMul; y*=iMul; z*=iMul; return *this; }
        MCORE_INLINE bool           operator==(const Vector3& v) const                      { return ((v.x == x) && (v.y == y) && (v.z == z)); }
        MCORE_INLINE bool           operator!=(const Vector3& v) const                      { return ((v.x != x) || (v.y != y) || (v.z != z)); }

        //MCORE_INLINE float&       operator[](const uint32 row)                            { return ((float*)&x)[row]; }
        //MCORE_INLINE const float& operator[](const uint32 row) const                      { return ((const float*)&x)[row]; }
        MCORE_INLINE operator       float*()                                                { return (float*)&x; }
        MCORE_INLINE operator       const float*() const                                    { return (const float*)&x; }

    public:
        float   x, y, z;
    };

    MCORE_INLINE Vector3    operator* (const Vector3& v,    const Vector3&  v2)     { return Vector3(v.x * v2.x, v.y * v2.y, v.z * v2.z); }
    MCORE_INLINE Vector3    operator+ (const Vector3& v,    const Vector3&  v2)     { return Vector3(v.x + v2.x, v.y + v2.y, v.z + v2.z); }
    MCORE_INLINE Vector3    operator- (const Vector3& v,    const Vector3&  v2)     { return Vector3(v.x - v2.x, v.y - v2.y, v.z - v2.z); }
    MCORE_INLINE Vector3    operator* (const Vector3& v,    float m)                { return Vector3(v.x * m, v.y * m, v.z * m); }
    MCORE_INLINE Vector3    operator* (float m,             const Vector3&  v)      { return Vector3(m * v.x, m * v.y, m * v.z); }
    //MCORE_INLINE Vector3  operator* (const Vector3& v,    double m)               { return Vector3(v.x*m, v.y*m, v.z*m); }
    //MCORE_INLINE Vector3  operator* (double m,            const Vector3&  v)      { return Vector3(m*v.x, m*v.y, m*v.z); }
    MCORE_INLINE Vector3    operator/ (const Vector3& v,    const Vector3& d)       { return Vector3(v.x / d.x, v.y / d.y, v.z / d.z); }
    MCORE_INLINE Vector3    operator/ (const Vector3& v,    float d)                { float iMul = 1.0f / d; return Vector3(v.x * iMul, v.y * iMul, v.z * iMul); }
    MCORE_INLINE Vector3    operator/ (float d,             const Vector3&  v)      { return Vector3(d / v.x, d / v.y, d / v.z); }
    //MCORE_INLINE Vector3  operator/ (const Vector3& v,    double d)               { float iMul=1.0f/d; return Vector3(v.x*iMul, v.y*iMul, v.z*iMul); }
    //MCORE_INLINE Vector3  operator/ (double d,            const Vector3&  v)      { return Vector3(d/v.x, d/v.y, d/v.z); }
}   // namespace MCore
