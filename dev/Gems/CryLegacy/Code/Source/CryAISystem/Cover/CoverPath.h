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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYAISYSTEM_COVER_COVERPATH_H
#define CRYINCLUDE_CRYAISYSTEM_COVER_COVERPATH_H
#pragma once


#include "Cover.h"


class CoverPath
{
public:
    enum PathFlags
    {
        Looped = 1 << 0,
    };

    struct Point
    {
        Point()
            : position(ZERO)
            , distance(0.0f)
        {
        }
        Point(const Vec3& pos, float dist)
            : position(pos)
            , distance(dist)
        {
        }

        Vec3 position;
        float distance;
    };

    CoverPath();

    typedef std::vector<Point> Points;
    const Points& GetPoints() const;

    bool Empty() const;
    void Clear();
    void Reserve(uint32 pointCount);
    void AddPoint(const Vec3& point);

    Vec3 GetPointAt(float distance) const;
    Vec3 GetClosestPoint(const Vec3& point, float* distanceToPath, float* distanceOnPath = 0) const;
    float GetDistanceAt(const Vec3& point, float tolerance = 0.05f) const;
    Vec3 GetNormalAt(const Vec3& point) const;

    float GetLength() const;

    uint32 GetFlags() const;
    void SetFlags(uint32 flags);

    bool Intersect(const Vec3& origin, const Vec3& dir, float* distance, Vec3* point) const;

    void DebugDraw();

private:
    Points m_points;
    uint32 m_flags;
};


#endif // CRYINCLUDE_CRYAISYSTEM_COVER_COVERPATH_H
