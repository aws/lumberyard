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

// include the required headers
#include "StandardHeaders.h"
#include "Vector.h"
#include "Algorithms.h"


namespace MCore
{
    /**
     * An axis aligned bounding box (AABB).
     * This box is constructed out of two 3D points, a minimum and a maximum.
     * This means the box does not rotate, but always keeps aligned each axis.
     * Usage for this template could be for building a bounding volume around any given 3D object and
     * use it to accelerate ray tracing or visibility tests.
     */
    class AABB
    {
    public:
        /**
         * Default constructor. Initializes the min and max point to the extremes.
         * This means it's basically inside out at infinite size. So you are ready to start encapsulating points.
         */
        MCORE_INLINE AABB()                                                                     { Init(); }

        /**
         * Constructor which inits the minimum and maximum point of the box.
         * @param minPnt The minimum point.
         * @param maxPnt The maximum point.
         */
        MCORE_INLINE AABB(const Vector3& minPnt, const Vector3& maxPnt)
            : mMin(minPnt)
            , mMax(maxPnt) {}

        /**
         * Initialize the box minimum and maximum points.
         * Sets the points to floating point maximum and minimum. After calling this method you are ready to encapsulate points in it.
         * Note, that the default constructor already calls this method. So you should only call it when you want to 'reset' the minimum and
         * maximum points of the box.
         */
        MCORE_INLINE void Init()                                                                { mMin.Set(FLT_MAX, FLT_MAX, FLT_MAX); mMax.Set(-FLT_MAX, -FLT_MAX, -FLT_MAX); }

        /**
         * Check if this is a valid AABB or not.
         * The box is only valid if the minimum values are all smaller or equal than the maximum values.
         * @result True when the AABB is valid, otherwise false is returned.
         */
        MCORE_INLINE bool CheckIfIsValid() const
        {
            if (mMin.x > mMax.x)
            {
                return false;
            }
            if (mMin.y > mMax.y)
            {
                return false;
            }
            if (mMin.z > mMax.z)
            {
                return false;
            }
            return true;
        }

        /**
         * Encapsulate a point in the box.
         * This means that the box will automatically calculate the new minimum and maximum points when needed.
         * @param v The vector (3D point) to 'add' to the box.
         */
        MCORE_INLINE void Encapsulate(const Vector3& v);

        /**
         * Encapsulate another AABB with this box.
         * This method automatically adjusts the minimum and maximum point of the box after 'adding' the given AABB to this box.
         * @param box The AABB to 'add' to this box.
         */
        MCORE_INLINE void Encapsulate(const AABB& box)                                          { Encapsulate(box.mMin); Encapsulate(box.mMax); }

        /**
         * Widen the box in all dimensions with a given number of units.
         * @param delta The delta size in units to enlarge the box with. The delta value will be added to the maximum point as well as subtracted from the minimum point.
         */
        MCORE_INLINE void Widen(float delta)                                                    { mMin.x -= delta; mMin.y -= delta; mMin.z -= delta; mMax.x += delta; mMax.y += delta; mMax.z += delta; }

        /**
         * Translates the box with a given offset vector.
         * This means the middle of the box will be moved by the given vector.
         * @param offset The offset vector to translate (move) the box with.
         */
        MCORE_INLINE void Translate(const Vector3& offset)                                      { mMin += offset; mMax += offset; }

        /**
         * Checks if a given point is inside this box or not.
         * Note that the edges/planes of the box are also counted as 'inside'.
         * @param v The vector (3D point) to perform the test with.
         * @result Returns true when the given point is inside the box, otherwise false is returned.
         */
        MCORE_INLINE bool Contains(const Vector3& v) const                                      { return (InRange(v.x, mMin.x, mMax.x) && InRange(v.z, mMin.z, mMax.z) && InRange(v.y, mMin.y, mMax.y)); }

        /**
         * Checks if a given AABB is COMPLETELY inside this box or not.
         * Note that the edges/planes of this box are counted as 'inside'.
         * @param box The AABB to perform the test with.
         * @result Returns true when the AABB 'b' is COMPLETELY inside this box. If it's completely or partially outside, false will be returned.
         */
        MCORE_INLINE bool Contains(const AABB& box) const                                       { return (Contains(box.mMin) && Contains(box.mMax)); }

        /**
         * Checks if a given AABB partially or completely contains, so intersects, this box or not.
         * Please note that the edges of this box are counted as 'inside'.
         * @param box The AABB to perform the test with.
         * @result Returns true when the given AABB 'b' is completely or partially inside this box. Only false will be returned when the given AABB 'b' is COMPLETELY outside this box.
         */
        MCORE_INLINE bool Intersects(const AABB& box) const                                     { return !(mMin.x > box.mMax.x || mMax.x < box.mMin.x || mMin.y > box.mMax.y || mMax.y < box.mMin.y || mMin.z > box.mMax.z || mMax.z < box.mMin.z); }

