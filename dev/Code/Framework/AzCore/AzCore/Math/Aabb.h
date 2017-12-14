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
#ifndef AZCORE_MATH_AABB_H
#define AZCORE_MATH_AABB_H 1

#include <AzCore/base.h>
#include <AzCore/Math/Vector3.h>

namespace AZ
{
    class Obb;

    /**
     * An axis aligned bounding box. It is defined as an closed set, i.e. it includes the boundary, so it
     * will always include at least one point.
     */
    class Aabb
    {
        friend void MathReflect(class BehaviorContext&);
        friend void MathReflect(class SerializeContext&);
    public:
        //AZ_DECLARE_CLASS_POOL_ALLOCATOR(Aabb);
        AZ_TYPE_INFO(Aabb, "{A54C2B36-D5B8-46A1-A529-4EBDBD2450E7}")

        Aabb()  { }

        ///Creates a null AABB, this is an invalid AABB which has no size, but is useful as adding a point to it will make it valid
        static AZ_MATH_FORCE_INLINE const Aabb  CreateNull()    { Aabb result; result.m_min = Vector3(g_fltMax); result.m_max = Vector3(-g_fltMax); return result; }

        static AZ_MATH_FORCE_INLINE const Aabb  CreateFromPoint(const Vector3& p)
        {
            Aabb aabb;
            aabb.m_min = p;
            aabb.m_max = p;
            return aabb;
        }

        static AZ_MATH_FORCE_INLINE const Aabb  CreateFromMinMax(const Vector3& min, const Vector3& max)
        {
            Aabb aabb;
            aabb.m_min = min;
            aabb.m_max = max;
            AZ_Assert(aabb.IsValid(), "Min must be less than Max");
            return aabb;
        }

        static AZ_MATH_FORCE_INLINE const Aabb  CreateCenterHalfExtents(const Vector3& center, const Vector3& halfExtents)
        {
            Aabb aabb;
            aabb.m_min = center - halfExtents;
            aabb.m_max = center + halfExtents;
            return aabb;
        }

        static AZ_MATH_FORCE_INLINE const Aabb  CreateCenterRadius(const Vector3& center, const VectorFloat& radius) { return CreateCenterHalfExtents(center, Vector3(radius)); }

        ///Creates an AABB which contains the specified points
        static AZ_MATH_FORCE_INLINE const Aabb  CreatePoints(const Vector3* pts, int numPts)
        {
            Aabb aabb = Aabb::CreateFromPoint(pts[0]);
            for (int i = 1; i < numPts; ++i)
            {
                aabb.AddPoint(pts[i]);
            }
            return aabb;
        }

        ///Creates an AABB which contains the specified OBB
        static const Aabb CreateFromObb(const Obb& obb);


        AZ_MATH_FORCE_INLINE const Vector3& GetMin() const      { return m_min; }
        AZ_MATH_FORCE_INLINE const Vector3& GetMax() const      { return m_max; }

        AZ_MATH_FORCE_INLINE void Set(const Vector3& min, const Vector3& max)
        {
            m_min = min;
            m_max = max;
            AZ_Assert(IsValid(), "Min must be less than Max");
        }
        AZ_MATH_FORCE_INLINE void SetMin(const Vector3& min)    { m_min = min; }
        AZ_MATH_FORCE_INLINE void SetMax(const Vector3& max)    { m_max = max; }

        AZ_FORCE_INLINE bool operator==(const AZ::Aabb& aabb) const
        {
            return (GetMin() == aabb.GetMin()) && (GetMax() == aabb.GetMax());
        }

        AZ_FORCE_INLINE bool operator!=(const AZ::Aabb& aabb) const
        {
            return !(*this == aabb);
        }

        AZ_MATH_FORCE_INLINE const VectorFloat GetWidth() const     { return m_max.GetX() - m_min.GetX(); }
        AZ_MATH_FORCE_INLINE const VectorFloat GetHeight() const        { return m_max.GetY() - m_min.GetY(); }
        AZ_MATH_FORCE_INLINE const VectorFloat GetDepth() const     { return m_max.GetZ() - m_min.GetZ(); }
        AZ_MATH_FORCE_INLINE const Vector3 GetExtents() const           { return m_max - m_min; }
        AZ_MATH_FORCE_INLINE const Vector3 GetCenter() const            { return 0.5f * (m_min + m_max); }
        AZ_MATH_FORCE_INLINE void GetAsSphere(Vector3& center, VectorFloat& radius) const
        {
            center = GetCenter();
            radius = (m_max - center).GetLength();
        }
        AZ_MATH_FORCE_INLINE bool Contains(const Vector3& v) const
        {
            return v.IsGreaterEqualThan(m_min) && v.IsLessEqualThan(m_max);
        }
        AZ_MATH_FORCE_INLINE bool Contains(const Aabb& aabb) const
        {
            return aabb.GetMin().IsGreaterEqualThan(m_min) && aabb.GetMax().IsLessEqualThan(m_max);
        }
        AZ_MATH_FORCE_INLINE bool Overlaps(const Aabb& aabb) const
        {
            return m_min.IsLessEqualThan(aabb.m_max) && m_max.IsGreaterEqualThan(aabb.m_min);
        }

