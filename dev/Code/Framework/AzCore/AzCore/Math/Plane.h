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
#ifndef AZCORE_MATH_PLANE_H
#define AZCORE_MATH_PLANE_H 1

#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Transform.h>

namespace AZ
{
    /**
     * Plane defined by a Vector4 with components (A,B,C,D), the plane equation used is Ax+By+Cz+D=0. The distance of
     * a point from the plane is given by Ax+By+Cz+D, or just simply plane.Dot4(point).
     *
     * Note that the D component is often referred to as 'distance' below, in the sense that it is the distance of the
     * origin from the plane. This is NOT the same as what many other people refer to as the plane distance, which
     * would be the negation of this.
     *
     * TODO: Remove references to D as 'distance', it's confusing and unnecessary. Refer to it either as just the D
     *       coefficient, or provide support functions.
     */
    class Plane
    {
        friend void MathReflect(class BehaviorContext&);
        friend void MathReflect(class SerializeContext&);
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Plane);
        AZ_TYPE_INFO(Plane, "{847DD984-9DBF-4789-8E25-E0334402E8AD}")

        Plane() { }

        static AZ_MATH_FORCE_INLINE const Plane CreateFromNormalAndPoint(const Vector3& normal, const Vector3& point)
        {
            Plane result;
            result.Set(normal, -normal.Dot(point));
            return result;
        }

        static AZ_MATH_FORCE_INLINE const Plane CreateFromNormalAndDistance(const Vector3& normal, float dist)
        {
            Plane result;
            result.Set(normal, dist);
            return result;
        }

        static AZ_MATH_FORCE_INLINE const Plane CreateFromCoefficients(const VectorFloat& a, const VectorFloat& b, const VectorFloat& c, const VectorFloat& d)
        {
            Plane result;
            result.Set(a, b, c, d);
            return result;
        }

        AZ_MATH_FORCE_INLINE void           Set(const Vector4& plane)               { m_plane = plane; }
        AZ_MATH_FORCE_INLINE void           Set(const Vector3& normal, float d)     { m_plane.Set(normal, d); }
        AZ_MATH_FORCE_INLINE void           Set(float a, float b, float c, float d) { m_plane.Set(a, b, c, d); }

        AZ_MATH_FORCE_INLINE void           SetNormal(const Vector3& normal)
        {
            m_plane.SetX(normal.GetX());
            m_plane.SetY(normal.GetY());
            m_plane.SetZ(normal.GetZ());
        }

        AZ_MATH_FORCE_INLINE void           SetDistance(float d)                    { m_plane.SetW(d); }

        ///Applies the specified transform to the plane, returns the transformed plane
        AZ_MATH_FORCE_INLINE const Plane    GetTransform(const Transform& tm) const
        {
            float newDist = -GetDistance();
            newDist += m_plane.GetX() * (tm.GetColumn(0).Dot(tm.GetTranslation()));
            newDist += m_plane.GetY() * (tm.GetColumn(1).Dot(tm.GetTranslation()));
            newDist += m_plane.GetZ() * (tm.GetColumn(2).Dot(tm.GetTranslation()));
            Vector3 normal = GetNormal();
            normal = tm.Multiply3x3(normal);
            return CreateFromNormalAndDistance(normal, -newDist);
        }

        AZ_MATH_FORCE_INLINE void           ApplyTransform(const Transform& tm)     { *this = GetTransform(tm); }

        ///Returns the coefficients of the plane equation as a Vector4, i.e. (A,B,C,D)
        AZ_MATH_FORCE_INLINE const Vector4& GetPlaneEquationCoefficients() const    { return m_plane; }

        AZ_MATH_FORCE_INLINE const Vector3  GetNormal() const           { return m_plane.GetAsVector3(); }
        AZ_MATH_FORCE_INLINE float          GetDistance() const         { return m_plane.GetW(); }

        ///Calculates the distance between a point and the plane
        AZ_MATH_FORCE_INLINE const VectorFloat  GetPointDist(const Vector3& pos) const  { return m_plane.Dot3(pos) + m_plane.GetW(); }

        ///Project vector onto a plane
        AZ_MATH_FORCE_INLINE Vector3 GetProjected(const Vector3& v) const   { Vector3 n = m_plane.GetAsVector3(); return v - (n * v.Dot(n)); }

        /// Returns true if ray plane intersect, otherwise false. If true hitPoint contains the result.
        AZ_MATH_FORCE_INLINE bool CastRay(const Vector3& start, const Vector3& dir, Vector3& hitPoint) const
        {
            VectorFloat t;
            if (!CastRay(start, dir, t))
            {
                return false;
            }
            hitPoint = start + dir * t;
            return true;
        }

        /// Returns true if ray plane intersect, otherwise false. If true hitTime contains the result.
        AZ_MATH_FORCE_INLINE bool CastRay(const Vector3& start, const Vector3& dir, VectorFloat& hitTime) const
        {
            VectorFloat nDotDir = m_plane.Dot3(dir);
            if (nDotDir.IsZero())
            {
                return false;
            }
            hitTime = -(m_plane.GetW() + m_plane.Dot3(start)) * nDotDir.GetReciprocal();
            return true;
        }

        /// Intersect the plane with line segment. Return true if they intersect otherwise false.
        AZ_MATH_FORCE_INLINE bool IntersectSegment(const Vector3& start, const Vector3& end, Vector3& hitPoint) const
        {
            Vector3 dir = end - start;
            VectorFloat hitTime;
            if (!CastRay(start, dir, hitTime))
            {
                return false;
            }
            if (hitTime.IsGreaterEqualThan(VectorFloat::CreateZero()) && hitTime.IsLessEqualThan(VectorFloat::CreateOne()))
            {
                hitPoint = start + dir * hitTime;
                return true;
            }
            return false;
        }

        /// Intersect the plane with line segment. Return true if they intersect otherwise false.
        AZ_MATH_FORCE_INLINE bool IntersectSegment(const Vector3& start, const Vector3& end, VectorFloat& hitTime) const
        {
            Vector3 dir = end - start;
            if (!CastRay(start, dir, hitTime))
            {
                return false;
            }
            if (hitTime.IsGreaterEqualThan(VectorFloat::CreateZero()) && hitTime.IsLessEqualThan(VectorFloat::CreateOne()))
            {
                return true;
            }
            return false;
        }

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return m_plane.IsFinite(); }

        AZ_MATH_FORCE_INLINE bool operator==(const Plane& rhs) const
        {
            return m_plane.IsClose(rhs.m_plane);
        }

        AZ_MATH_FORCE_INLINE bool operator!=(const Plane& rhs) const
        {
            return !(*this == rhs);
        }

    private:
        Vector4     m_plane;        ///< plane normal (x,y,z) and negative distance to the origin (w)
    };
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Plane);
#endif

#endif  // AZCORE_MATH_PLANE_H
#pragma once