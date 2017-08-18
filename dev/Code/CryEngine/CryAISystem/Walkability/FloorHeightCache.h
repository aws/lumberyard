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

#ifndef CRYINCLUDE_CRYAISYSTEM_WALKABILITY_FLOORHEIGHTCACHE_H
#define CRYINCLUDE_CRYAISYSTEM_WALKABILITY_FLOORHEIGHTCACHE_H
#pragma once


class FloorHeightCache
{
public:
    void Reset();
    void SetHeight(const Vec3& position, float height);
    bool GetHeight(const Vec3& position, float& height) const;

    Vec3 GetCellCenter(const Vec3& position) const;
    AABB GetAABB(Vec3 position) const;

    void Draw(const ColorB& colorGood, const ColorB& colorBad);
    size_t GetMemoryUsage() const;

private:
    static const float CellSize;
    static const float InvCellSize;
    static const float HalfCellSize;

    Vec3 GetCellCenter(uint16 x, uint16 y) const
    {
        return Vec3(x * CellSize + HalfCellSize, y * CellSize + HalfCellSize, 0.0f);
    }

    struct CacheEntry
    {
        CacheEntry()
        {
        }

        CacheEntry(Vec3 position, float _height = 0.0f)
            : height(_height)
        {
            x = (uint16)(position.x * InvCellSize);
            y = (uint16)(position.y * InvCellSize);
            z = (uint16)(position.z);
        }

        bool operator<(const CacheEntry& rhs) const
        {
            if (x != rhs.x)
            {
                return x < rhs.x;
            }

            if (y != rhs.y)
            {
                return y < rhs.y;
            }

            return z < rhs.z;
        }

        uint16 x;
        uint16 y;
        uint16 z;

        float height;
    };

    typedef std::set<CacheEntry> FloorHeights;
    FloorHeights m_floorHeights;
};

#endif // CRYINCLUDE_CRYAISYSTEM_WALKABILITY_FLOORHEIGHTCACHE_H