        AZ_MATH_FORCE_INLINE void Expand(const Vector3& delta)
        {
            AZ_Assert(delta.IsGreaterEqualThan(Vector3::CreateZero()), "delta must be positive");
            m_min -= delta;
            m_max += delta;
        }
        AZ_MATH_FORCE_INLINE const Aabb GetExpanded(const Vector3& delta) const
        {
            AZ_Assert(delta.IsGreaterEqualThan(Vector3::CreateZero()), "delta must be positive");
            return Aabb::CreateFromMinMax(m_min - delta, m_max + delta);
        }

        AZ_MATH_FORCE_INLINE void AddPoint(const Vector3& p)
        {
            m_min = m_min.GetMin(p);
            m_max = m_max.GetMax(p);
        }

        AZ_MATH_FORCE_INLINE void AddAabb(const Aabb& box)
        {
            m_min = m_min.GetMin(box.GetMin());
            m_max = m_max.GetMax(box.GetMax());
        }

        ///Calculates distance from the AABB to specified point, a point inside the AABB will return zero
        AZ_MATH_FORCE_INLINE const VectorFloat GetDistance(const Vector3& p) const
        {
            Vector3 closest = p.GetClamp(m_min, m_max);
            return p.GetDistance(closest);
        }

        ///Calculates distance from the AABB to specified point, a point inside the AABB will return zero
        AZ_MATH_FORCE_INLINE const VectorFloat GetDistanceSq(const Vector3& p) const
        {
            Vector3 closest = p.GetClamp(m_min, m_max);
            return p.GetDistanceSq(closest);
        }

        ///Clamps the AABB to be contained within the specified AABB
        AZ_MATH_FORCE_INLINE const Aabb GetClamped(const Aabb& clamp) const
        {
            Aabb clampedAabb = Aabb::CreateFromMinMax(m_min, m_max);
            clampedAabb.Clamp(clamp);
            return clampedAabb;
        }

        AZ_MATH_FORCE_INLINE void       Clamp(const Aabb& clamp)
        {
            m_min = m_min.GetClamp(clamp.m_min, clamp.m_max);
            m_max = m_max.GetClamp(clamp.m_min, clamp.m_max);
        }

        AZ_MATH_FORCE_INLINE void SetNull()
        {
            m_min = Vector3(g_fltMax);
            m_max = Vector3(-g_fltMax);
        }

        AZ_MATH_FORCE_INLINE void Translate(const Vector3& offset)
        {
            m_min += offset;
            m_max += offset;
        }

        AZ_MATH_FORCE_INLINE const Aabb GetTranslated(const Vector3& offset) const
        {
            return Aabb::CreateFromMinMax(m_min + offset, m_max + offset);
        }

        AZ_MATH_FORCE_INLINE float GetSurfaceArea() const
        {
            VectorFloat w = GetWidth();
            VectorFloat h = GetHeight();
            VectorFloat d = GetDepth();
            return VectorFloat(2.0f) * (w * h + h * d + d * w);
        }

        /************************************************************************
         *
         ************************************************************************/
        void            ApplyTransform(const Transform& transform);

        /**
         * Transforms an Aabb and returns the resulting Obb.
         */
        const class Obb GetTransformedObb(const Transform& transform) const;

        /**
         * Returns a new AABB containing the transformed AABB
         */
        AZ_MATH_FORCE_INLINE const Aabb     GetTransformedAabb(const Transform& transform) const
        {
            Aabb aabb = Aabb::CreateFromMinMax(m_min, m_max);
            aabb.ApplyTransform(transform);
            return aabb;
        }

        AZ_MATH_FORCE_INLINE bool IsValid() const                   { return m_min.IsLessEqualThan(m_max); }

        AZ_MATH_FORCE_INLINE bool IsFinite() const { return m_min.IsFinite() && m_max.IsFinite(); }

    protected:
        Vector3 m_min;
        Vector3 m_max;
    };
}

#ifndef AZ_PLATFORM_WINDOWS // Remove this once all compilers support POD (MSVC already does)
#   include <AzCore/std/typetraits/is_pod.h>
AZSTD_DECLARE_POD_TYPE(AZ::Aabb);
#endif

#endif
#pragma once