        /**
         * Calculates and returns the width of the box.
         * The width is the distance between the minimum and maximum point, along the X-axis.
         * @result The width of the box.
         */
        MCORE_INLINE float CalcWidth() const                                                    { return mMax.x - mMin.x; }

        /**
         * Calculates and returns the height of the box.
         * The height is the distance between the minimum and maximum point, along the Z-axis.
         * @result The height of the box.
         */
        MCORE_INLINE float CalcHeight() const                                                   { return mMax.z - mMin.z; }

        /**
         * Calculates and returns the depth of the box.
         * The depth is the distance between the minimum and maximum point, along the Y-axis.
         * @result The depth of the box.
         */
        MCORE_INLINE float CalcDepth() const                                                    { return mMax.y - mMin.y; }

        /**
         * Calculate the volume of the box.
         * This equals width x height x depth.
         * @result The volume of the box.
         */
        MCORE_INLINE float CalcVolume() const                                                   { return (mMax.x - mMin.x) * (mMax.y - mMin.y) * (mMax.z - mMin.z); }

        /**
         * Calculate the surface area of the box.
         * @result The surface area.
         */
        MCORE_INLINE float CalcSurfaceArea() const
        {
            const float width   = mMax.x - mMin.x;
            const float height  = mMax.z - mMin.z;
            const float depth   = mMax.y - mMin.y;
            return (2.0f * height * width) + (2.0f * height * depth) + (2.0f * width * depth);
        }

        /**
         * Calculates the center/middle of the box.
         * This is simply done by taking the average of the minimum and maximum point along each axis.
         * @result The center (or middle) point of this box.
         */
        MCORE_INLINE Vector3 CalcMiddle() const                                                 { return (mMin + mMax) * 0.5f; }

        /**
         * Calculates the extents of the box.
         * This is the vector from the center to a corner of the box.
         * @result The vector containing the extents.
         */
        MCORE_INLINE Vector3 CalcExtents() const                                                { return (mMax - mMin) * 0.5f; }

        /**
         * Calculates the radius of this box.
         * With radius we mean the length of the vector from the center of the box to the minimum or maximum point.
         * So if you would construct a sphere with as center the middle of this box and with a radius returned by this method, you will
         * get the minimum sphere which exactly contains this box.
         * @result The length of the center of the box to one of the extreme points. So the minimum radius of the bounding sphere containing this box.
         */
        MCORE_INLINE float CalcRadius() const                                                   { return (mMax - mMin).SafeLength() * 0.5f; }

        /**
         * Get the minimum point of the box.
         * @result The minimum point of the box.
         */
        MCORE_INLINE const Vector3& GetMin() const                                              { return mMin; }

        /**
         * Get the maximum point of the box.
         * @result The maximum point of the box.
         */
        MCORE_INLINE const Vector3& GetMax() const                                              { return mMax; }

        /**
         * Set the minimum point of the box.
         * @param minVec The vector representing the minimum point of the box.
         */
        MCORE_INLINE void SetMin(const Vector3& minVec)                                         { mMin = minVec; }

        /**
         * Set the maximum point of the box.
         * @param maxVec The vector representing the maximum point of the box.
         */
        MCORE_INLINE void SetMax(const Vector3& maxVec)                                         { mMax = maxVec; }

        void CalcCornerPoints(Vector3* outPoints) const
        {
            outPoints[0].Set(mMin.x, mMin.y, mMax.z);       //   4-------------5
            outPoints[1].Set(mMax.x, mMin.y, mMax.z);       //  /|           / |
            outPoints[2].Set(mMax.x, mMin.y, mMin.z);       // 0------------1  |
            outPoints[3].Set(mMin.x, mMin.y, mMin.z);       // | |          |  |
            outPoints[4].Set(mMin.x, mMax.y, mMax.z);       // | 7----------|--6
            outPoints[5].Set(mMax.x, mMax.y, mMax.z);       // | /          | /
            outPoints[6].Set(mMax.x, mMax.y, mMin.z);       // |/           |/
            outPoints[7].Set(mMin.x, mMax.y, mMin.z);       // 3------------2
        }


    private:
        Vector3     mMin;   /**< The minimum point. */
        Vector3     mMax;   /**< The maximum point. */
    };


    // encapsulate a point in the box
    MCORE_INLINE void AABB::Encapsulate(const Vector3& v)
    {
        mMin.x = Min(mMin.x, v.x);
        mMin.y = Min(mMin.y, v.y);
        mMin.z = Min(mMin.z, v.z);

        mMax.x = Max(mMax.x, v.x);
        mMax.y = Max(mMax.y, v.y);
        mMax.z = Max(mMax.z, v.z);
    }
}   // namespace MCore
